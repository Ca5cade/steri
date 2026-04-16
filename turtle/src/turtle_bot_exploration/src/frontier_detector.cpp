#include "turtle_bot_exploration/frontier_detector.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>
#include <iomanip>

namespace turtle_bot_exploration
{

FrontierDetector::FrontierDetector(const rclcpp::NodeOptions & options)
: Node("frontier_detector", options), map_received_(false)
{
  // Initialize parameters
  this->declare_parameter("min_frontier_size", 10.0);
  this->declare_parameter("max_frontier_distance", 10.0);
  this->declare_parameter("frontier_travel_point_distance", 0.5);
  this->declare_parameter("information_radius", 1.0);
  this->declare_parameter("robot_base_frame", "base_link");
  this->declare_parameter("global_frame", "map");

  min_frontier_size_ = this->get_parameter("min_frontier_size").as_double();
  max_frontier_distance_ = this->get_parameter("max_frontier_distance").as_double();
  frontier_travel_point_distance_ = this->get_parameter("frontier_travel_point_distance").as_double();
  information_radius_ = this->get_parameter("information_radius").as_double();
  robot_base_frame_ = this->get_parameter("robot_base_frame").as_string();
  global_frame_ = this->get_parameter("global_frame").as_string();

  // Initialize TF2
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Initialize subscribers and publishers
  map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
    "/map", rclcpp::QoS(1).transient_local(),
    std::bind(&FrontierDetector::mapCallback, this, std::placeholders::_1));

  frontier_marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
    "/exploration/frontiers", 10);

  frontier_goal_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>(
    "/exploration/frontier_goal", 10);

  // Timer for periodic frontier detection
  timer_ = this->create_wall_timer(
    std::chrono::milliseconds(1000),
    std::bind(&FrontierDetector::timerCallback, this));

  RCLCPP_INFO(this->get_logger(), "Frontier detector initialized");
}

void FrontierDetector::mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  current_map_ = msg;
  map_received_ = true;
  RCLCPP_DEBUG(this->get_logger(), "Received map with size %dx%d", 
               msg->info.width, msg->info.height);
}

void FrontierDetector::timerCallback()
{
  if (!map_received_ || !current_map_) {
    return;
  }

  // Detect frontiers
  auto frontiers = detectFrontiers(*current_map_);
  current_frontiers_ = frontiers;

  // Publish visualization markers
  publishFrontierMarkers(frontiers);

  RCLCPP_DEBUG(this->get_logger(), "Detected %zu frontiers", frontiers.size());
}

std::vector<Frontier> FrontierDetector::detectFrontiers(const nav_msgs::msg::OccupancyGrid & map)
{
  // Convert occupancy grid to OpenCV image
  cv::Mat map_image(map.info.height, map.info.width, CV_8UC1);
  
  for (unsigned int y = 0; y < map.info.height; ++y) {
    for (unsigned int x = 0; x < map.info.width; ++x) {
      int index = y * map.info.width + x;
      int8_t value = map.data[index];
      
      if (value == -1) {
        map_image.at<uint8_t>(y, x) = 128;  // Unknown
      } else if (value == 0) {
        map_image.at<uint8_t>(y, x) = 255;  // Free
      } else {
        map_image.at<uint8_t>(y, x) = 0;    // Occupied
      }
    }
  }

  // Find frontier cells
  auto frontier_cells = findFrontierCells(map_image);
  
  // Group frontier cells into frontiers
  auto frontiers = groupFrontierCells(frontier_cells, map);

  // Filter frontiers by size and distance
  std::vector<Frontier> filtered_frontiers;
  auto robot_pose = getRobotPose();
  
  for (auto & frontier : frontiers) {
    if (frontier.size >= min_frontier_size_) {
      // Calculate distance to robot
      double dx = frontier.centroid.x - robot_pose.pose.position.x;
      double dy = frontier.centroid.y - robot_pose.pose.position.y;
      frontier.distance_to_robot = std::sqrt(dx * dx + dy * dy);
      
      if (frontier.distance_to_robot <= max_frontier_distance_) {
        // Calculate information gain
        frontier.information_gain = calculateInformationGain(frontier, map);
        
        // Check reachability
        frontier.is_reachable = isReachable(frontier.centroid);
        
        if (frontier.is_reachable) {
          filtered_frontiers.push_back(frontier);
        }
      }
    }
  }

  return filtered_frontiers;
}

std::vector<cv::Point> FrontierDetector::findFrontierCells(const cv::Mat & map_image)
{
  std::vector<cv::Point> frontier_cells;
  
  for (int y = 1; y < map_image.rows - 1; ++y) {
    for (int x = 1; x < map_image.cols - 1; ++x) {
      if (isFrontierCell(map_image, x, y)) {
        frontier_cells.emplace_back(x, y);
      }
    }
  }
  
  return frontier_cells;
}

bool FrontierDetector::isFrontierCell(const cv::Mat & map, int x, int y)
{
  // A frontier cell is a free cell that has at least one unknown neighbor
  if (map.at<uint8_t>(y, x) != 255) {  // Not free space
    return false;
  }
  
  return hasUnknownNeighbor(map, x, y);
}

bool FrontierDetector::hasUnknownNeighbor(const cv::Mat & map, int x, int y)
{
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      
      int nx = x + dx;
      int ny = y + dy;
      
      if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows) {
        if (map.at<uint8_t>(ny, nx) == 128) {  // Unknown
          return true;
        }
      }
    }
  }
  return false;
}

bool FrontierDetector::hasFreeNeighbor(const cv::Mat & map, int x, int y)
{
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      
      int nx = x + dx;
      int ny = y + dy;
      
      if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows) {
        if (map.at<uint8_t>(ny, nx) == 255) {  // Free
          return true;
        }
      }
    }
  }
  return false;
}

std::vector<Frontier> FrontierDetector::groupFrontierCells(
  const std::vector<cv::Point> & frontier_cells,
  const nav_msgs::msg::OccupancyGrid & map)
{
  std::vector<Frontier> frontiers;
  std::vector<bool> visited(frontier_cells.size(), false);

  for (size_t i = 0; i < frontier_cells.size(); ++i) {
    if (visited[i]) continue;

    Frontier frontier;
    std::vector<cv::Point> cluster;
    std::queue<size_t> queue;

    queue.push(i);
    visited[i] = true;

    // BFS to find connected frontier cells
    while (!queue.empty()) {
      size_t current = queue.front();
      queue.pop();
      cluster.push_back(frontier_cells[current]);

      // Check neighbors
      for (size_t j = 0; j < frontier_cells.size(); ++j) {
        if (visited[j]) continue;

        int dx = frontier_cells[current].x - frontier_cells[j].x;
        int dy = frontier_cells[current].y - frontier_cells[j].y;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= 2.0) {  // Adjacent cells (including diagonal)
          visited[j] = true;
          queue.push(j);
        }
      }
    }

    if (cluster.size() >= 3) {  // Minimum cluster size
      // Calculate centroid
      double sum_x = 0, sum_y = 0;
      for (const auto & point : cluster) {
        sum_x += point.x;
        sum_y += point.y;
      }

      cv::Point centroid_grid(sum_x / cluster.size(), sum_y / cluster.size());
      frontier.centroid = gridToWorld(centroid_grid.x, centroid_grid.y, map);
      frontier.size = cluster.size();

      // Convert cluster points to world coordinates
      for (const auto & point : cluster) {
        frontier.points.push_back(gridToWorld(point.x, point.y, map));
      }

      frontiers.push_back(frontier);
    }
  }

  return frontiers;
}

geometry_msgs::msg::Point FrontierDetector::gridToWorld(
  int x, int y, const nav_msgs::msg::OccupancyGrid & map)
{
  geometry_msgs::msg::Point world_point;
  world_point.x = map.info.origin.position.x + (x + 0.5) * map.info.resolution;
  world_point.y = map.info.origin.position.y + (y + 0.5) * map.info.resolution;
  world_point.z = 0.0;
  return world_point;
}

cv::Point FrontierDetector::worldToGrid(
  const geometry_msgs::msg::Point & world_point, const nav_msgs::msg::OccupancyGrid & map)
{
  int x = static_cast<int>((world_point.x - map.info.origin.position.x) / map.info.resolution);
  int y = static_cast<int>((world_point.y - map.info.origin.position.y) / map.info.resolution);
  return cv::Point(x, y);
}

double FrontierDetector::calculateInformationGain(
  const Frontier & frontier, const nav_msgs::msg::OccupancyGrid & map)
{
  // Simple information gain calculation based on unknown cells in radius
  cv::Point center_grid = worldToGrid(frontier.centroid, map);
  int radius_cells = static_cast<int>(information_radius_ / map.info.resolution);

  int unknown_count = 0;
  int total_count = 0;

  for (int dy = -radius_cells; dy <= radius_cells; ++dy) {
    for (int dx = -radius_cells; dx <= radius_cells; ++dx) {
      if (dx * dx + dy * dy <= radius_cells * radius_cells) {
        int x = center_grid.x + dx;
        int y = center_grid.y + dy;

        if (x >= 0 && x < static_cast<int>(map.info.width) &&
            y >= 0 && y < static_cast<int>(map.info.height)) {
          int index = y * map.info.width + x;
          if (map.data[index] == -1) {
            unknown_count++;
          }
          total_count++;
        }
      }
    }
  }

  return total_count > 0 ? static_cast<double>(unknown_count) / total_count : 0.0;
}

bool FrontierDetector::isReachable(const geometry_msgs::msg::Point & point)
{
  // Simple reachability check - could be enhanced with path planning
  // For now, just check if the point is not too close to obstacles
  if (!current_map_) return false;

  cv::Point grid_point = worldToGrid(point, *current_map_);

  // Check if point is within map bounds
  if (grid_point.x < 0 || grid_point.x >= static_cast<int>(current_map_->info.width) ||
      grid_point.y < 0 || grid_point.y >= static_cast<int>(current_map_->info.height)) {
    return false;
  }

  // Check if point and surrounding area are free
  int safety_radius = 2;  // cells
  for (int dy = -safety_radius; dy <= safety_radius; ++dy) {
    for (int dx = -safety_radius; dx <= safety_radius; ++dx) {
      int x = grid_point.x + dx;
      int y = grid_point.y + dy;

      if (x >= 0 && x < static_cast<int>(current_map_->info.width) &&
          y >= 0 && y < static_cast<int>(current_map_->info.height)) {
        int index = y * current_map_->info.width + x;
        if (current_map_->data[index] > 50) {  // Occupied threshold
          return false;
        }
      }
    }
  }

  return true;
}

geometry_msgs::msg::PoseStamped FrontierDetector::getRobotPose()
{
  geometry_msgs::msg::PoseStamped robot_pose;
  robot_pose.header.frame_id = global_frame_;
  robot_pose.header.stamp = this->get_clock()->now();

  try {
    auto transform = tf_buffer_->lookupTransform(
      global_frame_, robot_base_frame_, tf2::TimePointZero);

    robot_pose.pose.position.x = transform.transform.translation.x;
    robot_pose.pose.position.y = transform.transform.translation.y;
    robot_pose.pose.position.z = transform.transform.translation.z;
    robot_pose.pose.orientation = transform.transform.rotation;
  } catch (const tf2::TransformException & ex) {
    RCLCPP_WARN(this->get_logger(), "Could not get robot pose: %s", ex.what());
    // Return default pose at origin
    robot_pose.pose.position.x = 0.0;
    robot_pose.pose.position.y = 0.0;
    robot_pose.pose.position.z = 0.0;
    robot_pose.pose.orientation.w = 1.0;
  }

  return robot_pose;
}

void FrontierDetector::publishFrontierMarkers(const std::vector<Frontier> & frontiers)
{
  visualization_msgs::msg::MarkerArray marker_array;

  // Clear previous markers
  visualization_msgs::msg::Marker clear_marker;
  clear_marker.header.frame_id = global_frame_;
  clear_marker.header.stamp = this->get_clock()->now();
  clear_marker.ns = "frontiers";
  clear_marker.action = visualization_msgs::msg::Marker::DELETEALL;
  marker_array.markers.push_back(clear_marker);

  // Add frontier markers
  for (size_t i = 0; i < frontiers.size(); ++i) {
    auto marker = createFrontierMarker(frontiers[i], i, "frontiers");
    marker_array.markers.push_back(marker);

    // Add text marker with information
    visualization_msgs::msg::Marker text_marker = marker;
    text_marker.id = i + 1000;
    text_marker.type = visualization_msgs::msg::Marker::TEXT_VIEW_FACING;
    text_marker.pose.position.z += 0.5;
    text_marker.scale.z = 0.3;
    text_marker.color.r = 1.0;
    text_marker.color.g = 1.0;
    text_marker.color.b = 1.0;
    text_marker.color.a = 1.0;

    std::ostringstream ss;
    ss << "F" << i << "\nSize: " << static_cast<int>(frontiers[i].size)
       << "\nDist: " << std::fixed << std::setprecision(1) << frontiers[i].distance_to_robot
       << "\nGain: " << std::fixed << std::setprecision(2) << frontiers[i].information_gain;
    text_marker.text = ss.str();

    marker_array.markers.push_back(text_marker);
  }

  frontier_marker_pub_->publish(marker_array);
}

visualization_msgs::msg::Marker FrontierDetector::createFrontierMarker(
  const Frontier & frontier, int id, const std::string & ns)
{
  visualization_msgs::msg::Marker marker;
  marker.header.frame_id = global_frame_;
  marker.header.stamp = this->get_clock()->now();
  marker.ns = ns;
  marker.id = id;
  marker.type = visualization_msgs::msg::Marker::SPHERE;
  marker.action = visualization_msgs::msg::Marker::ADD;

  marker.pose.position = frontier.centroid;
  marker.pose.orientation.w = 1.0;

  // Scale based on frontier size
  double scale = std::min(0.5, 0.1 + frontier.size * 0.01);
  marker.scale.x = scale;
  marker.scale.y = scale;
  marker.scale.z = scale;

  // Color based on information gain (green = high gain, red = low gain)
  marker.color.r = 1.0 - frontier.information_gain;
  marker.color.g = frontier.information_gain;
  marker.color.b = 0.0;
  marker.color.a = 0.8;

  marker.lifetime = rclcpp::Duration::from_seconds(2.0);

  return marker;
}

}  // namespace turtle_bot_exploration

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<turtle_bot_exploration::FrontierDetector>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
