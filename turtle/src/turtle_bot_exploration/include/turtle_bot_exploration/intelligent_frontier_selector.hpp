#ifndef TURTLE_BOT_EXPLORATION__INTELLIGENT_FRONTIER_SELECTOR_HPP_
#define TURTLE_BOT_EXPLORATION__INTELLIGENT_FRONTIER_SELECTOR_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

namespace turtle_bot_exploration
{

struct FrontierPoint
{
  geometry_msgs::msg::Point position;
  double size;                    // Size of the frontier cluster
  double distance_from_robot;     // Distance from current robot position
  double exploration_value;       // Potential information gain
  double accessibility_score;     // How easy it is to reach
  double time_efficiency_score;   // Time-based efficiency metric
  double combined_score;          // Final selection score
  int cluster_id;                 // Frontier cluster identifier
  bool is_visited;               // Whether this frontier was already explored
  rclcpp::Time last_updated;     // When this frontier was last seen
  std::vector<geometry_msgs::msg::Point> nearby_frontiers;  // Adjacent frontiers
};

enum class SelectionStrategy
{
  NEAREST_FIRST,           // Go to closest frontier
  LARGEST_FIRST,           // Go to largest frontier
  HIGHEST_VALUE,           // Go to highest information gain
  TIME_EFFICIENT,          // Optimize for time efficiency
  INTELLIGENT_COMBINED     // Smart combination of all factors
};

class IntelligentFrontierSelector : public rclcpp::Node
{
public:
  explicit IntelligentFrontierSelector(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~IntelligentFrontierSelector() = default;

private:
  // Callback functions
  void frontiersCallback(const geometry_msgs::msg::PointStamped::SharedPtr msg);
  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg);
  void selectionTimerCallback();
  
  // Intelligent selection algorithms
  FrontierPoint selectBestFrontier(const std::vector<FrontierPoint> & frontiers);
  std::vector<FrontierPoint> clusterFrontiers(const std::vector<FrontierPoint> & frontiers);
  double calculateExplorationValue(const FrontierPoint & frontier);
  double calculateAccessibilityScore(const FrontierPoint & frontier);
  double calculateTimeEfficiencyScore(const FrontierPoint & frontier);
  double calculateCombinedScore(const FrontierPoint & frontier);
  
  // Multi-frontier path planning
  std::vector<FrontierPoint> planOptimalSequence(const std::vector<FrontierPoint> & frontiers);
  double calculateTravelTime(const geometry_msgs::msg::Point & from, const geometry_msgs::msg::Point & to);
  bool isPathClear(const geometry_msgs::msg::Point & from, const geometry_msgs::msg::Point & to);
  
  // Frontier management
  void updateFrontierDatabase(const std::vector<FrontierPoint> & new_frontiers);
  void markFrontierAsVisited(const geometry_msgs::msg::Point & position);
  void cleanupOldFrontiers();
  std::vector<FrontierPoint> getActiveFrontiers();
  
  // Utility functions
  double calculateDistance(const geometry_msgs::msg::Point & p1, const geometry_msgs::msg::Point & p2);
  double calculateAngleDifference(const geometry_msgs::msg::Point & target);
  bool isInExploredArea(const geometry_msgs::msg::Point & point);
  geometry_msgs::msg::Point getCurrentPosition();
  
  // Visualization
  void publishFrontierVisualization(const std::vector<FrontierPoint> & frontiers);
  void publishSelectedFrontier(const FrontierPoint & selected);
  void publishExplorationPath(const std::vector<FrontierPoint> & path);
  
  // ROS2 interfaces
  rclcpp::Subscription<geometry_msgs::msg::PointStamped>::SharedPtr frontiers_sub_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr selected_frontier_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr frontier_markers_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr path_markers_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr strategy_status_pub_;
  
  rclcpp::TimerBase::SharedPtr selection_timer_;
  
  // TF2
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  
  // State variables
  std::vector<FrontierPoint> frontier_database_;
  std::vector<FrontierPoint> planned_sequence_;
  nav_msgs::msg::OccupancyGrid::SharedPtr current_map_;
  geometry_msgs::msg::Pose current_pose_;
  FrontierPoint current_target_;
  
  // Parameters
  SelectionStrategy selection_strategy_;
  double selection_frequency_;
  double frontier_timeout_;
  double visited_threshold_;
  double clustering_distance_;
  double min_frontier_size_;
  double max_travel_distance_;
  
  // Scoring weights
  double distance_weight_;
  double size_weight_;
  double value_weight_;
  double accessibility_weight_;
  double time_weight_;
  
  // Performance tracking
  rclcpp::Time exploration_start_time_;
  double total_distance_traveled_;
  int frontiers_explored_;
  std::unordered_set<int> visited_clusters_;
  
  // Frame IDs
  std::string global_frame_;
  std::string robot_frame_;
  
  // Advanced features
  bool enable_multi_frontier_planning_;
  bool enable_predictive_selection_;
  bool enable_dynamic_strategy_;
  int lookahead_frontiers_;
  double efficiency_threshold_;
};

}  // namespace turtle_bot_exploration

#endif  // TURTLE_BOT_EXPLORATION__INTELLIGENT_FRONTIER_SELECTOR_HPP_
