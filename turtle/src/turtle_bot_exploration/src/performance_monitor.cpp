#include "turtle_bot_exploration/performance_monitor.hpp"
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>

namespace turtle_bot_exploration
{

PerformanceMonitor::PerformanceMonitor(const rclcpp::NodeOptions & options)
: Node("performance_monitor", options),
  monitoring_active_(false),
  total_exploration_time_(0.0),
  total_distance_traveled_(0.0),
  goals_completed_(0),
  goals_failed_(0),
  average_goal_completion_time_(0.0),
  current_efficiency_score_(0.0),
  last_update_time_(std::chrono::steady_clock::now())
{
  // Initialize parameters
  this->declare_parameter("monitoring_frequency", 2.0);
  this->declare_parameter("performance_window_size", 10);
  this->declare_parameter("efficiency_threshold", 0.7);
  this->declare_parameter("enable_adaptive_tuning", true);
  this->declare_parameter("publish_detailed_metrics", true);
  
  monitoring_frequency_ = this->get_parameter("monitoring_frequency").as_double();
  performance_window_size_ = this->get_parameter("performance_window_size").as_int();
  efficiency_threshold_ = this->get_parameter("efficiency_threshold").as_double();
  enable_adaptive_tuning_ = this->get_parameter("enable_adaptive_tuning").as_bool();
  publish_detailed_metrics_ = this->get_parameter("publish_detailed_metrics").as_bool();
  
  // Initialize subscribers
  exploration_status_sub_ = this->create_subscription<std_msgs::msg::String>(
    "/exploration_status", 10,
    std::bind(&PerformanceMonitor::explorationStatusCallback, this, std::placeholders::_1));
    
  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    "/odom", 10,
    std::bind(&PerformanceMonitor::odomCallback, this, std::placeholders::_1));
    
  cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "/cmd_vel", 10,
    std::bind(&PerformanceMonitor::cmdVelCallback, this, std::placeholders::_1));
  
  // Initialize publishers
  performance_metrics_pub_ = this->create_publisher<std_msgs::msg::String>(
    "/performance_metrics", 10);
    
  efficiency_score_pub_ = this->create_publisher<std_msgs::msg::String>(
    "/efficiency_score", 10);
    
  adaptive_params_pub_ = this->create_publisher<std_msgs::msg::String>(
    "/adaptive_parameters", 10);
  
  // Initialize timer
  monitoring_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(static_cast<int>(1000.0 / monitoring_frequency_)),
    std::bind(&PerformanceMonitor::monitoringCallback, this));
    
  RCLCPP_INFO(this->get_logger(), "Performance Monitor initialized with %.1fHz monitoring", monitoring_frequency_);
}

void PerformanceMonitor::explorationStatusCallback(const std_msgs::msg::String::SharedPtr msg)
{
  std::string status = msg->data;
  
  // Track exploration state changes
  if (status.find("EXPLORING") != std::string::npos) {
    if (!monitoring_active_) {
      startMonitoring();
    }
  } else if (status.find("COMPLETE") != std::string::npos) {
    if (monitoring_active_) {
      stopMonitoring();
    }
  }
  
  // Track goal completions
  if (status.find("Goal completed") != std::string::npos) {
    recordGoalCompletion(true);
  } else if (status.find("Goal failed") != std::string::npos || status.find("timeout") != std::string::npos) {
    recordGoalCompletion(false);
  }
}

void PerformanceMonitor::odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
  if (!monitoring_active_) return;
  
  auto current_time = std::chrono::steady_clock::now();
  
  if (last_odom_msg_) {
    // Calculate distance traveled
    double dx = msg->pose.pose.position.x - last_odom_msg_->pose.pose.position.x;
    double dy = msg->pose.pose.position.y - last_odom_msg_->pose.pose.position.y;
    double distance = sqrt(dx * dx + dy * dy);
    
    total_distance_traveled_ += distance;
    
    // Calculate time elapsed
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      current_time - last_odom_time_).count() / 1000.0;
    total_exploration_time_ += time_diff;
  }
  
  last_odom_msg_ = msg;
  last_odom_time_ = current_time;
}

void PerformanceMonitor::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  if (!monitoring_active_) return;
  
  // Track velocity commands for efficiency analysis
  current_linear_velocity_ = sqrt(msg->linear.x * msg->linear.x + msg->linear.y * msg->linear.y);
  current_angular_velocity_ = abs(msg->angular.z);
  
  // Update velocity history for trend analysis
  velocity_history_.push_back({current_linear_velocity_, current_angular_velocity_, std::chrono::steady_clock::now()});
  
  // Keep only recent history
  auto cutoff_time = std::chrono::steady_clock::now() - std::chrono::seconds(30);
  velocity_history_.erase(
    std::remove_if(velocity_history_.begin(), velocity_history_.end(),
      [cutoff_time](const VelocityData& data) { return data.timestamp < cutoff_time; }),
    velocity_history_.end());
}

void PerformanceMonitor::monitoringCallback()
{
  if (!monitoring_active_) return;
  
  auto current_time = std::chrono::steady_clock::now();
  auto time_since_update = std::chrono::duration_cast<std::chrono::seconds>(
    current_time - last_update_time_).count();
  
  if (time_since_update >= 1.0) {  // Update every second
    updatePerformanceMetrics();
    publishPerformanceMetrics();
    
    if (enable_adaptive_tuning_) {
      performAdaptiveTuning();
    }
    
    last_update_time_ = current_time;
  }
}

void PerformanceMonitor::startMonitoring()
{
  monitoring_active_ = true;
  monitoring_start_time_ = std::chrono::steady_clock::now();
  
  RCLCPP_INFO(this->get_logger(), "Performance monitoring started");
}

void PerformanceMonitor::stopMonitoring()
{
  monitoring_active_ = false;
  
  // Calculate final metrics
  auto total_time = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - monitoring_start_time_).count();
    
  RCLCPP_INFO(this->get_logger(), 
              "Performance monitoring stopped. Total time: %lds, Distance: %.2fm, Goals: %d/%d",
              total_time, total_distance_traveled_, goals_completed_, goals_completed_ + goals_failed_);
}

void PerformanceMonitor::recordGoalCompletion(bool success)
{
  auto current_time = std::chrono::steady_clock::now();
  
  if (success) {
    goals_completed_++;
    
    // Update average completion time
    if (last_goal_start_time_.time_since_epoch().count() > 0) {
      auto completion_time = std::chrono::duration_cast<std::chrono::seconds>(
        current_time - last_goal_start_time_).count();
      
      goal_completion_times_.push_back(completion_time);
      
      // Keep only recent completion times
      if (goal_completion_times_.size() > performance_window_size_) {
        goal_completion_times_.erase(goal_completion_times_.begin());
      }
      
      // Calculate average
      double total_time = 0.0;
      for (double time : goal_completion_times_) {
        total_time += time;
      }
      average_goal_completion_time_ = total_time / goal_completion_times_.size();
    }
  } else {
    goals_failed_++;
  }
  
  last_goal_start_time_ = current_time;
}

void PerformanceMonitor::updatePerformanceMetrics()
{
  // Calculate efficiency score
  double success_rate = goals_completed_ > 0 ? 
    static_cast<double>(goals_completed_) / (goals_completed_ + goals_failed_) : 0.0;
  
  double time_efficiency = average_goal_completion_time_ > 0 ? 
    std::min(30.0 / average_goal_completion_time_, 1.0) : 0.0;
  
  double movement_efficiency = calculateMovementEfficiency();
  
  current_efficiency_score_ = (success_rate * 0.4) + (time_efficiency * 0.3) + (movement_efficiency * 0.3);
}

double PerformanceMonitor::calculateMovementEfficiency()
{
  if (velocity_history_.empty()) return 0.5;
  
  double total_linear = 0.0;
  double total_angular = 0.0;
  
  for (const auto& vel_data : velocity_history_) {
    total_linear += vel_data.linear_velocity;
    total_angular += vel_data.angular_velocity;
  }
  
  double avg_linear = total_linear / velocity_history_.size();
  double avg_angular = total_angular / velocity_history_.size();
  
  // Higher linear velocity and lower angular velocity indicate better efficiency
  double linear_score = std::min(avg_linear / 0.3, 1.0);  // Normalize to max expected velocity
  double angular_penalty = std::min(avg_angular / 1.0, 0.5);  // Penalize excessive rotation
  
  return std::max(0.0, linear_score - angular_penalty);
}

void PerformanceMonitor::publishPerformanceMetrics()
{
  if (!publish_detailed_metrics_) return;
  
  // Publish comprehensive metrics
  std_msgs::msg::String metrics_msg;
  metrics_msg.data = formatPerformanceMetrics();
  performance_metrics_pub_->publish(metrics_msg);
  
  // Publish efficiency score
  std_msgs::msg::String efficiency_msg;
  efficiency_msg.data = "Efficiency Score: " + std::to_string(current_efficiency_score_);
  efficiency_score_pub_->publish(efficiency_msg);
}

std::string PerformanceMonitor::formatPerformanceMetrics()
{
  std::ostringstream oss;
  oss << "Performance Metrics: ";
  oss << "Goals: " << goals_completed_ << "/" << (goals_completed_ + goals_failed_);
  oss << ", Avg Time: " << std::fixed << std::setprecision(1) << average_goal_completion_time_ << "s";
  oss << ", Distance: " << std::fixed << std::setprecision(2) << total_distance_traveled_ << "m";
  oss << ", Efficiency: " << std::fixed << std::setprecision(3) << current_efficiency_score_;
  
  return oss.str();
}

void PerformanceMonitor::performAdaptiveTuning()
{
  if (goals_completed_ < 3) return;  // Need sufficient data
  
  std::vector<std::string> adaptations;
  
  // Adapt based on efficiency score
  if (current_efficiency_score_ < efficiency_threshold_) {
    adaptations.push_back("Low efficiency detected - suggesting parameter adjustments");
    
    if (average_goal_completion_time_ > 40.0) {
      adaptations.push_back("Slow goal completion - recommend reducing goal timeout");
    }
    
    double success_rate = static_cast<double>(goals_completed_) / (goals_completed_ + goals_failed_);
    if (success_rate < 0.8) {
      adaptations.push_back("Low success rate - recommend increasing goal tolerance");
    }
  }
  
  if (!adaptations.empty()) {
    std_msgs::msg::String adaptive_msg;
    adaptive_msg.data = "Adaptive Recommendations: ";
    for (const auto& adaptation : adaptations) {
      adaptive_msg.data += adaptation + "; ";
    }
    adaptive_params_pub_->publish(adaptive_msg);
  }
}

}  // namespace turtle_bot_exploration
