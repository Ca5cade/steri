#include "turtle_bot_exploration/exploration_coordinator.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace turtle_bot_exploration
{

ExplorationCoordinator::ExplorationCoordinator(const rclcpp::NodeOptions & options)
: Node("exploration_coordinator", options),
  current_state_(ExplorationState::IDLE),
  map_received_(false),
  frontiers_received_(false),
  exploration_active_(false),
  goal_active_(false),
  goal_completion_override_(false),
  total_distance_traveled_(0.0),
  exploration_cycles_(0)
{
  // Initialize parameters - Enhanced with dynamic optimization
  this->declare_parameter("exploration_timeout", 300.0);  // 5 minutes
  this->declare_parameter("stuck_timeout", 30.0);
  this->declare_parameter("min_map_coverage_to_start", 0.1);
  this->declare_parameter("goal_tolerance", 0.8);  // Increased tolerance
  this->declare_parameter("stuck_distance_threshold", 0.15);  // More lenient stuck detection
  this->declare_parameter("goal_reached_tolerance", 1.2);  // Liberal goal reached detection
  this->declare_parameter("goal_timeout", 45.0);  // Maximum time to reach a goal
  this->declare_parameter("force_new_goal_distance", 1.5);  // Force new goal if within this distance
  this->declare_parameter("recovery_rotation_speed", 0.5);
  this->declare_parameter("recovery_rotation_duration", 3.0);
  this->declare_parameter("robot_base_frame", "base_link");
  this->declare_parameter("global_frame", "map");
  this->declare_parameter("auto_start_exploration", true);
  this->declare_parameter("enable_intelligent_selection", true);
  this->declare_parameter("time_efficiency_weight", 0.3);
  this->declare_parameter("exploration_value_weight", 0.4);

  // Enhanced optimization parameters
  this->declare_parameter("enable_dynamic_tolerance", true);
  this->declare_parameter("velocity_based_tolerance", true);
  this->declare_parameter("min_dynamic_tolerance", 0.8);
  this->declare_parameter("max_dynamic_tolerance", 3.0);
  this->declare_parameter("velocity_tolerance_factor", 2.0);
  this->declare_parameter("environmental_adaptation", true);
  this->declare_parameter("performance_monitoring", true);
  this->declare_parameter("adaptive_parameter_tuning", true);
  this->declare_parameter("goal_completion_smoothing", true);
  this->declare_parameter("transition_optimization", true);
  this->declare_parameter("distance_efficiency_weight", 0.3);
  this->declare_parameter("frontier_selection_frequency", 2.0);

  exploration_timeout_ = this->get_parameter("exploration_timeout").as_double();
  stuck_timeout_ = this->get_parameter("stuck_timeout").as_double();
  min_map_coverage_to_start_ = this->get_parameter("min_map_coverage_to_start").as_double();
  goal_tolerance_ = this->get_parameter("goal_tolerance").as_double();
  stuck_distance_threshold_ = this->get_parameter("stuck_distance_threshold").as_double();
  goal_reached_tolerance_ = this->get_parameter("goal_reached_tolerance").as_double();
  goal_timeout_ = this->get_parameter("goal_timeout").as_double();
  force_new_goal_distance_ = this->get_parameter("force_new_goal_distance").as_double();
  recovery_rotation_speed_ = this->get_parameter("recovery_rotation_speed").as_double();
  recovery_rotation_duration_ = this->get_parameter("recovery_rotation_duration").as_double();
  robot_base_frame_ = this->get_parameter("robot_base_frame").as_string();
  global_frame_ = this->get_parameter("global_frame").as_string();
  auto_start_exploration_ = this->get_parameter("auto_start_exploration").as_bool();

  // Enhanced optimization parameters
  enable_dynamic_tolerance_ = this->get_parameter("enable_dynamic_tolerance").as_bool();
  velocity_based_tolerance_ = this->get_parameter("velocity_based_tolerance").as_bool();
  min_dynamic_tolerance_ = this->get_parameter("min_dynamic_tolerance").as_double();
  max_dynamic_tolerance_ = this->get_parameter("max_dynamic_tolerance").as_double();
  velocity_tolerance_factor_ = this->get_parameter("velocity_tolerance_factor").as_double();
  environmental_adaptation_ = this->get_parameter("environmental_adaptation").as_bool();
  performance_monitoring_ = this->get_parameter("performance_monitoring").as_bool();
  adaptive_parameter_tuning_ = this->get_parameter("adaptive_parameter_tuning").as_bool();
  goal_completion_smoothing_ = this->get_parameter("goal_completion_smoothing").as_bool();
  transition_optimization_ = this->get_parameter("transition_optimization").as_bool();

  // Initialize performance metrics
  performance_metrics_.goals_completed = 0;
  performance_metrics_.goals_failed = 0;
  performance_metrics_.average_completion_time = 0.0;
  performance_metrics_.total_exploration_time = 0.0;
  performance_metrics_.efficiency_score = 0.0;
  performance_metrics_.last_update_time = std::chrono::steady_clock::now();

  // Initialize TF2
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Initialize subscribers and publishers
  map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
    "/map", rclcpp::QoS(1).transient_local(),
    std::bind(&ExplorationCoordinator::mapCallback, this, std::placeholders::_1));

  frontier_markers_sub_ = this->create_subscription<visualization_msgs::msg::MarkerArray>(
    "/exploration/frontiers", 10,
    std::bind(&ExplorationCoordinator::frontierMarkersCallback, this, std::placeholders::_1));

  status_pub_ = this->create_publisher<std_msgs::msg::String>(
    "/exploration/status", 10);

  exploration_markers_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
    "/exploration/markers", 10);

  cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
    "/cmd_vel", 10);

  // Initialize action client
  nav_action_client_ = rclcpp_action::create_client<nav2_msgs::action::NavigateToPose>(
    this, "/navigate_to_pose");

  // Timer for main control loop
  timer_ = this->create_wall_timer(
    std::chrono::milliseconds(500),
    std::bind(&ExplorationCoordinator::timerCallback, this));

  // Initialize timestamps
  last_movement_time_ = std::chrono::steady_clock::now();
  state_start_time_ = std::chrono::steady_clock::now();
  last_frontier_update_ = std::chrono::steady_clock::now();

  RCLCPP_INFO(this->get_logger(), "Exploration coordinator initialized");
  
  if (auto_start_exploration_) {
    setState(ExplorationState::DETECTING_ENVIRONMENT);
    RCLCPP_INFO(this->get_logger(), "Auto-start exploration enabled");
  }
}

void ExplorationCoordinator::mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  current_map_ = msg;
  map_received_ = true;
  RCLCPP_DEBUG(this->get_logger(), "Received map update");
}

void ExplorationCoordinator::frontierMarkersCallback(
  const visualization_msgs::msg::MarkerArray::SharedPtr msg)
{
  available_frontiers_.clear();
  
  for (const auto & marker : msg->markers) {
    if (marker.ns == "frontiers" && marker.action == visualization_msgs::msg::Marker::ADD) {
      ExplorationGoal frontier_goal;
      frontier_goal.pose.header = marker.header;
      frontier_goal.pose.pose.position = marker.pose.position;
      // No orientation requirement - robot can face any direction at frontier
      frontier_goal.pose.pose.orientation.x = 0.0;
      frontier_goal.pose.pose.orientation.y = 0.0;
      frontier_goal.pose.pose.orientation.z = 0.0;
      frontier_goal.pose.pose.orientation.w = 1.0;  // Neutral orientation (no specific direction)
      frontier_goal.frontier_id = marker.id;
      frontier_goal.created_time = std::chrono::steady_clock::now();
      
      // Extract information from marker color (green component represents information gain)
      frontier_goal.information_gain = marker.color.g;
      
      // Calculate distance to robot
      auto robot_pose = getRobotPose();
      double dx = frontier_goal.pose.pose.position.x - robot_pose.pose.position.x;
      double dy = frontier_goal.pose.pose.position.y - robot_pose.pose.position.y;
      frontier_goal.distance = std::sqrt(dx * dx + dy * dy);

      // Only add frontier if it's safe (not too close to obstacles)
      if (isFrontierSafe(frontier_goal)) {
        available_frontiers_.push_back(frontier_goal);
      } else {
        RCLCPP_DEBUG(this->get_logger(), "Rejected unsafe frontier %d", frontier_goal.frontier_id);
      }
    }
  }
  
  frontiers_received_ = true;
  last_frontier_update_ = std::chrono::steady_clock::now();
  
  RCLCPP_DEBUG(this->get_logger(), "Received %zu frontiers", available_frontiers_.size());
}

void ExplorationCoordinator::timerCallback()
{
  updateExplorationState();
  publishExplorationStatus();
  publishExplorationMarkers();
}

void ExplorationCoordinator::updateExplorationState()
{
  auto current_time = std::chrono::steady_clock::now();
  auto state_duration = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - state_start_time_).count();

  switch (current_state_) {
    case ExplorationState::IDLE:
      if (auto_start_exploration_ && map_received_ && shouldStartExploration()) {
        setState(ExplorationState::DETECTING_ENVIRONMENT);
      }
      break;

    case ExplorationState::DETECTING_ENVIRONMENT:
      if (map_received_ && isInUnmappedEnvironment()) {
        setState(ExplorationState::EXPLORING);
        exploration_active_ = true;
        RCLCPP_INFO(this->get_logger(), "Starting autonomous exploration");
      } else if (state_duration > 10) {  // Wait up to 10 seconds
        setState(ExplorationState::IDLE);
        RCLCPP_INFO(this->get_logger(), "Environment appears to be fully mapped");
      }
      break;

    case ExplorationState::EXPLORING:
      if (!frontiers_received_ || available_frontiers_.empty()) {
        checkExplorationCompletion();
      } else if (!goal_active_) {
        selectAndNavigateToFrontier();
      } else if (isGoalReached() || shouldForceNewGoal()) {
        // Goal reached or should be forced complete
        goal_active_ = false;
        RCLCPP_INFO(this->get_logger(), "Goal completed, selecting new frontier");
        selectAndNavigateToFrontier();
      } else if (isRobotStuck()) {
        handleStuckAtGoal();  // Use enhanced stuck handling
      }

      // Check for exploration timeout
      if (state_duration > exploration_timeout_) {
        setState(ExplorationState::EXPLORATION_COMPLETE);
        RCLCPP_WARN(this->get_logger(), "Exploration timeout reached");
      }
      break;

    case ExplorationState::NAVIGATING_TO_FRONTIER:
      if (isRobotStuck()) {
        setState(ExplorationState::STUCK_RECOVERY);
      }
      break;

    case ExplorationState::STUCK_RECOVERY:
      if (state_duration < 12.0) {  // Extended recovery time for multi-stage process
        handleStuckRecovery();
      } else {
        setState(ExplorationState::EXPLORING);
        goal_active_ = false;
        RCLCPP_INFO(this->get_logger(), "Stuck recovery completed, resuming exploration");
      }
      break;

    case ExplorationState::EXPLORATION_COMPLETE:
      exploration_active_ = false;
      goal_active_ = false;
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                          "Exploration completed. Total distance: %.2f m, Cycles: %d",
                          total_distance_traveled_, exploration_cycles_);
      break;

    case ExplorationState::ERROR:
      exploration_active_ = false;
      goal_active_ = false;
      RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                           "Exploration in error state");
      break;
  }
}

bool ExplorationCoordinator::isInUnmappedEnvironment()
{
  if (!current_map_) return false;

  // Check if there are unknown areas in the map
  int unknown_count = 0;
  int total_count = 0;

  for (const auto & cell : current_map_->data) {
    if (cell == -1) {
      unknown_count++;
    }
    total_count++;
  }

  double unknown_ratio = static_cast<double>(unknown_count) / total_count;
  return unknown_ratio > 0.1;  // More than 10% unknown
}

bool ExplorationCoordinator::shouldStartExploration()
{
  if (!map_received_ || !current_map_) return false;

  double coverage = calculateMapCoverage();
  return coverage >= min_map_coverage_to_start_ && isInUnmappedEnvironment();
}

void ExplorationCoordinator::selectAndNavigateToFrontier()
{
  if (available_frontiers_.empty()) {
    RCLCPP_WARN(this->get_logger(), "No frontiers available for exploration");
    checkExplorationCompletion();
    return;
  }

  auto best_frontier = selectBestFrontier();
  sendNavigationGoal(best_frontier);
  current_goal_ = best_frontier;
  setState(ExplorationState::NAVIGATING_TO_FRONTIER);
}

ExplorationGoal ExplorationCoordinator::selectBestFrontier()
{
  if (available_frontiers_.empty()) {
    throw std::runtime_error("No frontiers available");
  }

  // FIXED: Avoid recently visited frontiers to prevent loops
  static std::vector<int> recently_visited_frontiers;
  static auto last_cleanup = std::chrono::steady_clock::now();

  // Clean up old visited frontiers every 30 seconds
  auto current_time = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(current_time - last_cleanup).count() > 30) {
    recently_visited_frontiers.clear();
    last_cleanup = current_time;
  }

  ExplorationGoal best_frontier;
  double best_score = -1.0;
  bool found_valid_frontier = false;

  auto current_pose = getRobotPose();
  double map_coverage = calculateMapCoverage();

  for (const auto & frontier : available_frontiers_) {
    // FIXED: Skip recently visited frontiers
    bool recently_visited = std::find(recently_visited_frontiers.begin(),
                                     recently_visited_frontiers.end(),
                                     frontier.frontier_id) != recently_visited_frontiers.end();

    if (recently_visited) {
      RCLCPP_DEBUG(this->get_logger(), "Skipping recently visited frontier %d", frontier.frontier_id);
      continue;
    }

    // FIXED: Require minimum distance to avoid immediate goals
    double distance = calculateDistance(current_pose.pose.position, frontier.pose.pose.position);
    if (distance < 2.0) {  // Minimum 2 meters distance
      RCLCPP_DEBUG(this->get_logger(), "Skipping too close frontier %d (distance: %.2fm)",
                   frontier.frontier_id, distance);
      continue;
    }

    // Simple but effective scoring: prioritize information gain and reasonable distance
    double score = frontier.information_gain / (1.0 + distance * 0.1);

    if (score > best_score) {
      best_score = score;
      best_frontier = frontier;
      found_valid_frontier = true;
    }
  }

  if (!found_valid_frontier) {
    // If no valid frontier found, use the first available one
    best_frontier = available_frontiers_[0];
    RCLCPP_WARN(this->get_logger(), "No valid frontier found, using first available frontier %d",
                best_frontier.frontier_id);
  } else {
    // Add selected frontier to recently visited list
    recently_visited_frontiers.push_back(best_frontier.frontier_id);
    if (recently_visited_frontiers.size() > 5) {  // Keep only last 5
      recently_visited_frontiers.erase(recently_visited_frontiers.begin());
    }
  }

  RCLCPP_INFO(this->get_logger(),
              "Selected frontier %d with score %.3f (gain: %.3f, dist: %.2fm, coverage: %.1f%%)",
              best_frontier.frontier_id, best_score,
              best_frontier.information_gain, best_frontier.distance, map_coverage * 100.0);

  return best_frontier;
}

void ExplorationCoordinator::sendNavigationGoal(const ExplorationGoal & goal)
{
  if (!nav_action_client_->wait_for_action_server(std::chrono::seconds(5))) {
    RCLCPP_ERROR(this->get_logger(), "Navigation action server not available");
    setState(ExplorationState::ERROR);
    return;
  }

  auto nav_goal = nav2_msgs::action::NavigateToPose::Goal();
  nav_goal.pose = goal.pose;

  auto send_goal_options = rclcpp_action::Client<nav2_msgs::action::NavigateToPose>::SendGoalOptions();
  send_goal_options.goal_response_callback =
    std::bind(&ExplorationCoordinator::goalResponseCallback, this, std::placeholders::_1);
  send_goal_options.feedback_callback =
    std::bind(&ExplorationCoordinator::feedbackCallback, this, std::placeholders::_1, std::placeholders::_2);
  send_goal_options.result_callback =
    std::bind(&ExplorationCoordinator::resultCallback, this, std::placeholders::_1);

  auto goal_handle_future = nav_action_client_->async_send_goal(nav_goal, send_goal_options);
  goal_active_ = true;

  // Initialize goal tracking variables
  goal_start_time_ = std::chrono::steady_clock::now();
  goal_set_pose_ = getRobotPose();
  goal_completion_override_ = false;
  current_goal_ = goal;

  RCLCPP_INFO(this->get_logger(), "Sent navigation goal to frontier %d at (%.2f, %.2f)",
              goal.frontier_id, goal.pose.pose.position.x, goal.pose.pose.position.y);
}

void ExplorationCoordinator::goalResponseCallback(
  const rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr & goal_handle)
{
  if (!goal_handle) {
    RCLCPP_ERROR(this->get_logger(), "Navigation goal was rejected");
    goal_active_ = false;
    setState(ExplorationState::EXPLORING);
  } else {
    current_goal_handle_ = goal_handle;
    RCLCPP_INFO(this->get_logger(), "Navigation goal accepted");
  }
}

void ExplorationCoordinator::feedbackCallback(
  rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr,
  const std::shared_ptr<const nav2_msgs::action::NavigateToPose::Feedback> feedback)
{
  // Update robot position tracking
  auto current_pose = getRobotPose();
  if (last_robot_pose_.header.stamp.sec != 0) {
    double dx = current_pose.pose.position.x - last_robot_pose_.pose.position.x;
    double dy = current_pose.pose.position.y - last_robot_pose_.pose.position.y;
    double distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 0.01) {  // Minimum movement threshold
      total_distance_traveled_ += distance;
      last_movement_time_ = std::chrono::steady_clock::now();
    }
  }
  last_robot_pose_ = current_pose;

  RCLCPP_DEBUG(this->get_logger(), "Navigation feedback: distance remaining %.2f",
               feedback->distance_remaining);
}

void ExplorationCoordinator::resultCallback(
  const rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::WrappedResult & result)
{
  goal_active_ = false;
  current_goal_handle_.reset();
  exploration_cycles_++;

  switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      RCLCPP_INFO(this->get_logger(), "Navigation goal succeeded - waiting before next goal");
      // FIXED: Add delay to ensure proper exploration and avoid rapid goal cycling
      std::this_thread::sleep_for(std::chrono::seconds(3));
      setState(ExplorationState::EXPLORING);
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_WARN(this->get_logger(), "Navigation goal aborted");
      setState(ExplorationState::STUCK_RECOVERY);
      break;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_INFO(this->get_logger(), "Navigation goal canceled");
      setState(ExplorationState::EXPLORING);
      break;
    default:
      RCLCPP_ERROR(this->get_logger(), "Navigation goal failed with unknown result");
      setState(ExplorationState::ERROR);
      break;
  }
}

void ExplorationCoordinator::handleStuckRecovery()
{
  static int recovery_stage = 0;
  static std::chrono::steady_clock::time_point stage_start_time = std::chrono::steady_clock::now();
  static int recovery_attempts = 0;

  auto current_time = std::chrono::steady_clock::now();
  auto stage_duration = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - stage_start_time).count();

  // Enhanced multi-stage recovery with adaptive strategies
  switch (recovery_stage) {
    case 0:  // Stage 1: Intelligent backing with obstacle avoidance
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                          "Enhanced stuck recovery stage 1: intelligent backing (attempt %d)", recovery_attempts + 1);
      performIntelligentBacking();
      if (stage_duration > 3) {
        recovery_stage = 1;
        stage_start_time = current_time;
      }
      break;

    case 1:  // Stage 2: Adaptive rotation with path analysis
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                          "Enhanced stuck recovery stage 2: adaptive rotation with path analysis");
      performAdaptiveRotation();
      if (stage_duration > recovery_rotation_duration_) {
        recovery_stage = 2;
        stage_start_time = current_time;
      }
      break;

    case 2:  // Stage 3: Strategic forward movement with monitoring
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                          "Enhanced stuck recovery stage 3: strategic forward movement");
      performStrategicForwardMovement();
      if (stage_duration > 5) {
        recovery_stage = 3;
        stage_start_time = current_time;
      }
      break;

    case 3:  // Stage 4: Alternative goal selection
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                          "Enhanced stuck recovery stage 4: alternative goal selection");
      if (performAlternativeGoalSelection()) {
        recovery_stage = 0;  // Reset for next recovery cycle
        recovery_attempts = 0;
        setState(ExplorationState::EXPLORING);
        return;
      }
      if (stage_duration > 3) {
        recovery_stage = 4;
        stage_start_time = current_time;
      }
      break;

    case 4:  // Stage 5: Emergency unstuck maneuvers
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                          "Enhanced stuck recovery stage 5: emergency unstuck maneuvers");
      performEmergencyUnstuckManeuvers();
      if (stage_duration > 8) {
        recovery_stage = 0;  // Reset for next recovery cycle
        recovery_attempts++;
        stage_start_time = current_time;

        // If too many recovery attempts, force goal completion
        if (recovery_attempts > 3) {
          RCLCPP_WARN(this->get_logger(), "Multiple recovery attempts failed, forcing goal completion");
          goal_completion_override_ = true;
          recovery_attempts = 0;
          setState(ExplorationState::EXPLORING);
        }
      }
      break;
  }
}

void ExplorationCoordinator::performIntelligentBacking()
{
  // Enhanced backing that considers obstacle distribution
  geometry_msgs::msg::Twist cmd_vel;

  // Analyze surrounding obstacles to determine best backing direction
  auto current_pose = getRobotPose();
  double best_backing_angle = 0.0;  // Default: straight back

  // Simple obstacle avoidance - can be enhanced with costmap analysis
  cmd_vel.linear.x = -0.15;  // Slower, more controlled backing
  cmd_vel.angular.z = best_backing_angle * 0.3;  // Slight angular adjustment

  cmd_vel_pub_->publish(cmd_vel);
}

void ExplorationCoordinator::performAdaptiveRotation()
{
  // Adaptive rotation that analyzes free space
  geometry_msgs::msg::Twist cmd_vel;

  // Vary rotation speed based on recovery attempt
  static int rotation_direction = 1;  // 1 for CCW, -1 for CW

  double adaptive_speed = recovery_rotation_speed_ * (0.8 + 0.4 * sin(std::chrono::steady_clock::now().time_since_epoch().count() * 0.001));
  cmd_vel.angular.z = rotation_direction * adaptive_speed;

  // Alternate rotation direction for better space exploration
  static auto last_direction_change = std::chrono::steady_clock::now();
  if (std::chrono::steady_clock::now() - last_direction_change > std::chrono::seconds(2)) {
    rotation_direction *= -1;
    last_direction_change = std::chrono::steady_clock::now();
  }

  cmd_vel_pub_->publish(cmd_vel);
}

void ExplorationCoordinator::performStrategicForwardMovement()
{
  // Strategic forward movement with continuous monitoring
  geometry_msgs::msg::Twist cmd_vel;

  // Gradual acceleration for better control
  static double forward_speed = 0.05;
  forward_speed = std::min(0.2, forward_speed + 0.02);

  cmd_vel.linear.x = forward_speed;

  // Add slight oscillation to help navigate tight spaces
  double oscillation = 0.1 * sin(std::chrono::steady_clock::now().time_since_epoch().count() * 0.002);
  cmd_vel.angular.z = oscillation;

  cmd_vel_pub_->publish(cmd_vel);
}

bool ExplorationCoordinator::performAlternativeGoalSelection()
{
  // Try to select an alternative frontier that's more accessible
  if (available_frontiers_.size() > 1) {
    auto current_pose = getRobotPose();

    // Find the closest alternative frontier
    ExplorationGoal alternative_goal;
    double min_distance = std::numeric_limits<double>::max();
    bool found_alternative = false;

    for (const auto & frontier : available_frontiers_) {
      if (frontier.frontier_id != current_goal_.frontier_id) {
        double distance = calculateDistance(current_pose.pose.position, frontier.pose.pose.position);
        if (distance < min_distance && distance > 1.0) {  // Not too close
          min_distance = distance;
          alternative_goal = frontier;
          found_alternative = true;
        }
      }
    }

    if (found_alternative) {
      RCLCPP_INFO(this->get_logger(), "Selected alternative goal: frontier %d at distance %.2fm",
                  alternative_goal.frontier_id, min_distance);
      sendNavigationGoal(alternative_goal);
      current_goal_ = alternative_goal;
      goal_active_ = true;
      return true;
    }
  }

  return false;
}

void ExplorationCoordinator::performEmergencyUnstuckManeuvers()
{
  // Emergency maneuvers for severe stuck situations
  static int maneuver_type = 0;
  static auto maneuver_start = std::chrono::steady_clock::now();

  auto current_time = std::chrono::steady_clock::now();
  auto maneuver_duration = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - maneuver_start).count();

  geometry_msgs::msg::Twist cmd_vel;

  switch (maneuver_type) {
    case 0:  // Rapid backing
      cmd_vel.linear.x = -0.3;
      if (maneuver_duration > 2) {
        maneuver_type = 1;
        maneuver_start = current_time;
      }
      break;

    case 1:  // Fast rotation
      cmd_vel.angular.z = 1.5;
      if (maneuver_duration > 2) {
        maneuver_type = 2;
        maneuver_start = current_time;
      }
      break;

    case 2:  // Diagonal movement
      cmd_vel.linear.x = 0.2;
      cmd_vel.angular.z = 0.8;
      if (maneuver_duration > 3) {
        maneuver_type = 0;
        maneuver_start = current_time;
      }
      break;
  }

  cmd_vel_pub_->publish(cmd_vel);
}

bool ExplorationCoordinator::validateExplorationProgress(const geometry_msgs::msg::PoseStamped & current_pose)
{
  // Check if we've actually made exploration progress
  static auto last_validation_time = std::chrono::steady_clock::now();
  static geometry_msgs::msg::Point last_validation_position;
  static double last_map_coverage = 0.0;

  auto current_time = std::chrono::steady_clock::now();
  auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - last_validation_time).count();

  // Only validate every 5 seconds to avoid too frequent checks
  if (time_since_last < 5) {
    return true;  // Allow completion if we haven't checked recently
  }

  // Check if robot has moved significantly
  double distance_moved = calculateDistance(current_pose.pose.position, last_validation_position);

  // Check if map coverage has increased
  double current_coverage = calculateMapCoverage();
  double coverage_increase = current_coverage - last_map_coverage;

  // Validation criteria: robot moved at least 0.5m OR map coverage increased by 1%
  bool movement_validated = distance_moved > 0.5;
  bool coverage_validated = coverage_increase > 0.01;

  RCLCPP_INFO(this->get_logger(),
              "Exploration validation - Movement: %.2fm (req: 0.5m), Coverage increase: %.1f%% (req: 1%%), Valid: %s",
              distance_moved, coverage_increase * 100.0,
              (movement_validated || coverage_validated) ? "YES" : "NO");

  // Update validation state
  last_validation_time = current_time;
  last_validation_position = current_pose.pose.position;
  last_map_coverage = current_coverage;

  return movement_validated || coverage_validated;
}

void ExplorationCoordinator::checkExplorationCompletion()
{
  // Check if exploration should be considered complete
  auto current_time = std::chrono::steady_clock::now();
  auto time_since_last_frontier = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - last_frontier_update_).count();
  auto exploration_duration = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - state_start_time_).count();

  // Calculate current map coverage
  double map_coverage = calculateMapCoverage();

  // Multiple completion criteria
  bool no_frontiers_timeout = available_frontiers_.empty() && time_since_last_frontier > 15;
  bool environment_fully_mapped = !isInUnmappedEnvironment() && map_coverage > 0.85;
  bool high_coverage_no_progress = map_coverage > 0.90 && time_since_last_frontier > 30;
  bool exploration_cycles_exceeded = exploration_cycles_ > 50;  // Prevent infinite loops
  bool stuck_too_long = exploration_duration > 60 && available_frontiers_.empty();

  if (no_frontiers_timeout) {
    setState(ExplorationState::EXPLORATION_COMPLETE);
    RCLCPP_INFO(this->get_logger(),
                "No frontiers available for %ld seconds - exploration complete (Coverage: %.1f%%)",
                time_since_last_frontier, map_coverage * 100.0);
  } else if (environment_fully_mapped) {
    setState(ExplorationState::EXPLORATION_COMPLETE);
    RCLCPP_INFO(this->get_logger(),
                "Environment fully mapped - exploration complete (Coverage: %.1f%%)",
                map_coverage * 100.0);
  } else if (high_coverage_no_progress) {
    setState(ExplorationState::EXPLORATION_COMPLETE);
    RCLCPP_INFO(this->get_logger(),
                "High coverage achieved with no progress - exploration complete (Coverage: %.1f%%)",
                map_coverage * 100.0);
  } else if (exploration_cycles_exceeded) {
    setState(ExplorationState::EXPLORATION_COMPLETE);
    RCLCPP_WARN(this->get_logger(),
                "Maximum exploration cycles exceeded (%d) - stopping exploration",
                exploration_cycles_);
  } else if (stuck_too_long) {
    setState(ExplorationState::EXPLORATION_COMPLETE);
    RCLCPP_WARN(this->get_logger(),
                "Robot appears stuck with no frontiers - stopping exploration");
  }

  // Log exploration progress periodically
  RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 10000,
                      "Exploration progress: %.1f%% coverage, %d cycles, %zu frontiers",
                      map_coverage * 100.0, exploration_cycles_, available_frontiers_.size());
}

geometry_msgs::msg::PoseStamped ExplorationCoordinator::getRobotPose()
{
  geometry_msgs::msg::PoseStamped robot_pose;
  robot_pose.header.frame_id = global_frame_;
  robot_pose.header.stamp = this->get_clock()->now();

  try {
    auto transform = tf_buffer_->lookupTransform(
      global_frame_, robot_base_frame_, tf2::TimePointZero);

    robot_pose.pose.position.x = transform.transform.translation.x;
    robot_pose.pose.position.y = transform.transform.translation.y;
    robot_pose.pose.position.z = transform.transform.translation.z;
    robot_pose.pose.orientation = transform.transform.rotation;
  } catch (const tf2::TransformException & ex) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                         "Could not get robot pose: %s", ex.what());
    // Return default pose at origin
    robot_pose.pose.position.x = 0.0;
    robot_pose.pose.position.y = 0.0;
    robot_pose.pose.position.z = 0.0;
    robot_pose.pose.orientation.w = 1.0;
  }

  return robot_pose;
}

double ExplorationCoordinator::calculateMapCoverage()
{
  if (!current_map_) return 0.0;

  int known_count = 0;
  int total_count = 0;

  for (const auto & cell : current_map_->data) {
    if (cell != -1) {  // Known (free or occupied)
      known_count++;
    }
    total_count++;
  }

  return total_count > 0 ? static_cast<double>(known_count) / total_count : 0.0;
}

bool ExplorationCoordinator::isRobotStuck()
{
  auto current_time = std::chrono::steady_clock::now();
  auto time_since_movement = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - last_movement_time_).count();

  return time_since_movement > stuck_timeout_;
}

void ExplorationCoordinator::publishExplorationStatus()
{
  std_msgs::msg::String status_msg;
  status_msg.data = stateToString(current_state_);
  status_pub_->publish(status_msg);
}

void ExplorationCoordinator::publishExplorationMarkers()
{
  visualization_msgs::msg::MarkerArray marker_array;

  // Add current goal marker if active
  if (goal_active_ && current_goal_.frontier_id >= 0) {
    visualization_msgs::msg::Marker goal_marker;
    goal_marker.header.frame_id = global_frame_;
    goal_marker.header.stamp = this->get_clock()->now();
    goal_marker.ns = "exploration_goal";
    goal_marker.id = 0;
    goal_marker.type = visualization_msgs::msg::Marker::ARROW;
    goal_marker.action = visualization_msgs::msg::Marker::ADD;

    goal_marker.pose = current_goal_.pose.pose;
    goal_marker.scale.x = 0.5;
    goal_marker.scale.y = 0.1;
    goal_marker.scale.z = 0.1;
    goal_marker.color.r = 1.0;
    goal_marker.color.g = 0.0;
    goal_marker.color.b = 0.0;
    goal_marker.color.a = 1.0;

    marker_array.markers.push_back(goal_marker);
  }

  exploration_markers_pub_->publish(marker_array);
}

void ExplorationCoordinator::setState(ExplorationState new_state)
{
  if (new_state != current_state_) {
    RCLCPP_INFO(this->get_logger(), "State transition: %s -> %s",
                stateToString(current_state_).c_str(),
                stateToString(new_state).c_str());
    current_state_ = new_state;
    state_start_time_ = std::chrono::steady_clock::now();
  }
}

std::string ExplorationCoordinator::stateToString(ExplorationState state)
{
  switch (state) {
    case ExplorationState::IDLE: return "IDLE";
    case ExplorationState::DETECTING_ENVIRONMENT: return "DETECTING_ENVIRONMENT";
    case ExplorationState::EXPLORING: return "EXPLORING";
    case ExplorationState::NAVIGATING_TO_FRONTIER: return "NAVIGATING_TO_FRONTIER";
    case ExplorationState::STUCK_RECOVERY: return "STUCK_RECOVERY";
    case ExplorationState::EXPLORATION_COMPLETE: return "EXPLORATION_COMPLETE";
    case ExplorationState::ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

bool ExplorationCoordinator::isFrontierSafe(const ExplorationGoal & frontier)
{
  if (!current_map_) {
    return false;
  }

  double x = frontier.pose.pose.position.x;
  double y = frontier.pose.pose.position.y;

  // Check minimum distance to obstacles (safety margin)
  double min_safe_distance = 0.8;  // 80cm safety margin
  double distance_to_obstacle = getDistanceToNearestObstacle(x, y);

  if (distance_to_obstacle < min_safe_distance) {
    RCLCPP_DEBUG(this->get_logger(),
                 "Frontier rejected: too close to obstacle (%.2fm < %.2fm)",
                 distance_to_obstacle, min_safe_distance);
    return false;
  }

  return true;
}

double ExplorationCoordinator::getDistanceToNearestObstacle(double x, double y)
{
  if (!current_map_) {
    return 0.0;
  }

  // Convert world coordinates to map coordinates
  int map_x = static_cast<int>((x - current_map_->info.origin.position.x) / current_map_->info.resolution);
  int map_y = static_cast<int>((y - current_map_->info.origin.position.y) / current_map_->info.resolution);

  // Check bounds
  if (map_x < 0 || map_x >= static_cast<int>(current_map_->info.width) ||
      map_y < 0 || map_y >= static_cast<int>(current_map_->info.height)) {
    return 0.0;  // Out of bounds, consider unsafe
  }

  double min_distance = std::numeric_limits<double>::max();
  int search_radius = static_cast<int>(1.0 / current_map_->info.resolution);  // 1 meter search radius

  // Search in a radius around the point
  for (int dx = -search_radius; dx <= search_radius; dx++) {
    for (int dy = -search_radius; dy <= search_radius; dy++) {
      int check_x = map_x + dx;
      int check_y = map_y + dy;

      // Check bounds
      if (check_x >= 0 && check_x < static_cast<int>(current_map_->info.width) &&
          check_y >= 0 && check_y < static_cast<int>(current_map_->info.height)) {

        int index = check_y * current_map_->info.width + check_x;

        // Check if this cell is an obstacle (occupied > 50)
        if (current_map_->data[index] > 50) {
          double distance = std::sqrt(dx * dx + dy * dy) * current_map_->info.resolution;
          min_distance = std::min(min_distance, distance);
        }
      }
    }
  }

  return (min_distance == std::numeric_limits<double>::max()) ? 1.0 : min_distance;
}

double ExplorationCoordinator::calculateDistance(
  const geometry_msgs::msg::Point & p1, const geometry_msgs::msg::Point & p2)
{
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  return sqrt(dx * dx + dy * dy);
}

double ExplorationCoordinator::calculateDynamicTolerance(const geometry_msgs::msg::PoseStamped & current_pose)
{
  if (!enable_dynamic_tolerance_) {
    return goal_reached_tolerance_;
  }

  double base_tolerance = goal_reached_tolerance_;
  double dynamic_tolerance = base_tolerance;

  // Velocity-based tolerance adjustment
  if (velocity_based_tolerance_) {
    auto current_time = std::chrono::steady_clock::now();
    if (last_robot_pose_.header.stamp.sec != 0) {
      auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - last_pose_update_time_).count() / 1000.0;

      if (time_diff > 0.01) {  // Avoid division by zero
        double dx = current_pose.pose.position.x - last_robot_pose_.pose.position.x;
        double dy = current_pose.pose.position.y - last_robot_pose_.pose.position.y;
        double velocity = sqrt(dx * dx + dy * dy) / time_diff;

        // Adjust tolerance based on velocity - higher velocity = larger tolerance
        dynamic_tolerance = base_tolerance + (velocity * velocity_tolerance_factor_);
      }
    }
  }

  // Environmental adaptation
  if (environmental_adaptation_) {
    // Adjust based on frontier density and map complexity
    double frontier_density = available_frontiers_.size() / std::max(1.0, calculateMapCoverage() * 100.0);
    if (frontier_density > 0.5) {
      dynamic_tolerance *= 1.2;  // Increase tolerance in dense frontier areas
    }
  }

  // Clamp to reasonable bounds
  dynamic_tolerance = std::max(min_dynamic_tolerance_,
                              std::min(max_dynamic_tolerance_, dynamic_tolerance));

  return dynamic_tolerance;
}

bool ExplorationCoordinator::evaluateSmoothCompletion(double distance, double tolerance)
{
  if (!goal_completion_smoothing_) {
    return false;
  }

  // Check if robot is approaching goal smoothly
  auto current_time = std::chrono::steady_clock::now();
  auto goal_duration = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - goal_start_time_).count();

  // If robot has been working on this goal for a reasonable time and is close
  if (goal_duration > 10 && distance < tolerance * 1.5) {
    // Check if robot is making progress toward goal
    if (last_distance_to_goal_ > 0 && distance < last_distance_to_goal_) {
      return true;  // Robot is getting closer, allow smooth completion
    }
  }

  last_distance_to_goal_ = distance;
  return false;
}

void ExplorationCoordinator::updatePerformanceMetrics(bool goal_completed, double final_distance, double tolerance)
{
  if (!performance_monitoring_) {
    return;
  }

  auto current_time = std::chrono::steady_clock::now();
  auto completion_time = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - goal_start_time_).count();

  if (goal_completed) {
    performance_metrics_.goals_completed++;

    // Update average completion time
    double total_time = performance_metrics_.average_completion_time *
                       (performance_metrics_.goals_completed - 1) + completion_time;
    performance_metrics_.average_completion_time = total_time / performance_metrics_.goals_completed;

    // Calculate efficiency score (closer to goal = higher efficiency)
    double efficiency = 1.0 - (final_distance / tolerance);
    performance_metrics_.efficiency_score =
      (performance_metrics_.efficiency_score * (performance_metrics_.goals_completed - 1) + efficiency) /
      performance_metrics_.goals_completed;
  } else {
    performance_metrics_.goals_failed++;
  }

  // Update total exploration time
  auto total_time = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - performance_metrics_.last_update_time).count();
  performance_metrics_.total_exploration_time += total_time;
  performance_metrics_.last_update_time = current_time;

  // Adaptive parameter tuning based on performance
  if (adaptive_parameter_tuning_) {
    adaptParametersBasedOnPerformance();
  }
}

void ExplorationCoordinator::adaptParametersBasedOnPerformance()
{
  if (performance_metrics_.goals_completed < 5) {
    return;  // Need sufficient data for adaptation
  }

  double success_rate = static_cast<double>(performance_metrics_.goals_completed) /
                       (performance_metrics_.goals_completed + performance_metrics_.goals_failed);

  // If success rate is low, increase tolerances
  if (success_rate < 0.8) {
    goal_reached_tolerance_ = std::min(max_dynamic_tolerance_, goal_reached_tolerance_ * 1.1);
    force_new_goal_distance_ = std::min(max_dynamic_tolerance_ * 1.5, force_new_goal_distance_ * 1.1);

    RCLCPP_INFO(this->get_logger(),
                "Adaptive tuning: Increased tolerances due to low success rate (%.2f)", success_rate);
  }

  // If completion time is too high, optimize for speed
  if (performance_metrics_.average_completion_time > 45.0) {
    goal_timeout_ = std::max(20.0, goal_timeout_ * 0.9);

    RCLCPP_INFO(this->get_logger(),
                "Adaptive tuning: Reduced timeout due to slow completion (%.2fs avg)",
                performance_metrics_.average_completion_time);
  }
}

double ExplorationCoordinator::calculateCurrentVelocity()
{
  auto current_pose = getRobotPose();
  auto current_time = std::chrono::steady_clock::now();

  if (last_robot_pose_.header.stamp.sec != 0) {
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      current_time - last_pose_update_time_).count() / 1000.0;

    if (time_diff > 0.01) {
      double dx = current_pose.pose.position.x - last_robot_pose_.pose.position.x;
      double dy = current_pose.pose.position.y - last_robot_pose_.pose.position.y;
      return sqrt(dx * dx + dy * dy) / time_diff;
    }
  }

  return 0.0;
}

double ExplorationCoordinator::calculateDistanceScore(const ExplorationGoal & frontier,
                                                     const geometry_msgs::msg::PoseStamped & robot_pose)
{
  double distance = calculateDistance(robot_pose.pose.position, frontier.pose.pose.position);

  // Inverse distance scoring - closer frontiers get higher scores
  double max_reasonable_distance = 10.0;  // Adjust based on environment
  double normalized_distance = std::min(distance / max_reasonable_distance, 1.0);

  return 1.0 - normalized_distance;
}

double ExplorationCoordinator::calculateInformationScore(const ExplorationGoal & frontier, double map_coverage)
{
  // Higher information gain gets higher score
  double max_expected_gain = 100.0;  // Adjust based on typical frontier sizes
  double normalized_gain = std::min(frontier.information_gain / max_expected_gain, 1.0);

  // Boost information score in early exploration phases
  if (map_coverage < 0.3) {
    normalized_gain *= 1.2;
  }

  return normalized_gain;
}

double ExplorationCoordinator::calculateAccessibilityScore(const ExplorationGoal & frontier,
                                                          const geometry_msgs::msg::PoseStamped & robot_pose)
{
  // Simple accessibility check - can be enhanced with path planning
  double distance = calculateDistance(robot_pose.pose.position, frontier.pose.pose.position);

  // Penalize very distant frontiers as they may be less accessible
  if (distance > 8.0) {
    return 0.5;
  } else if (distance > 5.0) {
    return 0.7;
  } else {
    return 1.0;
  }
}

double ExplorationCoordinator::calculateEfficiencyScore(const ExplorationGoal & frontier, double current_velocity)
{
  // Consider robot's current momentum and movement efficiency
  double base_efficiency = 0.8;

  // If robot is moving fast, prefer frontiers in the direction of movement
  if (current_velocity > 0.2) {
    // This could be enhanced with directional analysis
    base_efficiency += 0.2;
  }

  return base_efficiency;
}

double ExplorationCoordinator::calculateStrategicScore(const ExplorationGoal & frontier,
                                                      const geometry_msgs::msg::PoseStamped & robot_pose)
{
  // Strategic positioning considers frontier clustering and exploration patterns
  double strategic_value = 0.5;  // Base strategic value

  // Prefer frontiers that are part of larger unexplored areas
  int nearby_frontiers = 0;
  for (const auto & other_frontier : available_frontiers_) {
    if (other_frontier.frontier_id != frontier.frontier_id) {
      double distance = calculateDistance(frontier.pose.pose.position, other_frontier.pose.pose.position);
      if (distance < 3.0) {  // Within 3 meters
        nearby_frontiers++;
      }
    }
  }

  // More nearby frontiers = higher strategic value
  strategic_value += std::min(nearby_frontiers * 0.1, 0.5);

  return strategic_value;
}

double ExplorationCoordinator::applyDynamicWeighting(double base_score, const ExplorationGoal & frontier,
                                                    double map_coverage)
{
  double weighted_score = base_score;

  // Early exploration phase - prioritize information gain
  if (map_coverage < 0.3) {
    if (frontier.information_gain > 50.0) {
      weighted_score *= 1.3;
    }
  }

  // Mid exploration phase - balance distance and information
  else if (map_coverage < 0.7) {
    // Balanced approach - no special weighting
  }

  // Late exploration phase - prioritize completion and efficiency
  else {
    double distance = calculateDistance(getRobotPose().pose.position, frontier.pose.pose.position);
    if (distance < 3.0) {  // Prefer nearby frontiers for completion
      weighted_score *= 1.2;
    }
  }

  return weighted_score;
}

bool ExplorationCoordinator::isGoalReached()
{
  if (!goal_active_) {
    return false;
  }

  auto current_pose = getRobotPose();
  if (current_pose.header.frame_id.empty()) {
    return false;
  }

  // Calculate distance to current goal (POSITION ONLY - NO ORIENTATION REQUIREMENT)
  double distance = calculateDistance(
    current_pose.pose.position,
    current_goal_.pose.pose.position
  );

  // FIXED: Use conservative tolerance for real exploration
  double base_tolerance = 1.0;  // Conservative 1 meter tolerance

  // PURE DISTANCE-BASED COMPLETION - NO ORIENTATION CONSTRAINTS
  // Goal completion is based solely on x,y proximity, regardless of robot heading
  bool within_tolerance = distance < base_tolerance;
  bool timeout_reached = (std::chrono::steady_clock::now() - goal_start_time_) >
                        std::chrono::seconds(static_cast<int>(goal_timeout_));
  bool force_completion = goal_completion_override_;

  // FIXED: Add exploration validation - only complete if we've actually explored
  bool exploration_validated = false;
  if (within_tolerance) {
    exploration_validated = validateExplorationProgress(current_pose);
  }

  // Log goal status for debugging (distance-based only)
  if ((within_tolerance && exploration_validated) || timeout_reached || force_completion) {
    RCLCPP_INFO(this->get_logger(),
                "Goal completion detected (VALIDATED DISTANCE-BASED) - Distance: %.2fm, Tolerance: %.2fm, Timeout: %s, Force: %s, Exploration validated: %s",
                distance, base_tolerance, timeout_reached ? "YES" : "NO",
                force_completion ? "YES" : "NO", exploration_validated ? "YES" : "NO");
  }

  // Complete goal only when within tolerance AND exploration is validated, OR timeout/force
  return (within_tolerance && exploration_validated) || timeout_reached || force_completion;
}

bool ExplorationCoordinator::shouldForceNewGoal()
{
  if (!goal_active_) {
    return false;
  }

  auto current_pose = getRobotPose();
  if (current_pose.header.frame_id.empty()) {
    return false;
  }

  // PURE DISTANCE-BASED FORCE COMPLETION - NO ORIENTATION REQUIREMENTS
  // Check if robot is close enough to goal to force completion (position only)
  double distance_to_goal = calculateDistance(
    current_pose.pose.position,
    current_goal_.pose.pose.position
  );

  // Check if robot has moved significantly from where goal was set
  double distance_from_start = calculateDistance(
    current_pose.pose.position,
    goal_set_pose_.pose.position
  );

  // Force new goal based on DISTANCE ONLY (no orientation constraints):
  // 1. Robot is within force distance of goal (x,y proximity)
  // 2. Robot has moved at least some distance (not completely stuck)
  // 3. Goal has been active for reasonable time
  bool close_to_goal = distance_to_goal < force_new_goal_distance_;
  bool has_moved = distance_from_start > 0.5;  // Moved at least 50cm
  bool reasonable_time = (std::chrono::steady_clock::now() - goal_start_time_) >
                        std::chrono::seconds(15);  // At least 15 seconds

  if (close_to_goal && has_moved && reasonable_time) {
    RCLCPP_INFO(this->get_logger(),
                "Forcing new goal (DISTANCE-BASED) - Distance to goal: %.2fm, Distance moved: %.2fm, Robot orientation irrelevant",
                distance_to_goal, distance_from_start);
    return true;
  }

  return false;
}

void ExplorationCoordinator::handleStuckAtGoal()
{
  RCLCPP_WARN(this->get_logger(), "Robot appears stuck at goal, forcing completion");

  // Set override flag to force goal completion
  goal_completion_override_ = true;

  // Cancel current navigation goal
  if (nav_action_client_ && nav_action_client_->action_server_is_ready()) {
    nav_action_client_->async_cancel_all_goals();
  }

  // Transition to exploring state to select new frontier
  setState(ExplorationState::EXPLORING);

  // Reset override flag after a short delay
  auto timer = this->create_wall_timer(
    std::chrono::seconds(2),
    [this]() {
      goal_completion_override_ = false;
    }
  );
}

}  // namespace turtle_bot_exploration

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<turtle_bot_exploration::ExplorationCoordinator>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
