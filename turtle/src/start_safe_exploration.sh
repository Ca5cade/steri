#!/bin/bash

# 🛡️ Complete Enhanced Safe Exploration System Launcher
# Single command to start everything with proper sequencing

echo "🚀 COMPLETE ENHANCED SAFE EXPLORATION SYSTEM"
echo "============================================="
echo ""
echo "This will start:"
echo "  1. 🌍 Gazebo simulation environment"
echo "  2. 🧭 Nav2 navigation with enhanced safety (60cm buffer)"
echo "  3. 🗺️  SLAM mapping (creates map while exploring)"
echo "  4. 🎯 RViz visualization"
echo "  5. 🔍 Enhanced frontier detection (safety validated)"
echo "  6. 🤖 Intelligent exploration coordinator"
echo "  7. 🛡️ Multi-stage stuck recovery system"
echo ""
echo "🛡️ SAFETY FEATURES INCLUDED:"
echo "  ✅ 60cm obstacle inflation radius"
echo "  ✅ 80cm frontier safety margin"
echo "  ✅ 5x stronger obstacle avoidance"
echo "  ✅ 3-stage stuck recovery (backup→rotate→advance)"
echo "  ✅ Intelligent goal selection"
echo "  ✅ Enhanced collision prevention"
echo ""

# Source the workspace
source install/setup.bash
echo "✅ Workspace sourced successfully"
echo ""

# Check if we're in the right directory
if [ ! -f "turtle_bot_exploration/launch/complete_safe_exploration.launch.py" ]; then
    echo "❌ Error: Launch file not found!"
    echo "   Please run this script from the workspace root directory"
    echo "   Expected: ~/turtle/src/"
    exit 1
fi

echo "🔧 Starting complete safe exploration system..."
echo "   (This will take about 30 seconds to fully initialize)"
echo ""
echo "📊 STARTUP SEQUENCE:"
echo "   0s  - Gazebo simulation starts"
echo "   8s  - Nav2 navigation stack starts"
echo "   10s - SLAM mapping starts"
echo "   12s - RViz visualization opens"
echo "   15s - Enhanced frontier detection starts"
echo "   18s - Exploration coordinator starts"
echo "   20s - Safety verification runs"
echo "   25s - System ready message"
echo ""

# Launch the complete system
echo "🚀 Launching complete safe exploration system..."
ros2 launch turtle_bot_exploration complete_safe_exploration.launch.py

echo ""
echo "🏁 Safe exploration system has been stopped."
echo "   All components have been shut down safely."
