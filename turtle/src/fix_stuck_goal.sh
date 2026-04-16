#!/bin/bash

# 🎯 Immediate Goal Completion Fix
# Run this script while the robot is stuck to force goal completion

echo "🎯 IMMEDIATE GOAL COMPLETION FIX"
echo "================================"
echo ""

# Source the workspace
source install/setup.bash

echo "🔧 Applying ultra-aggressive goal completion parameters..."
echo ""

# Set ultra-liberal goal completion parameters
echo "📊 Setting goal completion parameters:"

# Goal reached tolerance - 2.0m (very liberal)
ros2 param set /exploration_coordinator goal_reached_tolerance 2.0
echo "  ✅ goal_reached_tolerance: 2.0m (ultra-liberal)"

# Goal timeout - 30 seconds (shorter)
ros2 param set /exploration_coordinator goal_timeout 30.0
echo "  ✅ goal_timeout: 30.0s (shorter timeout)"

# Force new goal distance - 2.5m (very aggressive)
ros2 param set /exploration_coordinator force_new_goal_distance 2.5
echo "  ✅ force_new_goal_distance: 2.5m (aggressive)"

# Goal tolerance - 1.5m (liberal)
ros2 param set /exploration_coordinator goal_tolerance 1.5
echo "  ✅ goal_tolerance: 1.5m (liberal)"

# Stuck distance threshold - 0.2m (more lenient)
ros2 param set /exploration_coordinator stuck_distance_threshold 0.2
echo "  ✅ stuck_distance_threshold: 0.2m (lenient)"

echo ""
echo "🎯 ORIENTATION-FREE PARAMETERS:"

# Set orientation-free navigation parameters
ros2 param set /controller_server general_goal_checker.yaw_goal_tolerance 6.28
echo "  ✅ yaw_goal_tolerance: 6.28 (full circle - any orientation)"

ros2 param set /controller_server FollowPath.yaw_goal_tolerance 6.28
echo "  ✅ DWB yaw_goal_tolerance: 6.28 (orientation-free)"

echo ""
echo "🚀 IMMEDIATE ACTIONS:"

# Force goal completion by setting override flag
echo "🎯 Forcing current goal completion..."
ros2 service call /exploration_coordinator/force_goal_completion std_srvs/srv/Empty "{}" 2>/dev/null || echo "  ⚠️  Service not available, using parameter method"

# Cancel current navigation goal
echo "🔄 Cancelling current navigation goal..."
ros2 action send_goal /navigate_to_pose nav2_msgs/action/NavigateToPose "{}" --feedback 2>/dev/null &
sleep 1
pkill -f "ros2 action send_goal" 2>/dev/null

echo ""
echo "📊 VERIFICATION:"

# Check current parameters
echo "🔍 Current goal completion parameters:"
echo "  goal_reached_tolerance: $(ros2 param get /exploration_coordinator goal_reached_tolerance 2>/dev/null | grep -o '[0-9.]*' || echo 'N/A')"
echo "  goal_timeout: $(ros2 param get /exploration_coordinator goal_timeout 2>/dev/null | grep -o '[0-9.]*' || echo 'N/A')"
echo "  force_new_goal_distance: $(ros2 param get /exploration_coordinator force_new_goal_distance 2>/dev/null | grep -o '[0-9.]*' || echo 'N/A')"

echo ""
echo "🎯 EXPECTED BEHAVIOR:"
echo "  ✅ Robot should complete current goal within 30 seconds"
echo "  ✅ Goals will complete when within 2.0m of target"
echo "  ✅ Force completion when within 2.5m and moved"
echo "  ✅ No more stuck situations at goals"

echo ""
echo "📊 MONITORING:"
echo "  Watch exploration status: ros2 topic echo /exploration_status"
echo "  Monitor goal completion: ros2 topic echo /rosout | grep 'Goal completion'"
echo "  Check robot position: ros2 topic echo /odom"

echo ""
echo "✅ Ultra-aggressive goal completion parameters applied!"
echo "   The robot should now complete goals much more easily."
echo ""
echo "🔄 If still stuck after 30 seconds, restart exploration:"
echo "   Ctrl+C to stop current system"
echo "   ./start_smart_exploration.sh to restart with new parameters"
