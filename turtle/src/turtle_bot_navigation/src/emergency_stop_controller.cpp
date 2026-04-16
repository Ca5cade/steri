#include "turtle_bot_navigation/emergency_stop_controller.hpp"
#include <cmath>
#include <algorithm>
#include <tf2/utils.h>

namespace turtle_bot_navigation
{

EmergencyStopController::EmergencyStopController(const rclcpp::NodeOptions & options)
: Node("emergency_stop_controller", options),
  current_safety_state_(SafetyState::SAFE),
  is_in_recovery_(false),
  emergency_stop_active_(false)
{
  // Declare parameters
  this->declare_parameter("emergency_stop_distance", 0.15);  // 15cm emergency stop
  this->declare_parameter("warning_distance", 0.25);        // 25cm warning zone
  this->declare_parameter("inflation_radius", 0.60);        // 60cm inflation radius
  this->declare_parameter("collision_detection_distance", 0.10);  // 10cm collision detection
  this->declare_parameter("safety_check_frequency", 50.0);  // 50Hz safety checks
  this->declare_parameter("max_linear_velocity", 0.4);      // Max linear velocity
  this->declare_parameter("max_angular_velocity", 1.0);     // Max angular velocity
  this->declare_parameter("recovery_backup_distance", 0.3); // 30cm backup
  this->declare_parameter("recovery_rotation_angle", 1.57); // 90 degrees
  this->declare_parameter("recovery_timeout", 10.0);        // 10 seconds
  this->declare_parameter("base_frame", std::string("base_link"));
  this->declare_parameter("laser_frame", std::string("laser"));

  // Get parameters
  emergency_stop_distance_ = this->get_parameter("emergency_stop_distance").as_double();
  warning_distance_ = this->get_parameter("warning_distance").as_double();
  inflation_radius_ = this->get_parameter("inflation_radius").as_double();
  collision_detection_distance_ = this->get_parameter("collision_detection_distance").as_double();
  safety_check_frequency_ = this->get_parameter("safety_check_frequency").as_double();
  max_linear_velocity_ = this->get_parameter("max_linear_velocity").as_double();
  max_angular_velocity_ = this->get_parameter("max_angular_velocity").as_double();
  recovery_backup_distance_ = this->get_parameter("recovery_backup_distance").as_double();
  recovery_rotation_angle_ = this->get_parameter("recovery_rotation_angle").as_double();
  recovery_timeout_ = this->get_parameter("recovery_timeout").as_double();
  base_frame_ = this->get_parameter("base_frame").as_string();
  laser_frame_ = this->get_parameter("laser_frame").as_string();

  // Initialize TF2
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Create subscribers
  laser_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
    "/scan", rclcpp::SensorDataQoS(),
    std::bind(&EmergencyStopController::laserScanCallback, this, std::placeholders::_1));
  
  cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "/cmd_vel", 10,
    std::bind(&EmergencyStopController::cmdVelCallback, this, std::placeholders::_1));
  
  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    "/odom", 10,
    std::bind(&EmergencyStopController::odomCallback, this, std::placeholders::_1));

  // Create publishers
  safe_cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_safe", 10);
  emergency_stop_pub_ = this->create_publisher<std_msgs::msg::Bool>("/emergency_stop", 10);
  safety_status_pub_ = this->create_publisher<std_msgs::msg::String>("/safety_status", 10);

  // Create timers
  auto safety_timer_period = std::chrono::milliseconds(
    static_cast<int>(1000.0 / safety_check_frequency_));
  safety_timer_ = this->create_wall_timer(
    safety_timer_period, std::bind(&EmergencyStopController::safetyTimerCallback, this));

  auto recovery_timer_period = std::chrono::milliseconds(100);  // 10Hz recovery
  recovery_timer_ = this->create_wall_timer(
    recovery_timer_period, std::bind(&EmergencyStopController::recoveryTimerCallback, this));

  // Initialize safety sectors
  initializeSafetySectors();

  // Initialize timestamps
  last_safe_time_ = this->now();

  RCLCPP_INFO(this->get_logger(), 
              "Emergency Stop Controller initialized - Emergency stop: %.2fm, Warning: %.2fm",
              emergency_stop_distance_, warning_distance_);
}

void EmergencyStopController::laserScanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(safety_mutex_);
  latest_scan_ = msg;
  
  // Update safety sectors
  updateSafetySectors(msg);
  
  // Analyze safety state
  current_safety_state_ = analyzeSafetyState(msg);
}

void EmergencyStopController::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(safety_mutex_);
  latest_cmd_vel_ = *msg;
}

void EmergencyStopController::odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(safety_mutex_);
  latest_odom_ = msg;
}

void EmergencyStopController::safetyTimerCallback()
{
  std::lock_guard<std::mutex> lock(safety_mutex_);
  
  if (!latest_scan_) {
    return;
  }

  // Publish safety status
  publishSafetyStatus();

  // Handle different safety states
  switch (current_safety_state_) {
    case SafetyState::SAFE:
      emergency_stop_active_ = false;
      last_safe_time_ = this->now();
      publishSafeVelocity(latest_cmd_vel_);
      break;
      
    case SafetyState::WARNING:
      emergency_stop_active_ = false;
      // Reduce velocity in warning zone
      {
        auto reduced_cmd = latest_cmd_vel_;
        reduced_cmd.linear.x *= 0.5;  // 50% speed reduction
        reduced_cmd.angular.z *= 0.7; // 30% angular reduction
        publishSafeVelocity(reduced_cmd);
      }
      break;
      
    case SafetyState::EMERGENCY_STOP:
    case SafetyState::COLLISION_DETECTED:
      executeEmergencyStop();
      break;
      
    case SafetyState::RECOVERY_NEEDED:
      if (!is_in_recovery_) {
        executeRecoveryBehavior();
      }
      break;
  }
}

void EmergencyStopController::recoveryTimerCallback()
{
  if (!is_in_recovery_) {
    return;
  }
  
  auto recovery_duration = (this->now() - recovery_start_time_).seconds();
  
  if (recovery_duration > recovery_timeout_) {
    // Recovery timeout - stop recovery
    is_in_recovery_ = false;
    executeEmergencyStop();
    RCLCPP_WARN(this->get_logger(), "Recovery timeout - stopping robot");
    return;
  }
  
  // Execute recovery behavior
  geometry_msgs::msg::Twist recovery_cmd;
  
  if (recovery_duration < 2.0) {
    // Phase 1: Backup
    recovery_cmd.linear.x = -0.1;  // Slow backup
    recovery_cmd.angular.z = 0.0;
  } else if (recovery_duration < 4.0) {
    // Phase 2: Rotate
    recovery_cmd.linear.x = 0.0;
    recovery_cmd.angular.z = 0.5;  // Rotate to find clear path
  } else {
    // Phase 3: Check if clear
    if (current_safety_state_ == SafetyState::SAFE) {
      is_in_recovery_ = false;
      RCLCPP_INFO(this->get_logger(), "Recovery successful - resuming normal operation");
    } else {
      // Continue recovery
      recovery_cmd.linear.x = -0.05;  // Very slow backup
      recovery_cmd.angular.z = 0.3;   // Slow rotation
    }
  }
  
  if (is_in_recovery_) {
    safe_cmd_vel_pub_->publish(recovery_cmd);
  }
}

SafetyState EmergencyStopController::analyzeSafetyState(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  if (!scan || scan->ranges.empty()) {
    return SafetyState::SAFE;
  }
  
  // Check for collision
  if (isCollisionImminent(scan)) {
    return SafetyState::COLLISION_DETECTED;
  }
  
  // Check for emergency stop condition
  double min_distance = std::numeric_limits<double>::max();
  for (size_t i = 0; i < scan->ranges.size(); ++i) {
    double range = scan->ranges[i];
    if (range > scan->range_min && range < scan->range_max && std::isfinite(range)) {
      min_distance = std::min(min_distance, static_cast<double>(range));
    }
  }
  
  if (min_distance < emergency_stop_distance_) {
    return SafetyState::EMERGENCY_STOP;
  }
  
  // Check if stuck against wall
  if (isStuckAgainstWall(scan)) {
    return SafetyState::RECOVERY_NEEDED;
  }
  
  // Check for warning zone
  if (min_distance < warning_distance_) {
    return SafetyState::WARNING;
  }
  
  // Check if in inflation zone while moving towards obstacle
  if (isInInflationZone(scan) && isMovingTowardsObstacle(latest_cmd_vel_, scan)) {
    return SafetyState::WARNING;
  }
  
  return SafetyState::SAFE;
}

bool EmergencyStopController::isCollisionImminent(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  // Check front sector for imminent collision
  double front_min_distance = getMinDistanceInSector(scan, -0.5, 0.5);  // ±30 degrees
  
  return front_min_distance < collision_detection_distance_;
}

bool EmergencyStopController::isInInflationZone(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  // Check if any obstacle is within inflation radius
  for (size_t i = 0; i < scan->ranges.size(); ++i) {
    double range = scan->ranges[i];
    if (range > scan->range_min && range < scan->range_max && 
        std::isfinite(range) && range < inflation_radius_) {
      return true;
    }
  }
  return false;
}

bool EmergencyStopController::isStuckAgainstWall(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  // Check if robot is very close to obstacles on multiple sides
  double front_distance = getMinDistanceInSector(scan, -0.3, 0.3);    // Front
  double left_distance = getMinDistanceInSector(scan, 0.3, 1.2);      // Left
  double right_distance = getMinDistanceInSector(scan, -1.2, -0.3);   // Right
  
  int close_sides = 0;
  if (front_distance < warning_distance_) close_sides++;
  if (left_distance < warning_distance_) close_sides++;
  if (right_distance < warning_distance_) close_sides++;
  
  return close_sides >= 2;  // Stuck if close on 2+ sides
}

void EmergencyStopController::executeEmergencyStop()
{
  if (!emergency_stop_active_) {
    emergency_stop_start_time_ = this->now();
    emergency_stop_active_ = true;
    RCLCPP_WARN(this->get_logger(), "EMERGENCY STOP ACTIVATED!");
  }
  
  // Publish zero velocity
  geometry_msgs::msg::Twist stop_cmd;
  stop_cmd.linear.x = 0.0;
  stop_cmd.linear.y = 0.0;
  stop_cmd.linear.z = 0.0;
  stop_cmd.angular.x = 0.0;
  stop_cmd.angular.y = 0.0;
  stop_cmd.angular.z = 0.0;
  
  safe_cmd_vel_pub_->publish(stop_cmd);
  
  // Publish emergency stop status
  std_msgs::msg::Bool emergency_msg;
  emergency_msg.data = true;
  emergency_stop_pub_->publish(emergency_msg);
}

void EmergencyStopController::executeRecoveryBehavior()
{
  if (!is_in_recovery_) {
    is_in_recovery_ = true;
    recovery_start_time_ = this->now();
    RCLCPP_WARN(this->get_logger(), "Starting recovery behavior - robot appears stuck");
  }
}

void EmergencyStopController::publishSafeVelocity(const geometry_msgs::msg::Twist & original_cmd)
{
  auto safe_cmd = original_cmd;
  
  // Limit velocities to safe ranges
  safe_cmd.linear.x = std::max(-max_linear_velocity_, 
                               std::min(max_linear_velocity_, safe_cmd.linear.x));
  safe_cmd.angular.z = std::max(-max_angular_velocity_, 
                                std::min(max_angular_velocity_, safe_cmd.angular.z));
  
  safe_cmd_vel_pub_->publish(safe_cmd);
  
  // Publish emergency stop status (false when safe)
  std_msgs::msg::Bool emergency_msg;
  emergency_msg.data = false;
  emergency_stop_pub_->publish(emergency_msg);
}

void EmergencyStopController::publishSafetyStatus()
{
  std_msgs::msg::String status_msg;
  
  switch (current_safety_state_) {
    case SafetyState::SAFE:
      status_msg.data = "SAFE";
      break;
    case SafetyState::WARNING:
      status_msg.data = "WARNING";
      break;
    case SafetyState::EMERGENCY_STOP:
      status_msg.data = "EMERGENCY_STOP";
      break;
    case SafetyState::COLLISION_DETECTED:
      status_msg.data = "COLLISION_DETECTED";
      break;
    case SafetyState::RECOVERY_NEEDED:
      status_msg.data = "RECOVERY_NEEDED";
      break;
  }
  
  safety_status_pub_->publish(status_msg);
}

double EmergencyStopController::getMinDistanceInSector(
  const sensor_msgs::msg::LaserScan::SharedPtr scan,
  double start_angle, double end_angle)
{
  double min_distance = std::numeric_limits<double>::max();
  
  for (size_t i = 0; i < scan->ranges.size(); ++i) {
    double angle = scan->angle_min + i * scan->angle_increment;
    
    if (angle >= start_angle && angle <= end_angle) {
      double range = scan->ranges[i];
      if (range > scan->range_min && range < scan->range_max && std::isfinite(range)) {
        min_distance = std::min(min_distance, static_cast<double>(range));
      }
    }
  }
  
  return min_distance;
}

bool EmergencyStopController::isMovingTowardsObstacle(
  const geometry_msgs::msg::Twist & cmd,
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  // Only check if moving forward
  if (cmd.linear.x <= 0.0) {
    return false;
  }
  
  // Check front sector for obstacles
  double front_distance = getMinDistanceInSector(scan, -0.5, 0.5);
  
  return front_distance < inflation_radius_;
}

void EmergencyStopController::initializeSafetySectors()
{
  // Initialize 8 sectors around the robot
  const int num_sectors = 8;
  const double sector_angle = 2 * M_PI / num_sectors;
  
  safety_sectors_.clear();
  for (int i = 0; i < num_sectors; ++i) {
    SafetySector sector;
    sector.start_angle = i * sector_angle - M_PI;
    sector.end_angle = (i + 1) * sector_angle - M_PI;
    sector.min_distance = std::numeric_limits<double>::max();
    sector.is_safe = true;
    safety_sectors_.push_back(sector);
  }
}

void EmergencyStopController::updateSafetySectors(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  // Reset sectors
  for (auto & sector : safety_sectors_) {
    sector.min_distance = std::numeric_limits<double>::max();
    sector.is_safe = true;
  }
  
  // Update sector distances
  for (auto & sector : safety_sectors_) {
    sector.min_distance = getMinDistanceInSector(scan, sector.start_angle, sector.end_angle);
    sector.is_safe = sector.min_distance > warning_distance_;
  }
}

}  // namespace turtle_bot_navigation
