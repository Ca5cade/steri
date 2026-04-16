#ifndef TURTLE_BOT_NAVIGATION__WALL_FOLLOWER_HPP_
#define TURTLE_BOT_NAVIGATION__WALL_FOLLOWER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include <vector>
#include <memory>
#include <string>

namespace turtle_bot_navigation
{

struct WallSegment
{
  double start_angle;
  double end_angle;
  double distance;
  double length;
  std::vector<std::pair<double, double>> points;  // (angle, distance) pairs
  double confidence;
};

enum class WallFollowingState
{
  SEARCHING_FOR_WALL,
  APPROACHING_WALL,
  FOLLOWING_WALL_LEFT,
  FOLLOWING_WALL_RIGHT,
  CORNER_NAVIGATION,
  OBSTACLE_AVOIDANCE,
  LOST_WALL
};

class WallFollower : public rclcpp::Node
{
public:
  explicit WallFollower(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~WallFollower() = default;

private:
  // Callback functions
  void laserScanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg);
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg);
  void controlTimerCallback();
  
  // Wall detection functions
  std::vector<WallSegment> detectWalls(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  WallSegment findBestWallToFollow(const std::vector<WallSegment> & walls);
  bool isValidWallPoint(double distance, double angle, const sensor_msgs::msg::LaserScan::SharedPtr scan);
  
  // Wall following control
  geometry_msgs::msg::Twist calculateWallFollowingCommand(const WallSegment & wall);
  geometry_msgs::msg::Twist calculateApproachCommand(const WallSegment & wall);
  geometry_msgs::msg::Twist calculateSearchCommand();
  
  // Utility functions
  double calculateWallDistance(const WallSegment & wall);
  double calculateWallAngle(const WallSegment & wall);
  bool isWallLost(const std::vector<WallSegment> & walls);
  void updateState(const std::vector<WallSegment> & walls);
  void publishVisualization(const std::vector<WallSegment> & walls);
  
  // ROS2 interfaces
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr laser_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr wall_markers_pub_;
  rclcpp::TimerBase::SharedPtr control_timer_;
  
  // TF2
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  
  // State variables
  WallFollowingState current_state_;
  WallSegment current_wall_;
  geometry_msgs::msg::Pose current_pose_;
  sensor_msgs::msg::LaserScan::SharedPtr latest_scan_;
  
  // Parameters
  double target_wall_distance_;      // Desired distance from wall (30-50cm)
  double max_wall_detection_range_;  // Maximum range for wall detection (2-3m)
  double min_wall_length_;           // Minimum wall segment length
  double wall_following_speed_;      // Speed while following walls
  double approach_speed_;            // Speed when approaching walls
  double search_speed_;              // Speed when searching for walls
  double angular_gain_;              // PID gain for angular control
  double distance_gain_;             // PID gain for distance control
  double wall_confidence_threshold_; // Minimum confidence for wall detection
  
  // Control parameters
  double max_linear_velocity_;
  double max_angular_velocity_;
  double control_frequency_;
  
  // Wall detection parameters
  double min_wall_points_;
  double max_point_gap_;
  double wall_angle_tolerance_;
  
  // Safety parameters
  double emergency_stop_distance_;
  double obstacle_avoidance_distance_;
  
  // State tracking
  rclcpp::Time last_wall_seen_;
  double wall_lost_timeout_;
  
  // Frame IDs
  std::string base_frame_;
  std::string laser_frame_;
};

}  // namespace turtle_bot_navigation

#endif  // TURTLE_BOT_NAVIGATION__WALL_FOLLOWER_HPP_
