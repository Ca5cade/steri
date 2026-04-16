#ifndef TURTLE_BOT_NAVIGATION__ENHANCED_OBSTACLE_LAYER_HPP_
#define TURTLE_BOT_NAVIGATION__ENHANCED_OBSTACLE_LAYER_HPP_

#include <nav2_costmap_2d/obstacle_layer.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <vector>
#include <memory>

namespace turtle_bot_navigation
{

class EnhancedObstacleLayer : public nav2_costmap_2d::ObstacleLayer
{
public:
  EnhancedObstacleLayer() = default;
  virtual ~EnhancedObstacleLayer() = default;

  virtual void onInitialize() override;
  virtual void updateBounds(
    double robot_x, double robot_y, double robot_yaw,
    double * min_x, double * min_y, double * max_x, double * max_y) override;
  virtual void updateCosts(
    nav2_costmap_2d::Costmap2D & master_grid,
    int min_i, int min_j, int max_i, int max_j) override;

protected:
  // Enhanced wall detection
  void processLaserScan(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  std::vector<geometry_msgs::msg::Point> detectWallPoints(
    const sensor_msgs::msg::LaserScan::SharedPtr scan);
  void enhanceWallDetection(
    nav2_costmap_2d::Costmap2D & master_grid,
    int min_i, int min_j, int max_i, int max_j);
  double calculateSegmentLinearity(
    const std::vector<std::pair<double, double>> & segment);
  
  // Wall-specific parameters
  double wall_detection_range_;
  double wall_thickness_;
  double wall_confidence_threshold_;
  bool enable_wall_enhancement_;
  
  // Wall data storage
  std::vector<geometry_msgs::msg::Point> detected_walls_;
  std::mutex wall_data_mutex_;
  
private:
  // Enhanced laser scan callback
  void laserScanCallback(const sensor_msgs::msg::LaserScan::SharedPtr scan);
  
  // Subscription for enhanced processing
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr enhanced_laser_sub_;
};

}  // namespace turtle_bot_navigation

#endif  // TURTLE_BOT_NAVIGATION__ENHANCED_OBSTACLE_LAYER_HPP_
