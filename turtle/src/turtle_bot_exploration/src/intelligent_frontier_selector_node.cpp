#include "turtle_bot_exploration/intelligent_frontier_selector.hpp"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<turtle_bot_exploration::IntelligentFrontierSelector>();
  
  RCLCPP_INFO(node->get_logger(), "Intelligent Frontier Selector Node started");
  
  rclcpp::spin(node);
  
  rclcpp::shutdown();
  return 0;
}
