#ifndef TURTLE_BOT_NAVIGATION__EMERGENCY_STOP_CONTROLLER_HPP_
#define TURTLE_BOT_NAVIGATION__EMERGENCY_STOP_CONTROLLER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

#include <vector>
#include <memory>
#include <string>
#include <mutex>

namespace turtle_bot_navigation
{

enum class SafetyState
{
  SAFE,
  WARNING,
  EMERGENCY_STOP,
  COLLISION_DETECTED,
  RECOVERY_NEEDED
};

class EmergencyStopController : public rclcpp::Node
{
public:
  explicit EmergencyStopController(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~EmergencyStopController() = default;

private:
  // Callback functions
  void laserScanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg);
  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg);
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg);
  void safetyTimerCallback();
  void recoveryTimerCallback();
  
  // Safety analysis functions
  SafetyState analyzeSafetyState(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  bool isCollisionImminent(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  bool isInInflationZone(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  bool isStuckAgainstWall(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  
  // Safety response functions
  void executeEmergencyStop();
  void executeRecoveryBehavior();
  void publishSafeVelocity(const geometry_msgs::msg::Twist & original_cmd);
  void publishSafetyStatus();
  
  // Utility functions
  double getMinDistanceInSector(
    const sensor_msgs::msg::LaserScan::SharedPtr scan,
    double start_angle, double end_angle);
  bool isMovingTowardsObstacle(
    const geometry_msgs::msg::Twist & cmd,
    const sensor_msgs::msg::LaserScan::SharedPtr scan);
  
  // ROS2 interfaces
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr laser_sub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr safe_cmd_vel_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr emergency_stop_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr safety_status_pub_;
  
  rclcpp::TimerBase::SharedPtr safety_timer_;
  rclcpp::TimerBase::SharedPtr recovery_timer_;
  
  // TF2
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  
  // State variables
  SafetyState current_safety_state_;
  sensor_msgs::msg::LaserScan::SharedPtr latest_scan_;
  geometry_msgs::msg::Twist latest_cmd_vel_;
  nav_msgs::msg::Odometry::SharedPtr latest_odom_;
  
  // Safety parameters
  double emergency_stop_distance_;      // 15cm emergency stop
  double warning_distance_;             // 25cm warning zone
  double inflation_radius_;             // 60cm inflation radius
  double collision_detection_distance_; // 10cm collision detection
  
  // Control parameters
  double safety_check_frequency_;       // 50Hz safety checks
  double max_linear_velocity_;          // Maximum allowed linear velocity
  double max_angular_velocity_;         // Maximum allowed angular velocity
  
  // Recovery parameters
  double recovery_backup_distance_;     // Distance to backup during recovery
  double recovery_rotation_angle_;      // Angle to rotate during recovery
  double recovery_timeout_;             // Maximum recovery time
  
  // State tracking
  rclcpp::Time emergency_stop_start_time_;
  rclcpp::Time last_safe_time_;
  rclcpp::Time recovery_start_time_;
  bool is_in_recovery_;
  bool emergency_stop_active_;
  
  // Thread safety
  std::mutex safety_mutex_;
  
  // Frame IDs
  std::string base_frame_;
  std::string laser_frame_;
  
  // Sector analysis for directional safety
  struct SafetySector
  {
    double start_angle;
    double end_angle;
    double min_distance;
    bool is_safe;
  };
  
  std::vector<SafetySector> safety_sectors_;
  void initializeSafetySectors();
  void updateSafetySectors(const sensor_msgs::msg::LaserScan::SharedPtr scan);
};

}  // namespace turtle_bot_navigation

#endif  // TURTLE_BOT_NAVIGATION__EMERGENCY_STOP_CONTROLLER_HPP_
