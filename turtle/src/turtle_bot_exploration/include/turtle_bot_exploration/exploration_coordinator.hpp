#ifndef TURTLE_BOT_EXPLORATION__EXPLORATION_COORDINATOR_HPP_
#define TURTLE_BOT_EXPLORATION__EXPLORATION_COORDINATOR_HPP_

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/string.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace turtle_bot_exploration
{

enum class ExplorationState
{
  IDLE,
  DETECTING_ENVIRONMENT,
  EXPLORING,
  NAVIGATING_TO_FRONTIER,
  STUCK_RECOVERY,
  EXPLORATION_COMPLETE,
  ERROR
};

struct ExplorationGoal
{
  geometry_msgs::msg::PoseStamped pose;
  double information_gain;
  double distance;
  int frontier_id;
  std::chrono::steady_clock::time_point created_time;
};

struct PerformanceMetrics
{
  int goals_completed;
  int goals_failed;
  double average_completion_time;
  double total_exploration_time;
  double efficiency_score;
  std::chrono::steady_clock::time_point last_update_time;
};

class ExplorationCoordinator : public rclcpp::Node
{
public:
  explicit ExplorationCoordinator(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  // Callback functions
  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void frontierMarkersCallback(const visualization_msgs::msg::MarkerArray::SharedPtr msg);
  void timerCallback();
  void goalResponseCallback(
    const rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr & goal_handle);
  void feedbackCallback(
    rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr,
    const std::shared_ptr<const nav2_msgs::action::NavigateToPose::Feedback> feedback);
  void resultCallback(
    const rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::WrappedResult & result);

  // Core exploration functions
  void updateExplorationState();
  bool isInUnmappedEnvironment();
  bool shouldStartExploration();
  void selectAndNavigateToFrontier();
  ExplorationGoal selectBestFrontier();
  void sendNavigationGoal(const ExplorationGoal & goal);
  void handleStuckRecovery();
  void checkExplorationCompletion();
  
  // Utility functions
  geometry_msgs::msg::PoseStamped getRobotPose();
  double calculateMapCoverage();
  bool isRobotStuck();
  bool isFrontierSafe(const ExplorationGoal & frontier);
  double getDistanceToNearestObstacle(double x, double y);
  void publishExplorationStatus();
  void publishExplorationMarkers();
  bool isGoalReached();  // Enhanced goal completion detection
  bool shouldForceNewGoal();  // Check if we should force a new goal
  void handleStuckAtGoal();  // Handle stuck at goal situation
  double calculateDistance(const geometry_msgs::msg::Point & p1, const geometry_msgs::msg::Point & p2);

  // Enhanced optimization methods
  double calculateDynamicTolerance(const geometry_msgs::msg::PoseStamped & current_pose);
  bool evaluateSmoothCompletion(double distance, double tolerance);
  void updatePerformanceMetrics(bool goal_completed, double final_distance, double tolerance);
  void adaptParametersBasedOnPerformance();
  void publishPerformanceMetrics();
  void optimizeTransitionSmoothing();
  bool validateExplorationProgress(const geometry_msgs::msg::PoseStamped & current_pose);

  // Enhanced frontier selection methods
  double calculateCurrentVelocity();
  double calculateDistanceScore(const ExplorationGoal & frontier, const geometry_msgs::msg::PoseStamped & robot_pose);
  double calculateInformationScore(const ExplorationGoal & frontier, double map_coverage);
  double calculateAccessibilityScore(const ExplorationGoal & frontier, const geometry_msgs::msg::PoseStamped & robot_pose);
  double calculateEfficiencyScore(const ExplorationGoal & frontier, double current_velocity);
  double calculateStrategicScore(const ExplorationGoal & frontier, const geometry_msgs::msg::PoseStamped & robot_pose);
  double applyDynamicWeighting(double base_score, const ExplorationGoal & frontier, double map_coverage);

  // Enhanced error recovery methods
  void performIntelligentBacking();
  void performAdaptiveRotation();
  void performStrategicForwardMovement();
  bool performAlternativeGoalSelection();
  void performEmergencyUnstuckManeuvers();

  // State management
  void setState(ExplorationState new_state);
  std::string stateToString(ExplorationState state);

  // ROS2 components
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  rclcpp::Subscription<visualization_msgs::msg::MarkerArray>::SharedPtr frontier_markers_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr exploration_markers_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  // Action client for navigation
  rclcpp_action::Client<nav2_msgs::action::NavigateToPose>::SharedPtr nav_action_client_;
  rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr current_goal_handle_;

  // TF2 components
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  // Parameters
  double exploration_timeout_;
  double stuck_timeout_;
  double min_map_coverage_to_start_;
  double goal_tolerance_;
  double stuck_distance_threshold_;
  double goal_reached_tolerance_;
  double goal_timeout_;
  double force_new_goal_distance_;
  double recovery_rotation_speed_;
  double recovery_rotation_duration_;
  std::string robot_base_frame_;
  std::string global_frame_;
  bool auto_start_exploration_;

  // Enhanced optimization parameters
  bool enable_dynamic_tolerance_;
  bool velocity_based_tolerance_;
  double min_dynamic_tolerance_;
  double max_dynamic_tolerance_;
  double velocity_tolerance_factor_;
  bool environmental_adaptation_;
  bool performance_monitoring_;
  bool adaptive_parameter_tuning_;
  bool goal_completion_smoothing_;
  bool transition_optimization_;

  // State variables
  ExplorationState current_state_;
  nav_msgs::msg::OccupancyGrid::SharedPtr current_map_;
  std::vector<ExplorationGoal> available_frontiers_;
  ExplorationGoal current_goal_;
  geometry_msgs::msg::PoseStamped last_robot_pose_;
  std::chrono::steady_clock::time_point last_movement_time_;
  std::chrono::steady_clock::time_point state_start_time_;
  std::chrono::steady_clock::time_point last_frontier_update_;
  bool map_received_;
  bool frontiers_received_;
  bool exploration_active_;
  bool goal_active_;
  bool goal_completion_override_;  // Force goal completion
  std::chrono::steady_clock::time_point goal_start_time_;  // When current goal was set
  geometry_msgs::msg::PoseStamped goal_set_pose_;  // Robot pose when goal was set
  double total_distance_traveled_;
  int exploration_cycles_;

  // Enhanced optimization state variables
  PerformanceMetrics performance_metrics_;
  std::chrono::steady_clock::time_point last_pose_update_time_;
  double last_distance_to_goal_;
};

}  // namespace turtle_bot_exploration

#endif  // TURTLE_BOT_EXPLORATION__EXPLORATION_COORDINATOR_HPP_
