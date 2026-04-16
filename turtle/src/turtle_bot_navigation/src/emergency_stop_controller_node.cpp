#include "turtle_bot_navigation/emergency_stop_controller.hpp"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<turtle_bot_navigation::EmergencyStopController>();
  
  RCLCPP_INFO(node->get_logger(), "Emergency Stop Controller Node started");
  
  rclcpp::spin(node);
  
  rclcpp::shutdown();
  return 0;
}
