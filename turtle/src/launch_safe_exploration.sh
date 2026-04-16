#!/bin/bash

# 🛡️ Enhanced Safe Exploration System - XML Launch
# Uses traditional ROS2 XML launch file format

echo "🚀 ENHANCED SAFE EXPLORATION SYSTEM (XML Launch)"
echo "================================================="
echo ""
echo "🛡️ SAFETY FEATURES:"
echo "  ✅ 60cm obstacle inflation radius"
echo "  ✅ 80cm frontier safety margin"
echo "  ✅ 5x stronger obstacle avoidance"
echo "  ✅ Multi-stage stuck recovery"
echo "  ✅ Intelligent goal selection"
echo "  ✅ SLAM mapping enabled"
echo ""

# Source the workspace
source install/setup.bash
echo "✅ Workspace sourced"

# Check if we're in the right directory
if [ ! -f "turtle_bot_exploration/launch/safe_exploration.launch" ]; then
    echo "❌ Error: XML launch file not found!"
    echo "   Please run this script from the workspace root directory"
    echo "   Expected: ~/turtle/src/"
    exit 1
fi

echo "🔧 Starting enhanced safe exploration system..."
echo "   Using XML launch file format"
echo ""

# Launch using XML launch file
echo "🚀 Launching with XML launch file..."
ros2 launch turtle_bot_exploration safe_exploration.launch

echo ""
echo "🏁 Safe exploration system has been stopped."
echo "   All components have been shut down safely."
