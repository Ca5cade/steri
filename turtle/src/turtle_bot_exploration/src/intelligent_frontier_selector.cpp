#include "turtle_bot_exploration/intelligent_frontier_selector.hpp"
#include <cmath>
#include <algorithm>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/utils.h>

namespace turtle_bot_exploration
{

IntelligentFrontierSelector::IntelligentFrontierSelector(const rclcpp::NodeOptions & options)
: Node("intelligent_frontier_selector", options),
  selection_strategy_(SelectionStrategy::INTELLIGENT_COMBINED),
  total_distance_traveled_(0.0),
  frontiers_explored_(0)
{
  // Declare parameters
  this->declare_parameter("selection_frequency", 2.0);
  this->declare_parameter("frontier_timeout", 30.0);
  this->declare_parameter("visited_threshold", 1.0);
  this->declare_parameter("clustering_distance", 2.0);
  this->declare_parameter("min_frontier_size", 10);
  this->declare_parameter("max_travel_distance", 15.0);
  
  // Scoring weights
  this->declare_parameter("distance_weight", 0.3);
  this->declare_parameter("size_weight", 0.2);
  this->declare_parameter("value_weight", 0.25);
  this->declare_parameter("accessibility_weight", 0.15);
  this->declare_parameter("time_weight", 0.1);
  
  // Advanced features
  this->declare_parameter("enable_multi_frontier_planning", true);
  this->declare_parameter("enable_predictive_selection", true);
  this->declare_parameter("enable_dynamic_strategy", true);
  this->declare_parameter("lookahead_frontiers", 3);
  this->declare_parameter("efficiency_threshold", 0.7);
  
  this->declare_parameter("global_frame", std::string("map"));
  this->declare_parameter("robot_frame", std::string("base_link"));

  // Get parameters
  selection_frequency_ = this->get_parameter("selection_frequency").as_double();
  frontier_timeout_ = this->get_parameter("frontier_timeout").as_double();
  visited_threshold_ = this->get_parameter("visited_threshold").as_double();
  clustering_distance_ = this->get_parameter("clustering_distance").as_double();
  min_frontier_size_ = this->get_parameter("min_frontier_size").as_int();
  max_travel_distance_ = this->get_parameter("max_travel_distance").as_double();
  
  distance_weight_ = this->get_parameter("distance_weight").as_double();
  size_weight_ = this->get_parameter("size_weight").as_double();
  value_weight_ = this->get_parameter("value_weight").as_double();
  accessibility_weight_ = this->get_parameter("accessibility_weight").as_double();
  time_weight_ = this->get_parameter("time_weight").as_double();
  
  enable_multi_frontier_planning_ = this->get_parameter("enable_multi_frontier_planning").as_bool();
  enable_predictive_selection_ = this->get_parameter("enable_predictive_selection").as_bool();
  enable_dynamic_strategy_ = this->get_parameter("enable_dynamic_strategy").as_bool();
  lookahead_frontiers_ = this->get_parameter("lookahead_frontiers").as_int();
  efficiency_threshold_ = this->get_parameter("efficiency_threshold").as_double();
  
  global_frame_ = this->get_parameter("global_frame").as_string();
  robot_frame_ = this->get_parameter("robot_frame").as_string();

  // Initialize TF2
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Create subscribers
  frontiers_sub_ = this->create_subscription<geometry_msgs::msg::PointStamped>(
    "/frontiers", 10, std::bind(&IntelligentFrontierSelector::frontiersCallback, this, std::placeholders::_1));
  
  map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
    "/map", 10, std::bind(&IntelligentFrontierSelector::mapCallback, this, std::placeholders::_1));
  
  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    "/odom", 10, std::bind(&IntelligentFrontierSelector::odomCallback, this, std::placeholders::_1));

  // Create publishers
  selected_frontier_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/selected_frontier", 10);
  frontier_markers_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("/frontier_markers", 10);
  path_markers_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("/exploration_path", 10);
  strategy_status_pub_ = this->create_publisher<std_msgs::msg::String>("/exploration_strategy", 10);

  // Create timer
  auto timer_period = std::chrono::milliseconds(static_cast<int>(1000.0 / selection_frequency_));
  selection_timer_ = this->create_wall_timer(
    timer_period, std::bind(&IntelligentFrontierSelector::selectionTimerCallback, this));

  // Initialize timestamps
  exploration_start_time_ = this->now();

  RCLCPP_INFO(this->get_logger(), 
              "Intelligent Frontier Selector initialized with strategy: INTELLIGENT_COMBINED");
}

void IntelligentFrontierSelector::frontiersCallback(const geometry_msgs::msg::PointStamped::SharedPtr msg)
{
  // Convert incoming frontier to FrontierPoint
  FrontierPoint frontier;
  frontier.position = msg->point;
  frontier.last_updated = this->now();
  frontier.is_visited = false;
  frontier.cluster_id = -1;  // Will be assigned during clustering
  
  // Calculate basic properties
  frontier.distance_from_robot = calculateDistance(frontier.position, getCurrentPosition());
  frontier.size = min_frontier_size_;  // Default size, will be updated by clustering
  
  // Add to temporary list for processing
  std::vector<FrontierPoint> temp_frontiers = {frontier};
  updateFrontierDatabase(temp_frontiers);
}

void IntelligentFrontierSelector::mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  current_map_ = msg;
}

void IntelligentFrontierSelector::odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
  current_pose_ = msg->pose.pose;
}

void IntelligentFrontierSelector::selectionTimerCallback()
{
  // Clean up old frontiers
  cleanupOldFrontiers();
  
  // Get active frontiers
  auto active_frontiers = getActiveFrontiers();
  
  if (active_frontiers.empty()) {
    return;
  }
  
  // Cluster frontiers for better organization
  auto clustered_frontiers = clusterFrontiers(active_frontiers);
  
  // Select the best frontier using intelligent algorithm
  auto best_frontier = selectBestFrontier(clustered_frontiers);
  
  // Plan optimal sequence if multi-frontier planning is enabled
  if (enable_multi_frontier_planning_ && clustered_frontiers.size() > 1) {
    planned_sequence_ = planOptimalSequence(clustered_frontiers);
    publishExplorationPath(planned_sequence_);
  }
  
  // Publish selected frontier
  publishSelectedFrontier(best_frontier);
  publishFrontierVisualization(clustered_frontiers);
  
  // Update current target
  current_target_ = best_frontier;
  
  // Publish strategy status
  std_msgs::msg::String strategy_msg;
  strategy_msg.data = "INTELLIGENT_COMBINED: Selected frontier at (" + 
                     std::to_string(best_frontier.position.x) + ", " + 
                     std::to_string(best_frontier.position.y) + ") with score " + 
                     std::to_string(best_frontier.combined_score);
  strategy_status_pub_->publish(strategy_msg);
}

FrontierPoint IntelligentFrontierSelector::selectBestFrontier(const std::vector<FrontierPoint> & frontiers)
{
  if (frontiers.empty()) {
    FrontierPoint empty_frontier;
    return empty_frontier;
  }
  
  std::vector<FrontierPoint> scored_frontiers = frontiers;
  
  // Calculate scores for each frontier
  for (auto & frontier : scored_frontiers) {
    frontier.exploration_value = calculateExplorationValue(frontier);
    frontier.accessibility_score = calculateAccessibilityScore(frontier);
    frontier.time_efficiency_score = calculateTimeEfficiencyScore(frontier);
    frontier.combined_score = calculateCombinedScore(frontier);
  }
  
  // Sort by combined score (highest first)
  std::sort(scored_frontiers.begin(), scored_frontiers.end(),
    [](const FrontierPoint & a, const FrontierPoint & b) {
      return a.combined_score > b.combined_score;
    });
  
  // Apply dynamic strategy adjustment if enabled
  if (enable_dynamic_strategy_) {
    // If we've been exploring for a while, prioritize efficiency
    auto exploration_duration = (this->now() - exploration_start_time_).seconds();
    if (exploration_duration > 300.0) {  // 5 minutes
      // Boost time efficiency weight
      for (auto & frontier : scored_frontiers) {
        frontier.combined_score = frontier.combined_score * 0.7 + frontier.time_efficiency_score * 0.3;
      }
      
      // Re-sort with adjusted scores
      std::sort(scored_frontiers.begin(), scored_frontiers.end(),
        [](const FrontierPoint & a, const FrontierPoint & b) {
          return a.combined_score > b.combined_score;
        });
    }
  }
  
  return scored_frontiers[0];
}

std::vector<FrontierPoint> IntelligentFrontierSelector::clusterFrontiers(
  const std::vector<FrontierPoint> & frontiers)
{
  std::vector<FrontierPoint> clustered = frontiers;
  int cluster_id = 0;
  
  for (size_t i = 0; i < clustered.size(); ++i) {
    if (clustered[i].cluster_id != -1) continue;  // Already clustered
    
    clustered[i].cluster_id = cluster_id;
    std::vector<geometry_msgs::msg::Point> cluster_points = {clustered[i].position};
    
    // Find nearby frontiers to cluster together
    for (size_t j = i + 1; j < clustered.size(); ++j) {
      if (clustered[j].cluster_id != -1) continue;
      
      double distance = calculateDistance(clustered[i].position, clustered[j].position);
      if (distance < clustering_distance_) {
        clustered[j].cluster_id = cluster_id;
        cluster_points.push_back(clustered[j].position);
      }
    }
    
    // Update cluster size for all frontiers in this cluster
    for (auto & frontier : clustered) {
      if (frontier.cluster_id == cluster_id) {
        frontier.size = cluster_points.size();
        frontier.nearby_frontiers = cluster_points;
      }
    }
    
    cluster_id++;
  }
  
  return clustered;
}

double IntelligentFrontierSelector::calculateExplorationValue(const FrontierPoint & frontier)
{
  if (!current_map_) {
    return 0.5;  // Default value
  }
  
  // Calculate potential information gain based on unexplored area around frontier
  double exploration_radius = 3.0;  // 3 meter radius
  int unexplored_cells = 0;
  int total_cells = 0;
  
  // Sample area around frontier
  for (double dx = -exploration_radius; dx <= exploration_radius; dx += current_map_->info.resolution) {
    for (double dy = -exploration_radius; dy <= exploration_radius; dy += current_map_->info.resolution) {
      double distance = sqrt(dx * dx + dy * dy);
      if (distance > exploration_radius) continue;
      
      // Convert to map coordinates
      int map_x = static_cast<int>((frontier.position.x + dx - current_map_->info.origin.position.x) / 
                                   current_map_->info.resolution);
      int map_y = static_cast<int>((frontier.position.y + dy - current_map_->info.origin.position.y) / 
                                   current_map_->info.resolution);
      
      if (map_x >= 0 && map_x < static_cast<int>(current_map_->info.width) &&
          map_y >= 0 && map_y < static_cast<int>(current_map_->info.height)) {
        int index = map_y * current_map_->info.width + map_x;
        total_cells++;
        
        if (current_map_->data[index] == -1) {  // Unexplored
          unexplored_cells++;
        }
      }
    }
  }
  
  return total_cells > 0 ? static_cast<double>(unexplored_cells) / total_cells : 0.0;
}

double IntelligentFrontierSelector::calculateAccessibilityScore(const FrontierPoint & frontier)
{
  // Check if path to frontier is clear
  auto current_pos = getCurrentPosition();
  bool path_clear = isPathClear(current_pos, frontier.position);
  
  // Base accessibility score
  double score = path_clear ? 1.0 : 0.3;
  
  // Adjust based on distance (closer is more accessible)
  double distance_factor = std::max(0.1, 1.0 - (frontier.distance_from_robot / max_travel_distance_));
  score *= distance_factor;
  
  // Adjust based on angle difference (less turning is more accessible)
  double angle_diff = calculateAngleDifference(frontier.position);
  double angle_factor = std::max(0.5, 1.0 - (angle_diff / M_PI));
  score *= angle_factor;
  
  return score;
}

double IntelligentFrontierSelector::calculateTimeEfficiencyScore(const FrontierPoint & frontier)
{
  // Estimate time to reach this frontier
  double travel_time = calculateTravelTime(getCurrentPosition(), frontier.position);
  
  // Estimate exploration value per unit time
  double value_per_time = frontier.exploration_value / std::max(1.0, travel_time);
  
  // Consider cluster size (larger clusters provide more exploration per visit)
  double cluster_bonus = std::min(2.0, frontier.size / 5.0);
  
  // Consider if this frontier leads to other unexplored areas
  double connectivity_bonus = frontier.nearby_frontiers.size() * 0.1;
  
  return value_per_time * cluster_bonus * (1.0 + connectivity_bonus);
}

double IntelligentFrontierSelector::calculateCombinedScore(const FrontierPoint & frontier)
{
  // Normalize distance score (closer is better)
  double distance_score = std::max(0.0, 1.0 - (frontier.distance_from_robot / max_travel_distance_));
  
  // Normalize size score
  double size_score = std::min(1.0, frontier.size / 20.0);
  
  // Combine all scores with weights
  double combined = distance_weight_ * distance_score +
                   size_weight_ * size_score +
                   value_weight_ * frontier.exploration_value +
                   accessibility_weight_ * frontier.accessibility_score +
                   time_weight_ * frontier.time_efficiency_score;
  
  return combined;
}

std::vector<FrontierPoint> IntelligentFrontierSelector::planOptimalSequence(
  const std::vector<FrontierPoint> & frontiers)
{
  if (frontiers.size() <= 1) {
    return frontiers;
  }
  
  // Simple greedy approach for now - can be enhanced with TSP algorithms
  std::vector<FrontierPoint> sequence;
  std::vector<bool> visited(frontiers.size(), false);
  auto current_pos = getCurrentPosition();
  
  for (int step = 0; step < lookahead_frontiers_ && step < static_cast<int>(frontiers.size()); ++step) {
    int best_idx = -1;
    double best_score = -1.0;
    
    for (size_t i = 0; i < frontiers.size(); ++i) {
      if (visited[i]) continue;
      
      double travel_time = calculateTravelTime(current_pos, frontiers[i].position);
      double efficiency = frontiers[i].exploration_value / std::max(1.0, travel_time);
      
      if (efficiency > best_score) {
        best_score = efficiency;
        best_idx = i;
      }
    }
    
    if (best_idx != -1) {
      sequence.push_back(frontiers[best_idx]);
      visited[best_idx] = true;
      current_pos = frontiers[best_idx].position;
    }
  }
  
  return sequence;
}

double IntelligentFrontierSelector::calculateTravelTime(
  const geometry_msgs::msg::Point & from, const geometry_msgs::msg::Point & to)
{
  double distance = calculateDistance(from, to);
  double average_speed = 0.3;  // m/s
  return distance / average_speed;
}

bool IntelligentFrontierSelector::isPathClear(
  const geometry_msgs::msg::Point & from, const geometry_msgs::msg::Point & to)
{
  if (!current_map_) {
    return true;  // Assume clear if no map
  }
  
  // Simple line-of-sight check
  double dx = to.x - from.x;
  double dy = to.y - from.y;
  double distance = sqrt(dx * dx + dy * dy);
  int steps = static_cast<int>(distance / current_map_->info.resolution);
  
  for (int i = 0; i <= steps; ++i) {
    double t = static_cast<double>(i) / steps;
    double x = from.x + t * dx;
    double y = from.y + t * dy;
    
    // Convert to map coordinates
    int map_x = static_cast<int>((x - current_map_->info.origin.position.x) / current_map_->info.resolution);
    int map_y = static_cast<int>((y - current_map_->info.origin.position.y) / current_map_->info.resolution);
    
    if (map_x >= 0 && map_x < static_cast<int>(current_map_->info.width) &&
        map_y >= 0 && map_y < static_cast<int>(current_map_->info.height)) {
      int index = map_y * current_map_->info.width + map_x;
      
      if (current_map_->data[index] > 50) {  // Obstacle
        return false;
      }
    }
  }
  
  return true;
}

void IntelligentFrontierSelector::updateFrontierDatabase(const std::vector<FrontierPoint> & new_frontiers)
{
  for (const auto & new_frontier : new_frontiers) {
    // Check if this frontier already exists (within threshold distance)
    bool exists = false;
    for (auto & existing : frontier_database_) {
      if (calculateDistance(existing.position, new_frontier.position) < visited_threshold_) {
        existing.last_updated = this->now();
        exists = true;
        break;
      }
    }
    
    if (!exists) {
      frontier_database_.push_back(new_frontier);
    }
  }
}

void IntelligentFrontierSelector::markFrontierAsVisited(const geometry_msgs::msg::Point & position)
{
  for (auto & frontier : frontier_database_) {
    if (calculateDistance(frontier.position, position) < visited_threshold_) {
      frontier.is_visited = true;
      visited_clusters_.insert(frontier.cluster_id);
      frontiers_explored_++;
    }
  }
}

void IntelligentFrontierSelector::cleanupOldFrontiers()
{
  auto current_time = this->now();
  frontier_database_.erase(
    std::remove_if(frontier_database_.begin(), frontier_database_.end(),
      [this, current_time](const FrontierPoint & frontier) {
        return (current_time - frontier.last_updated).seconds() > frontier_timeout_;
      }),
    frontier_database_.end());
}

std::vector<FrontierPoint> IntelligentFrontierSelector::getActiveFrontiers()
{
  std::vector<FrontierPoint> active;
  for (const auto & frontier : frontier_database_) {
    if (!frontier.is_visited) {
      active.push_back(frontier);
    }
  }
  return active;
}

double IntelligentFrontierSelector::calculateDistance(
  const geometry_msgs::msg::Point & p1, const geometry_msgs::msg::Point & p2)
{
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  return sqrt(dx * dx + dy * dy);
}

double IntelligentFrontierSelector::calculateAngleDifference(const geometry_msgs::msg::Point & target)
{
  auto current_pos = getCurrentPosition();
  double dx = target.x - current_pos.x;
  double dy = target.y - current_pos.y;
  double target_angle = atan2(dy, dx);
  
  // Get current robot orientation
  tf2::Quaternion q;
  tf2::fromMsg(current_pose_.orientation, q);
  double current_angle = tf2::getYaw(q);
  
  double angle_diff = std::abs(target_angle - current_angle);
  if (angle_diff > M_PI) {
    angle_diff = 2 * M_PI - angle_diff;
  }
  
  return angle_diff;
}

geometry_msgs::msg::Point IntelligentFrontierSelector::getCurrentPosition()
{
  geometry_msgs::msg::Point pos;
  pos.x = current_pose_.position.x;
  pos.y = current_pose_.position.y;
  pos.z = current_pose_.position.z;
  return pos;
}

void IntelligentFrontierSelector::publishSelectedFrontier(const FrontierPoint & selected)
{
  geometry_msgs::msg::PoseStamped pose_msg;
  pose_msg.header.stamp = this->now();
  pose_msg.header.frame_id = global_frame_;
  pose_msg.pose.position = selected.position;
  pose_msg.pose.orientation.w = 1.0;  // Default orientation
  
  selected_frontier_pub_->publish(pose_msg);
}

void IntelligentFrontierSelector::publishFrontierVisualization(const std::vector<FrontierPoint> & frontiers)
{
  visualization_msgs::msg::MarkerArray marker_array;
  
  // Clear previous markers
  visualization_msgs::msg::Marker clear_marker;
  clear_marker.header.frame_id = global_frame_;
  clear_marker.header.stamp = this->now();
  clear_marker.action = visualization_msgs::msg::Marker::DELETEALL;
  marker_array.markers.push_back(clear_marker);
  
  // Add frontier markers
  for (size_t i = 0; i < frontiers.size(); ++i) {
    visualization_msgs::msg::Marker marker;
    marker.header.frame_id = global_frame_;
    marker.header.stamp = this->now();
    marker.ns = "intelligent_frontiers";
    marker.id = i;
    marker.type = visualization_msgs::msg::Marker::ARROW;
    marker.action = visualization_msgs::msg::Marker::ADD;
    
    marker.pose.position = frontiers[i].position;
    marker.pose.orientation.w = 1.0;
    
    marker.scale.x = 0.5;
    marker.scale.y = 0.1;
    marker.scale.z = 0.1;
    
    // Color based on score (green = high score, red = low score)
    marker.color.r = 1.0 - frontiers[i].combined_score;
    marker.color.g = frontiers[i].combined_score;
    marker.color.b = 0.0;
    marker.color.a = 0.8;
    
    marker_array.markers.push_back(marker);
  }
  
  frontier_markers_pub_->publish(marker_array);
}

void IntelligentFrontierSelector::publishExplorationPath(const std::vector<FrontierPoint> & path)
{
  visualization_msgs::msg::MarkerArray marker_array;
  
  // Path line
  visualization_msgs::msg::Marker line_marker;
  line_marker.header.frame_id = global_frame_;
  line_marker.header.stamp = this->now();
  line_marker.ns = "exploration_path";
  line_marker.id = 0;
  line_marker.type = visualization_msgs::msg::Marker::LINE_STRIP;
  line_marker.action = visualization_msgs::msg::Marker::ADD;
  
  line_marker.scale.x = 0.05;
  line_marker.color.r = 0.0;
  line_marker.color.g = 0.0;
  line_marker.color.b = 1.0;
  line_marker.color.a = 0.7;
  
  // Add current position as start
  geometry_msgs::msg::Point start_point = getCurrentPosition();
  line_marker.points.push_back(start_point);
  
  // Add path points
  for (const auto & frontier : path) {
    line_marker.points.push_back(frontier.position);
  }
  
  marker_array.markers.push_back(line_marker);
  path_markers_pub_->publish(marker_array);
}

}  // namespace turtle_bot_exploration
