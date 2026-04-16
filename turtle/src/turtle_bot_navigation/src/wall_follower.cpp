#include "turtle_bot_navigation/wall_follower.hpp"
#include <cmath>
#include <algorithm>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/utils.h>

namespace turtle_bot_navigation
{

WallFollower::WallFollower(const rclcpp::NodeOptions & options)
: Node("wall_follower", options),
  current_state_(WallFollowingState::SEARCHING_FOR_WALL)
{
  // Declare parameters
  this->declare_parameter("target_wall_distance", 0.4);  // 40cm from wall
  this->declare_parameter("max_wall_detection_range", 2.5);  // 2.5m detection range
  this->declare_parameter("min_wall_length", 0.3);  // 30cm minimum wall length
  this->declare_parameter("wall_following_speed", 0.3);  // 30cm/s following speed
  this->declare_parameter("approach_speed", 0.2);  // 20cm/s approach speed
  this->declare_parameter("search_speed", 0.15);  // 15cm/s search speed
  this->declare_parameter("angular_gain", 2.0);  // Angular control gain
  this->declare_parameter("distance_gain", 1.5);  // Distance control gain
  this->declare_parameter("wall_confidence_threshold", 0.7);  // 70% confidence
  this->declare_parameter("max_linear_velocity", 0.5);
  this->declare_parameter("max_angular_velocity", 1.0);
  this->declare_parameter("control_frequency", 20.0);  // 20Hz control
  this->declare_parameter("min_wall_points", 5);  // Minimum points for wall
  this->declare_parameter("max_point_gap", 0.1);  // 10cm max gap between points
  this->declare_parameter("wall_angle_tolerance", 0.2);  // 0.2 rad tolerance
  this->declare_parameter("emergency_stop_distance", 0.15);  // 15cm emergency stop
  this->declare_parameter("obstacle_avoidance_distance", 0.25);  // 25cm obstacle avoid
  this->declare_parameter("wall_lost_timeout", 3.0);  // 3 seconds timeout
  this->declare_parameter("base_frame", std::string("base_link"));
  this->declare_parameter("laser_frame", std::string("laser"));

  // Get parameters
  target_wall_distance_ = this->get_parameter("target_wall_distance").as_double();
  max_wall_detection_range_ = this->get_parameter("max_wall_detection_range").as_double();
  min_wall_length_ = this->get_parameter("min_wall_length").as_double();
  wall_following_speed_ = this->get_parameter("wall_following_speed").as_double();
  approach_speed_ = this->get_parameter("approach_speed").as_double();
  search_speed_ = this->get_parameter("search_speed").as_double();
  angular_gain_ = this->get_parameter("angular_gain").as_double();
  distance_gain_ = this->get_parameter("distance_gain").as_double();
  wall_confidence_threshold_ = this->get_parameter("wall_confidence_threshold").as_double();
  max_linear_velocity_ = this->get_parameter("max_linear_velocity").as_double();
  max_angular_velocity_ = this->get_parameter("max_angular_velocity").as_double();
  control_frequency_ = this->get_parameter("control_frequency").as_double();
  min_wall_points_ = this->get_parameter("min_wall_points").as_int();
  max_point_gap_ = this->get_parameter("max_point_gap").as_double();
  wall_angle_tolerance_ = this->get_parameter("wall_angle_tolerance").as_double();
  emergency_stop_distance_ = this->get_parameter("emergency_stop_distance").as_double();
  obstacle_avoidance_distance_ = this->get_parameter("obstacle_avoidance_distance").as_double();
  wall_lost_timeout_ = this->get_parameter("wall_lost_timeout").as_double();
  base_frame_ = this->get_parameter("base_frame").as_string();
  laser_frame_ = this->get_parameter("laser_frame").as_string();

  // Initialize TF2
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Create subscribers
  laser_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
    "/scan", 10, std::bind(&WallFollower::laserScanCallback, this, std::placeholders::_1));
  
  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    "/odom", 10, std::bind(&WallFollower::odomCallback, this, std::placeholders::_1));

  // Create publishers
  cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
  wall_markers_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
    "/wall_markers", 10);

  // Create control timer
  auto timer_period = std::chrono::milliseconds(
    static_cast<int>(1000.0 / control_frequency_));
  control_timer_ = this->create_wall_timer(
    timer_period, std::bind(&WallFollower::controlTimerCallback, this));

  // Initialize state
  last_wall_seen_ = this->now();

  RCLCPP_INFO(this->get_logger(), 
              "Wall Follower initialized - Target distance: %.2fm, Detection range: %.2fm",
              target_wall_distance_, max_wall_detection_range_);
}

void WallFollower::laserScanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
{
  latest_scan_ = msg;
  
  // Detect walls in the scan
  auto walls = detectWalls(msg);
  
  // Update state based on detected walls
  updateState(walls);
  
  // Publish visualization
  publishVisualization(walls);
}

void WallFollower::odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
  current_pose_ = msg->pose.pose;
}

void WallFollower::controlTimerCallback()
{
  if (!latest_scan_) {
    return;
  }

  geometry_msgs::msg::Twist cmd_vel;
  
  // Detect walls
  auto walls = detectWalls(latest_scan_);
  
  switch (current_state_) {
    case WallFollowingState::SEARCHING_FOR_WALL:
      cmd_vel = calculateSearchCommand();
      break;
      
    case WallFollowingState::APPROACHING_WALL:
      if (!walls.empty()) {
        auto best_wall = findBestWallToFollow(walls);
        cmd_vel = calculateApproachCommand(best_wall);
      } else {
        current_state_ = WallFollowingState::SEARCHING_FOR_WALL;
        cmd_vel = calculateSearchCommand();
      }
      break;
      
    case WallFollowingState::FOLLOWING_WALL_LEFT:
    case WallFollowingState::FOLLOWING_WALL_RIGHT:
      if (!walls.empty()) {
        auto best_wall = findBestWallToFollow(walls);
        cmd_vel = calculateWallFollowingCommand(best_wall);
        current_wall_ = best_wall;
        last_wall_seen_ = this->now();
      } else {
        // Check if wall is lost
        auto time_since_wall = (this->now() - last_wall_seen_).seconds();
        if (time_since_wall > wall_lost_timeout_) {
          current_state_ = WallFollowingState::LOST_WALL;
        }
        // Continue with last known wall direction
        cmd_vel = calculateWallFollowingCommand(current_wall_);
      }
      break;
      
    case WallFollowingState::LOST_WALL:
      current_state_ = WallFollowingState::SEARCHING_FOR_WALL;
      cmd_vel = calculateSearchCommand();
      break;
      
    default:
      cmd_vel = calculateSearchCommand();
      break;
  }
  
  // Safety check - emergency stop if too close to obstacle
  bool emergency_stop = false;
  for (size_t i = 0; i < latest_scan_->ranges.size(); ++i) {
    double range = latest_scan_->ranges[i];
    if (range > 0.0 && range < emergency_stop_distance_) {
      emergency_stop = true;
      break;
    }
  }
  
  if (emergency_stop) {
    cmd_vel.linear.x = 0.0;
    cmd_vel.angular.z = 0.0;
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                         "Emergency stop - obstacle too close!");
  }
  
  // Publish command
  cmd_vel_pub_->publish(cmd_vel);
}

std::vector<WallSegment> WallFollower::detectWalls(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  std::vector<WallSegment> walls;
  
  if (!scan || scan->ranges.empty()) {
    return walls;
  }
  
  std::vector<std::pair<double, double>> valid_points;
  
  // Extract valid points within detection range
  for (size_t i = 0; i < scan->ranges.size(); ++i) {
    double range = scan->ranges[i];
    double angle = scan->angle_min + i * scan->angle_increment;
    
    if (range > scan->range_min && range < std::min(static_cast<double>(scan->range_max), max_wall_detection_range_)) {
      valid_points.emplace_back(angle, range);
    }
  }
  
  if (valid_points.size() < min_wall_points_) {
    return walls;
  }
  
  // Group consecutive points into wall segments
  std::vector<std::vector<std::pair<double, double>>> segments;
  std::vector<std::pair<double, double>> current_segment;
  
  for (size_t i = 0; i < valid_points.size(); ++i) {
    if (current_segment.empty()) {
      current_segment.push_back(valid_points[i]);
    } else {
      // Check if point is close enough to previous point
      auto prev_point = current_segment.back();
      double dx = valid_points[i].second * cos(valid_points[i].first) - 
                  prev_point.second * cos(prev_point.first);
      double dy = valid_points[i].second * sin(valid_points[i].first) - 
                  prev_point.second * sin(prev_point.first);
      double distance = sqrt(dx * dx + dy * dy);
      
      if (distance < max_point_gap_) {
        current_segment.push_back(valid_points[i]);
      } else {
        if (current_segment.size() >= min_wall_points_) {
          segments.push_back(current_segment);
        }
        current_segment.clear();
        current_segment.push_back(valid_points[i]);
      }
    }
  }
  
  // Add last segment
  if (current_segment.size() >= min_wall_points_) {
    segments.push_back(current_segment);
  }
  
  // Convert segments to wall objects
  for (const auto & segment : segments) {
    if (segment.size() < min_wall_points_) continue;
    
    WallSegment wall;
    wall.points = segment;
    wall.start_angle = segment.front().first;
    wall.end_angle = segment.back().first;
    
    // Calculate average distance and length
    double total_distance = 0.0;
    double total_length = 0.0;
    
    for (size_t i = 0; i < segment.size(); ++i) {
      total_distance += segment[i].second;
      
      if (i > 0) {
        double dx = segment[i].second * cos(segment[i].first) - 
                    segment[i-1].second * cos(segment[i-1].first);
        double dy = segment[i].second * sin(segment[i].first) - 
                    segment[i-1].second * sin(segment[i-1].first);
        total_length += sqrt(dx * dx + dy * dy);
      }
    }
    
    wall.distance = total_distance / segment.size();
    wall.length = total_length;
    
    // Calculate confidence based on length and consistency
    wall.confidence = std::min(1.0, wall.length / min_wall_length_);
    
    if (wall.confidence >= wall_confidence_threshold_ && wall.length >= min_wall_length_) {
      walls.push_back(wall);
    }
  }
  
  return walls;
}

WallSegment WallFollower::findBestWallToFollow(const std::vector<WallSegment> & walls)
{
  if (walls.empty()) {
    return WallSegment{};
  }

  // Find wall closest to target distance
  WallSegment best_wall = walls[0];
  double best_score = std::abs(best_wall.distance - target_wall_distance_);

  for (const auto & wall : walls) {
    // Score based on distance to target and confidence
    double distance_score = std::abs(wall.distance - target_wall_distance_);
    double confidence_score = wall.confidence;
    double combined_score = distance_score - 0.5 * confidence_score;

    if (combined_score < best_score) {
      best_score = combined_score;
      best_wall = wall;
    }
  }

  return best_wall;
}

geometry_msgs::msg::Twist WallFollower::calculateWallFollowingCommand(const WallSegment & wall)
{
  geometry_msgs::msg::Twist cmd_vel;

  if (wall.points.empty()) {
    return cmd_vel;
  }

  // Calculate distance error
  double distance_error = wall.distance - target_wall_distance_;

  // Calculate wall angle (direction of wall relative to robot)
  double wall_angle = calculateWallAngle(wall);

  // PID control for maintaining distance and parallel orientation
  double angular_velocity = -distance_gain_ * distance_error - angular_gain_ * wall_angle;

  // Limit angular velocity
  angular_velocity = std::max(-max_angular_velocity_,
                             std::min(max_angular_velocity_, angular_velocity));

  // Set forward velocity based on how well aligned we are
  double alignment_factor = 1.0 - std::abs(wall_angle) / M_PI;
  double linear_velocity = wall_following_speed_ * alignment_factor;

  // Reduce speed if too close to wall
  if (wall.distance < target_wall_distance_ * 0.8) {
    linear_velocity *= 0.5;
  }

  cmd_vel.linear.x = std::max(0.0, std::min(max_linear_velocity_, linear_velocity));
  cmd_vel.angular.z = angular_velocity;

  return cmd_vel;
}

geometry_msgs::msg::Twist WallFollower::calculateApproachCommand(const WallSegment & wall)
{
  geometry_msgs::msg::Twist cmd_vel;

  if (wall.points.empty()) {
    return cmd_vel;
  }

  // Calculate angle to wall center
  double wall_center_angle = (wall.start_angle + wall.end_angle) / 2.0;

  // Turn towards wall if not facing it
  if (std::abs(wall_center_angle) > 0.1) {
    cmd_vel.angular.z = angular_gain_ * wall_center_angle;
    cmd_vel.angular.z = std::max(-max_angular_velocity_,
                                std::min(max_angular_velocity_, cmd_vel.angular.z));
  } else {
    // Move towards wall
    cmd_vel.linear.x = approach_speed_;

    // Switch to wall following when close enough
    if (wall.distance <= target_wall_distance_ * 1.2) {
      current_state_ = WallFollowingState::FOLLOWING_WALL_LEFT;
    }
  }

  return cmd_vel;
}

geometry_msgs::msg::Twist WallFollower::calculateSearchCommand()
{
  geometry_msgs::msg::Twist cmd_vel;

  // Rotate slowly to search for walls
  cmd_vel.linear.x = search_speed_;
  cmd_vel.angular.z = 0.3;  // Slow rotation while moving forward

  return cmd_vel;
}

double WallFollower::calculateWallAngle(const WallSegment & wall)
{
  if (wall.points.size() < 2) {
    return 0.0;
  }

  // Calculate wall direction using first and last points
  auto first_point = wall.points.front();
  auto last_point = wall.points.back();

  double x1 = first_point.second * cos(first_point.first);
  double y1 = first_point.second * sin(first_point.first);
  double x2 = last_point.second * cos(last_point.first);
  double y2 = last_point.second * sin(last_point.first);

  // Wall direction vector
  double wall_dx = x2 - x1;
  double wall_dy = y2 - y1;

  // Wall angle relative to robot's forward direction
  double wall_angle = atan2(wall_dy, wall_dx);

  // Normalize to [-pi, pi]
  while (wall_angle > M_PI) wall_angle -= 2 * M_PI;
  while (wall_angle < -M_PI) wall_angle += 2 * M_PI;

  return wall_angle;
}

void WallFollower::updateState(const std::vector<WallSegment> & walls)
{
  switch (current_state_) {
    case WallFollowingState::SEARCHING_FOR_WALL:
      if (!walls.empty()) {
        auto best_wall = findBestWallToFollow(walls);
        if (best_wall.distance > target_wall_distance_ * 1.5) {
          current_state_ = WallFollowingState::APPROACHING_WALL;
        } else {
          current_state_ = WallFollowingState::FOLLOWING_WALL_LEFT;
        }
      }
      break;

    case WallFollowingState::APPROACHING_WALL:
      if (!walls.empty()) {
        auto best_wall = findBestWallToFollow(walls);
        if (best_wall.distance <= target_wall_distance_ * 1.2) {
          current_state_ = WallFollowingState::FOLLOWING_WALL_LEFT;
        }
      } else {
        current_state_ = WallFollowingState::SEARCHING_FOR_WALL;
      }
      break;

    case WallFollowingState::FOLLOWING_WALL_LEFT:
    case WallFollowingState::FOLLOWING_WALL_RIGHT:
      if (walls.empty()) {
        auto time_since_wall = (this->now() - last_wall_seen_).seconds();
        if (time_since_wall > wall_lost_timeout_) {
          current_state_ = WallFollowingState::LOST_WALL;
        }
      } else {
        last_wall_seen_ = this->now();
      }
      break;

    default:
      break;
  }
}

void WallFollower::publishVisualization(const std::vector<WallSegment> & walls)
{
  visualization_msgs::msg::MarkerArray marker_array;

  // Clear previous markers
  visualization_msgs::msg::Marker clear_marker;
  clear_marker.header.frame_id = laser_frame_;
  clear_marker.header.stamp = this->now();
  clear_marker.action = visualization_msgs::msg::Marker::DELETEALL;
  marker_array.markers.push_back(clear_marker);

  // Add wall markers
  for (size_t i = 0; i < walls.size(); ++i) {
    visualization_msgs::msg::Marker marker;
    marker.header.frame_id = laser_frame_;
    marker.header.stamp = this->now();
    marker.ns = "walls";
    marker.id = i;
    marker.type = visualization_msgs::msg::Marker::LINE_STRIP;
    marker.action = visualization_msgs::msg::Marker::ADD;

    marker.scale.x = 0.02;  // Line width
    marker.color.r = 1.0;
    marker.color.g = 0.0;
    marker.color.b = 0.0;
    marker.color.a = walls[i].confidence;

    // Add points
    for (const auto & point : walls[i].points) {
      geometry_msgs::msg::Point p;
      p.x = point.second * cos(point.first);
      p.y = point.second * sin(point.first);
      p.z = 0.0;
      marker.points.push_back(p);
    }

    marker_array.markers.push_back(marker);
  }

  wall_markers_pub_->publish(marker_array);
}

}  // namespace turtle_bot_navigation

// Component registration removed for standalone executable
