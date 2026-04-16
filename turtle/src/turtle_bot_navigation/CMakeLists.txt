cmake_minimum_required(VERSION 3.8)
project(turtle_bot_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp_components REQUIRED)

# Include directories
include_directories(include)

# Wall Follower Node
add_executable(wall_follower_node
  src/wall_follower_node.cpp
  src/wall_follower.cpp
)

ament_target_dependencies(wall_follower_node
  rclcpp
  sensor_msgs
  geometry_msgs
  nav_msgs
  visualization_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
)

# Emergency Stop Controller Node
add_executable(emergency_stop_controller_node
  src/emergency_stop_controller_node.cpp
  src/emergency_stop_controller.cpp
)

ament_target_dependencies(emergency_stop_controller_node
  rclcpp
  sensor_msgs
  geometry_msgs
  nav_msgs
  std_msgs
  tf2
  tf2_ros
)

# Enhanced Obstacle Layer Plugin
add_library(enhanced_obstacle_layer SHARED
  src/enhanced_obstacle_layer.cpp
)

ament_target_dependencies(enhanced_obstacle_layer
  rclcpp
  nav2_costmap_2d
  sensor_msgs
  geometry_msgs
  pluginlib
)

# Install targets
install(TARGETS
  wall_follower_node
  emergency_stop_controller_node
  enhanced_obstacle_layer
  DESTINATION lib/${PROJECT_NAME}
)

# Install directories
install(
  DIRECTORY config launch maps behavior_trees include/
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
