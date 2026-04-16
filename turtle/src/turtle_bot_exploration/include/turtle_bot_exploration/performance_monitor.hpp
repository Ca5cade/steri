#ifndef TURTLE_BOT_EXPLORATION__PERFORMANCE_MONITOR_HPP_
#define TURTLE_BOT_EXPLORATION__PERFORMANCE_MONITOR_HPP_

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <vector>
#include <chrono>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace turtle_bot_exploration
{

struct VelocityData
{
  double linear_velocity;
  double angular_velocity;
  std::chrono::steady_clock::time_point timestamp;
};

class PerformanceMonitor : public rclcpp::Node
{
public:
  explicit PerformanceMonitor(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~PerformanceMonitor() = default;

private:
  // Callback functions
  void explorationStatusCallback(const std_msgs::msg::String::SharedPtr msg);
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg);
  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg);
  void monitoringCallback();
  
  // Core monitoring functions
  void startMonitoring();
  void stopMonitoring();
  void recordGoalCompletion(bool success);
  void updatePerformanceMetrics();
  double calculateMovementEfficiency();
  void publishPerformanceMetrics();
  std::string formatPerformanceMetrics();
  void performAdaptiveTuning();
  
  // ROS2 components
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr exploration_status_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr performance_metrics_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr efficiency_score_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr adaptive_params_pub_;
  
  rclcpp::TimerBase::SharedPtr monitoring_timer_;
  
  // Parameters
  double monitoring_frequency_;
  int performance_window_size_;
  double efficiency_threshold_;
  bool enable_adaptive_tuning_;
  bool publish_detailed_metrics_;
  
  // Monitoring state
  bool monitoring_active_;
  std::chrono::steady_clock::time_point monitoring_start_time_;
  std::chrono::steady_clock::time_point last_update_time_;
  std::chrono::steady_clock::time_point last_goal_start_time_;
  
  // Performance metrics
  double total_exploration_time_;
  double total_distance_traveled_;
  int goals_completed_;
  int goals_failed_;
  double average_goal_completion_time_;
  double current_efficiency_score_;
  std::vector<double> goal_completion_times_;
  
  // Velocity tracking
  double current_linear_velocity_;
  double current_angular_velocity_;
  std::vector<VelocityData> velocity_history_;
  
  // Odometry tracking
  nav_msgs::msg::Odometry::SharedPtr last_odom_msg_;
  std::chrono::steady_clock::time_point last_odom_time_;
};

}  // namespace turtle_bot_exploration

#endif  // TURTLE_BOT_EXPLORATION__PERFORMANCE_MONITOR_HPP_
