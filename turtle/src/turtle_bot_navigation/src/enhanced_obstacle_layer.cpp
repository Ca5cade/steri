#include "turtle_bot_navigation/enhanced_obstacle_layer.hpp"
#include <nav2_costmap_2d/costmap_math.hpp>
#include <pluginlib/class_list_macros.hpp>
#include <cmath>
#include <algorithm>

namespace turtle_bot_navigation
{

void EnhancedObstacleLayer::onInitialize()
{
  // Call parent initialization
  ObstacleLayer::onInitialize();
  
  // Declare enhanced parameters
  declareParameter("wall_detection_range", rclcpp::ParameterValue(3.0));
  declareParameter("wall_thickness", rclcpp::ParameterValue(0.1));
  declareParameter("wall_confidence_threshold", rclcpp::ParameterValue(0.8));
  declareParameter("enable_wall_enhancement", rclcpp::ParameterValue(true));
  
  // Get enhanced parameters
  auto node = node_.lock();
  if (node) {
    node->get_parameter(name_ + ".wall_detection_range", wall_detection_range_);
    node->get_parameter(name_ + ".wall_thickness", wall_thickness_);
    node->get_parameter(name_ + ".wall_confidence_threshold", wall_confidence_threshold_);
    node->get_parameter(name_ + ".enable_wall_enhancement", enable_wall_enhancement_);
  }
  
  // Create enhanced laser scan subscription
  if (enable_wall_enhancement_) {
    auto node = node_.lock();
    if (node) {
      enhanced_laser_sub_ = node->create_subscription<sensor_msgs::msg::LaserScan>(
        "/scan", rclcpp::SensorDataQoS(),
        std::bind(&EnhancedObstacleLayer::laserScanCallback, this, std::placeholders::_1));
    }
  }
  
  RCLCPP_INFO(
    logger_,
    "Enhanced Obstacle Layer initialized with wall detection range: %.2fm",
    wall_detection_range_);
}

void EnhancedObstacleLayer::laserScanCallback(const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  if (!enable_wall_enhancement_) {
    return;
  }
  
  // Process scan for enhanced wall detection
  processLaserScan(scan);
}

void EnhancedObstacleLayer::processLaserScan(const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  if (!scan || scan->ranges.empty()) {
    return;
  }
  
  // Detect wall points with enhanced algorithm
  auto wall_points = detectWallPoints(scan);
  
  // Update wall data with thread safety
  std::lock_guard<std::mutex> lock(wall_data_mutex_);
  detected_walls_ = wall_points;
}

std::vector<geometry_msgs::msg::Point> EnhancedObstacleLayer::detectWallPoints(
  const sensor_msgs::msg::LaserScan::SharedPtr scan)
{
  std::vector<geometry_msgs::msg::Point> wall_points;
  
  if (!scan || scan->ranges.empty()) {
    return wall_points;
  }
  
  // Enhanced wall detection algorithm
  std::vector<std::pair<double, double>> valid_points;
  
  // Extract valid points within enhanced detection range
  for (size_t i = 0; i < scan->ranges.size(); ++i) {
    double range = scan->ranges[i];
    double angle = scan->angle_min + i * scan->angle_increment;
    
    if (range > scan->range_min && 
        range < std::min(static_cast<double>(scan->range_max), wall_detection_range_) &&
        std::isfinite(range)) {
      valid_points.emplace_back(angle, range);
    }
  }
  
  if (valid_points.size() < 3) {
    return wall_points;
  }
  
  // Group consecutive points and detect linear segments (walls)
  std::vector<std::vector<std::pair<double, double>>> segments;
  std::vector<std::pair<double, double>> current_segment;
  
  const double max_gap = 0.15;  // 15cm max gap between points
  const size_t min_segment_size = 3;
  
  for (size_t i = 0; i < valid_points.size(); ++i) {
    if (current_segment.empty()) {
      current_segment.push_back(valid_points[i]);
    } else {
      // Check distance between consecutive points
      auto prev_point = current_segment.back();
      double x1 = prev_point.second * cos(prev_point.first);
      double y1 = prev_point.second * sin(prev_point.first);
      double x2 = valid_points[i].second * cos(valid_points[i].first);
      double y2 = valid_points[i].second * sin(valid_points[i].first);
      double distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
      
      if (distance < max_gap) {
        current_segment.push_back(valid_points[i]);
      } else {
        if (current_segment.size() >= min_segment_size) {
          segments.push_back(current_segment);
        }
        current_segment.clear();
        current_segment.push_back(valid_points[i]);
      }
    }
  }
  
  // Add last segment
  if (current_segment.size() >= min_segment_size) {
    segments.push_back(current_segment);
  }
  
  // Convert segments to wall points with enhanced processing
  for (const auto & segment : segments) {
    if (segment.size() < min_segment_size) continue;
    
    // Check if segment is linear (wall-like)
    double linearity = calculateSegmentLinearity(segment);
    
    if (linearity >= wall_confidence_threshold_) {
      // Add points from this wall segment
      for (const auto & point : segment) {
        geometry_msgs::msg::Point wall_point;
        wall_point.x = point.second * cos(point.first);
        wall_point.y = point.second * sin(point.first);
        wall_point.z = 0.0;
        wall_points.push_back(wall_point);
      }
    }
  }
  
  return wall_points;
}

double EnhancedObstacleLayer::calculateSegmentLinearity(
  const std::vector<std::pair<double, double>> & segment)
{
  if (segment.size() < 3) {
    return 0.0;
  }

  // Convert to Cartesian coordinates
  std::vector<std::pair<double, double>> points;
  for (const auto & polar_point : segment) {
    double x = polar_point.second * cos(polar_point.first);
    double y = polar_point.second * sin(polar_point.first);
    points.emplace_back(x, y);
  }

  // Fit line using least squares
  double sum_x = 0.0, sum_y = 0.0, sum_xx = 0.0, sum_xy = 0.0;
  size_t n = points.size();

  for (const auto & point : points) {
    sum_x += point.first;
    sum_y += point.second;
    sum_xx += point.first * point.first;
    sum_xy += point.first * point.second;
  }

  double mean_x = sum_x / n;
  double mean_y = sum_y / n;

  // Calculate R-squared (coefficient of determination)
  double ss_tot = 0.0, ss_res = 0.0;

  // Line parameters
  double denominator = sum_xx - n * mean_x * mean_x;
  if (std::abs(denominator) < 1e-6) {
    return 0.0;  // Vertical line or degenerate case
  }

  double slope = (sum_xy - n * mean_x * mean_y) / denominator;
  double intercept = mean_y - slope * mean_x;

  for (const auto & point : points) {
    double predicted_y = slope * point.first + intercept;
    ss_res += (point.second - predicted_y) * (point.second - predicted_y);
    ss_tot += (point.second - mean_y) * (point.second - mean_y);
  }

  if (ss_tot < 1e-6) {
    return 1.0;  // All points are the same
  }

  double r_squared = 1.0 - (ss_res / ss_tot);
  return std::max(0.0, std::min(1.0, r_squared));
}

void EnhancedObstacleLayer::updateBounds(
  double robot_x, double robot_y, double robot_yaw,
  double * min_x, double * min_y, double * max_x, double * max_y)
{
  // Call parent method
  ObstacleLayer::updateBounds(robot_x, robot_y, robot_yaw, min_x, min_y, max_x, max_y);
  
  if (!enable_wall_enhancement_) {
    return;
  }
  
  // Expand bounds to include detected walls
  std::lock_guard<std::mutex> lock(wall_data_mutex_);
  for (const auto & wall_point : detected_walls_) {
    *min_x = std::min(*min_x, wall_point.x - wall_thickness_);
    *min_y = std::min(*min_y, wall_point.y - wall_thickness_);
    *max_x = std::max(*max_x, wall_point.x + wall_thickness_);
    *max_y = std::max(*max_y, wall_point.y + wall_thickness_);
  }
}

void EnhancedObstacleLayer::updateCosts(
  nav2_costmap_2d::Costmap2D & master_grid,
  int min_i, int min_j, int max_i, int max_j)
{
  // Call parent method
  ObstacleLayer::updateCosts(master_grid, min_i, min_j, max_i, max_j);
  
  if (!enable_wall_enhancement_) {
    return;
  }
  
  // Apply enhanced wall detection
  enhanceWallDetection(master_grid, min_i, min_j, max_i, max_j);
}

void EnhancedObstacleLayer::enhanceWallDetection(
  nav2_costmap_2d::Costmap2D & master_grid,
  int /*min_i*/, int /*min_j*/, int /*max_i*/, int /*max_j*/)
{
  std::lock_guard<std::mutex> lock(wall_data_mutex_);
  
  // Mark detected wall points with higher confidence
  for (const auto & wall_point : detected_walls_) {
    unsigned int mx, my;
    if (master_grid.worldToMap(wall_point.x, wall_point.y, mx, my)) {
      // Set high cost for wall points
      master_grid.setCost(mx, my, nav2_costmap_2d::LETHAL_OBSTACLE);
      
      // Add thickness around wall points for better detection
      int thickness_cells = static_cast<int>(wall_thickness_ / master_grid.getResolution());
      for (int dx = -thickness_cells; dx <= thickness_cells; ++dx) {
        for (int dy = -thickness_cells; dy <= thickness_cells; ++dy) {
          int nx = static_cast<int>(mx) + dx;
          int ny = static_cast<int>(my) + dy;
          if (nx >= 0 && ny >= 0 && 
              nx < static_cast<int>(master_grid.getSizeInCellsX()) &&
              ny < static_cast<int>(master_grid.getSizeInCellsY())) {
            double distance = sqrt(dx * dx + dy * dy) * master_grid.getResolution();
            if (distance <= wall_thickness_) {
              unsigned char current_cost = master_grid.getCost(nx, ny);
              unsigned char new_cost = nav2_costmap_2d::INSCRIBED_INFLATED_OBSTACLE;
              master_grid.setCost(nx, ny, std::max(current_cost, new_cost));
            }
          }
        }
      }
    }
  }
}

}  // namespace turtle_bot_navigation

PLUGINLIB_EXPORT_CLASS(turtle_bot_navigation::EnhancedObstacleLayer, nav2_costmap_2d::Layer)
