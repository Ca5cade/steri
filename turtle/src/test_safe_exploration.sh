#!/bin/bash

# 🛡️ Enhanced Safe Exploration Test Script
# This script launches the improved collision avoidance system

echo "🚀 Starting Enhanced Safe Exploration System..."
echo "================================================"

# Source the workspace
source install/setup.bash

echo "✅ Workspace sourced successfully"

# Check if Gazebo is running
if ! pgrep -x "gzserver" > /dev/null; then
    echo "⚠️  Gazebo not detected. Please start Gazebo first:"
    echo "   ros2 launch turtle_bot_gazebo turtle_bot_world.launch.py"
    echo ""
    read -p "Press Enter when Gazebo is running..."
fi

echo "🛡️ Launching Enhanced Safe Exploration with:"
echo "   • 60cm safety buffer around obstacles"
echo "   • Intelligent frontier validation"
echo "   • Multi-stage stuck recovery"
echo "   • Enhanced obstacle avoidance"
echo ""

# Launch the enhanced safe exploration with existing navigation
echo "🔧 Starting enhanced exploration with safety features..."
echo "   Note: Make sure navigation is already running!"
echo ""

# Check if navigation is running
if ! ros2 node list | grep -q "controller_server"; then
    echo "⚠️  Navigation stack not detected. Starting it first..."
    echo "🔧 Launching navigation stack..."
    ros2 launch turtle_bot_navigation turtle_bot_navigation.launch.py &
    NAV_PID=$!

    echo "⏳ Waiting for navigation to initialize..."
    sleep 15
    echo "✅ Navigation should be ready"
fi

# Launch just the exploration nodes
echo "🔧 Starting exploration nodes with enhanced safety..."
ros2 launch turtle_bot_exploration exploration.launch.py

echo "🏁 Safe exploration session completed!"
