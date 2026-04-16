#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/bool.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>

class VelocitySafetyFilter : public rclcpp::Node
{
public:
  VelocitySafetyFilter() : Node("velocity_safety_filter")
  {
    // Declare parameters
    this->declare_parameter("emergency_stop_distance", 0.15);
    this->declare_parameter("warning_distance", 0.25);
    
    emergency_stop_distance_ = this->get_parameter("emergency_stop_distance").as_double();
    warning_distance_ = this->get_parameter("warning_distance").as_double();
    
    // Subscribers
    cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", 10, std::bind(&VelocitySafetyFilter::cmdVelCallback, this, std::placeholders::_1));
    
    emergency_stop_sub_ = this->create_subscription<std_msgs::msg::Bool>(
      "/emergency_stop", 10, std::bind(&VelocitySafetyFilter::emergencyStopCallback, this, std::placeholders::_1));
    
    laser_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
      "/scan", rclcpp::SensorDataQoS(), std::bind(&VelocitySafetyFilter::laserCallback, this, std::placeholders::_1));
    
    // Publishers
    safe_cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_filtered", 10);
    
    // Initialize state
    emergency_stop_active_ = false;
    
    RCLCPP_INFO(this->get_logger(), "Velocity Safety Filter initialized");
  }

private:
  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    auto safe_cmd = *msg;
    
    // Apply emergency stop if active
    if (emergency_stop_active_) {
      safe_cmd.linear.x = 0.0;
      safe_cmd.linear.y = 0.0;
      safe_cmd.linear.z = 0.0;
      safe_cmd.angular.x = 0.0;
      safe_cmd.angular.y = 0.0;
      safe_cmd.angular.z = 0.0;
    } else if (latest_scan_) {
      // Apply additional safety filtering based on laser data
      applySafetyFiltering(safe_cmd);
    }
    
    safe_cmd_vel_pub_->publish(safe_cmd);
  }
  
  void emergencyStopCallback(const std_msgs::msg::Bool::SharedPtr msg)
  {
    emergency_stop_active_ = msg->data;
    if (emergency_stop_active_) {
      RCLCPP_WARN(this->get_logger(), "Emergency stop activated by safety controller");
    }
  }
  
  void laserCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
  {
    latest_scan_ = msg;
  }
  
  void applySafetyFiltering(geometry_msgs::msg::Twist & cmd)
  {
    if (!latest_scan_ || latest_scan_->ranges.empty()) {
      return;
    }
    
    // Check front sector for obstacles
    double min_front_distance = std::numeric_limits<double>::max();
    size_t front_start = latest_scan_->ranges.size() * 0.4;  // -36 degrees
    size_t front_end = latest_scan_->ranges.size() * 0.6;    // +36 degrees
    
    for (size_t i = front_start; i < front_end && i < latest_scan_->ranges.size(); ++i) {
      double range = latest_scan_->ranges[i];
      if (range > latest_scan_->range_min && range < latest_scan_->range_max && std::isfinite(range)) {
        min_front_distance = std::min(min_front_distance, static_cast<double>(range));
      }
    }
    
    // Apply velocity scaling based on obstacle distance
    if (min_front_distance < emergency_stop_distance_) {
      // Emergency stop
      cmd.linear.x = 0.0;
      cmd.angular.z = 0.0;
    } else if (min_front_distance < warning_distance_) {
      // Reduce velocity in warning zone
      double scale_factor = (min_front_distance - emergency_stop_distance_) / 
                           (warning_distance_ - emergency_stop_distance_);
      scale_factor = std::max(0.1, std::min(1.0, scale_factor));
      
      if (cmd.linear.x > 0.0) {  // Only scale forward motion
        cmd.linear.x *= scale_factor;
      }
    }
    
    // Limit maximum velocities for safety
    cmd.linear.x = std::max(-0.35, std::min(0.35, cmd.linear.x));
    cmd.angular.z = std::max(-0.8, std::min(0.8, cmd.angular.z));
  }
  
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr emergency_stop_sub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr laser_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr safe_cmd_vel_pub_;
  
  sensor_msgs::msg::LaserScan::SharedPtr latest_scan_;
  bool emergency_stop_active_;
  double emergency_stop_distance_;
  double warning_distance_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<VelocitySafetyFilter>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
