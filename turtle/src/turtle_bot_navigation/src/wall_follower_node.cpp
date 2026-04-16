#include "turtle_bot_navigation/wall_follower.hpp"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<turtle_bot_navigation::WallFollower>();
  
  RCLCPP_INFO(node->get_logger(), "Wall Follower Node started");
  
  rclcpp::spin(node);
  
  rclcpp::shutdown();
  return 0;
}
