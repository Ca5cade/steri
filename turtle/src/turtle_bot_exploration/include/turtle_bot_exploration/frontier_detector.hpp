#ifndef TURTLE_BOT_EXPLORATION__FRONTIER_DETECTOR_HPP_
#define TURTLE_BOT_EXPLORATION__FRONTIER_DETECTOR_HPP_

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>

namespace turtle_bot_exploration
{

struct Frontier
{
  geometry_msgs::msg::Point centroid;
  std::vector<geometry_msgs::msg::Point> points;
  double size;
  double distance_to_robot;
  double information_gain;
  bool is_reachable;
};

class FrontierDetector : public rclcpp::Node
{
public:
  explicit FrontierDetector(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  // Callback functions
  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void timerCallback();

  // Core frontier detection functions
  std::vector<Frontier> detectFrontiers(const nav_msgs::msg::OccupancyGrid & map);
  std::vector<cv::Point> findFrontierCells(const cv::Mat & map_image);
  std::vector<Frontier> groupFrontierCells(
    const std::vector<cv::Point> & frontier_cells,
    const nav_msgs::msg::OccupancyGrid & map);
  
  // Utility functions
  bool isFrontierCell(const cv::Mat & map, int x, int y);
  bool hasUnknownNeighbor(const cv::Mat & map, int x, int y);
  bool hasFreeNeighbor(const cv::Mat & map, int x, int y);
  geometry_msgs::msg::Point gridToWorld(int x, int y, const nav_msgs::msg::OccupancyGrid & map);
  cv::Point worldToGrid(const geometry_msgs::msg::Point & world_point, const nav_msgs::msg::OccupancyGrid & map);
  double calculateInformationGain(const Frontier & frontier, const nav_msgs::msg::OccupancyGrid & map);
  bool isReachable(const geometry_msgs::msg::Point & point);
  geometry_msgs::msg::PoseStamped getRobotPose();

  // Visualization functions
  void publishFrontierMarkers(const std::vector<Frontier> & frontiers);
  visualization_msgs::msg::Marker createFrontierMarker(
    const Frontier & frontier, int id, const std::string & ns);

  // ROS2 components
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr frontier_marker_pub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr frontier_goal_pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  // TF2 components
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  // Parameters
  double min_frontier_size_;
  double max_frontier_distance_;
  double frontier_travel_point_distance_;
  double information_radius_;
  std::string robot_base_frame_;
  std::string global_frame_;

  // State variables
  nav_msgs::msg::OccupancyGrid::SharedPtr current_map_;
  std::vector<Frontier> current_frontiers_;
  bool map_received_;
};

}  // namespace turtle_bot_exploration

#endif  // TURTLE_BOT_EXPLORATION__FRONTIER_DETECTOR_HPP_
