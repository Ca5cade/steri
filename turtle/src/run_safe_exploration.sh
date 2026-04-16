#!/bin/bash

# 🛡️ Enhanced Safe Exploration Runner
# This script runs the enhanced exploration with safety improvements

echo "🚀 Enhanced Safe Exploration System"
echo "=================================="
echo ""

# Source the workspace
source install/setup.bash
echo "✅ Workspace sourced"

# Check if Gazebo is running
if ! pgrep -x "gzserver" > /dev/null; then
    echo "⚠️  Gazebo not detected. Please start Gazebo first:"
    echo "   ros2 launch turtle_bot_gazebo turtle_bot_world.launch.py"
    echo ""
    read -p "Press Enter when Gazebo is running..."
fi

# Check if navigation is running
if ! ros2 node list 2>/dev/null | grep -q "controller_server"; then
    echo "⚠️  Navigation not detected. Please start navigation first:"
    echo "   ros2 launch turtle_bot_navigation turtle_bot_navigation.launch.py"
    echo ""
    read -p "Press Enter when navigation is running..."
fi

echo ""
echo "🛡️ Safety Features Active:"
echo "   • 60cm obstacle inflation radius"
echo "   • 80cm frontier safety margin" 
echo "   • Multi-stage stuck recovery"
echo "   • Enhanced obstacle avoidance"
echo "   • Intelligent goal selection"
echo ""

echo "🔧 Starting enhanced exploration nodes..."

# Start frontier detection with safety parameters
echo "Starting frontier detection..."
ros2 run turtle_bot_exploration frontier_detection_node \
    --ros-args \
    -p use_sim_time:=true \
    -p frontier_min_size:=15 \
    -p frontier_travel_point_distance:=0.8 \
    -p frontier_blacklist_radius:=1.2 \
    -p frontier_search_radius:=8.0 \
    -p frontier_min_distance:=1.0 &

FRONTIER_PID=$!

# Wait a moment for frontier detection to start
sleep 3

# Start exploration coordinator with enhanced safety
echo "Starting exploration coordinator with safety features..."
ros2 run turtle_bot_exploration exploration_coordinator_node \
    --ros-args \
    -p use_sim_time:=true \
    -p exploration_timeout:=1800.0 \
    -p stuck_timeout:=8.0 \
    -p recovery_rotation_speed:=0.5 \
    -p recovery_rotation_duration:=12.0 \
    -p auto_start_exploration:=true \
    -p global_frame:=map \
    -p robot_base_frame:=base_link \
    -p min_frontier_distance:=1.0 \
    -p max_exploration_range:=15.0 \
    -p goal_tolerance:=0.3

echo ""
echo "🏁 Exploration completed or stopped!"
echo "Cleaning up processes..."

# Clean up background processes
kill $FRONTIER_PID 2>/dev/null

echo "✅ Safe exploration session ended"
