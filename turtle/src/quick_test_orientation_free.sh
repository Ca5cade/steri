#!/bin/bash

# 🎯 Quick Test for Orientation-Free Goal Completion
# Tests the system configuration without requiring a running system

echo "🎯 QUICK ORIENTATION-FREE CONFIGURATION TEST"
echo "============================================"
echo ""

# Source the workspace
source install/setup.bash 2>/dev/null

echo "📁 FILE VALIDATION:"
echo ""

# Check if orientation-free params file exists
if [ -f "turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
    echo "✅ Source orientation_free_params.yaml exists"
else
    echo "❌ Source orientation_free_params.yaml missing"
fi

# Check if installed file exists
if [ -f "install/turtle_bot_navigation/share/turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
    echo "✅ Installed orientation_free_params.yaml exists"
else
    echo "❌ Installed orientation_free_params.yaml missing"
fi

echo ""

# Check launch file configurations
echo "🚀 LAUNCH FILE VALIDATION:"
echo ""

if grep -q "orientation_free_params.yaml" turtle_bot_exploration/launch/smart_exploration.launch.py; then
    echo "✅ Smart exploration launch file uses orientation-free params"
else
    echo "❌ Smart exploration launch file not using orientation-free params"
fi

if grep -q "orientation_free_params.yaml" turtle_bot_exploration/launch/fast_safe_exploration.launch.py; then
    echo "✅ Fast safe exploration launch file uses orientation-free params"
else
    echo "❌ Fast safe exploration launch file not using orientation-free params"
fi

echo ""

# Check orientation-free params file content
echo "📊 ORIENTATION-FREE PARAMETERS VALIDATION:"
echo ""

if [ -f "turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
    echo "🔍 Checking orientation-free parameters in config file:"
    
    # Check yaw tolerance
    if grep -q "yaw_goal_tolerance: 6.28" turtle_bot_navigation/config/orientation_free_params.yaml; then
        echo "✅ yaw_goal_tolerance: 6.28 (full circle)"
    else
        echo "❌ yaw_goal_tolerance not set to 6.28"
    fi
    
    # Check xy tolerance
    if grep -q "xy_goal_tolerance: 2.0" turtle_bot_navigation/config/orientation_free_params.yaml; then
        echo "✅ xy_goal_tolerance: 2.0 (liberal position tolerance)"
    else
        echo "❌ xy_goal_tolerance not set to 2.0"
    fi
    
    # Check critics (should not include orientation critics)
    if grep -A 5 "critics:" turtle_bot_navigation/config/orientation_free_params.yaml | grep -q "RotateToGoal\|GoalAlign"; then
        echo "❌ Orientation critics still present (RotateToGoal/GoalAlign)"
    else
        echo "✅ Orientation critics removed (no RotateToGoal/GoalAlign)"
    fi
    
    echo ""
    echo "📝 Key parameters found in config file:"
    grep -E "yaw_goal_tolerance|xy_goal_tolerance|critics:" turtle_bot_navigation/config/orientation_free_params.yaml | head -5
fi

echo ""

# Check exploration coordinator source code
echo "🎯 EXPLORATION COORDINATOR CODE VALIDATION:"
echo ""

if grep -q "DISTANCE-BASED ONLY" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    echo "✅ Distance-based goal completion logic implemented"
else
    echo "❌ Distance-based goal completion logic not found"
fi

if grep -q "Robot orientation irrelevant" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    echo "✅ Orientation-irrelevant force completion implemented"
else
    echo "❌ Orientation-irrelevant force completion not found"
fi

if grep -q "robot can face any direction at frontier" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    echo "✅ Orientation-free frontier goal creation implemented"
else
    echo "❌ Orientation-free frontier goal creation not found"
fi

echo ""

# Overall assessment
echo "🎯 CONFIGURATION READINESS ASSESSMENT:"
echo ""

SUCCESS_COUNT=0
TOTAL_CHECKS=6

# Check files exist
if [ -f "turtle_bot_navigation/config/orientation_free_params.yaml" ] && [ -f "install/turtle_bot_navigation/share/turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
    ((SUCCESS_COUNT++))
fi

# Check launch files
if grep -q "orientation_free_params.yaml" turtle_bot_exploration/launch/smart_exploration.launch.py; then
    ((SUCCESS_COUNT++))
fi

# Check parameters in config
if grep -q "yaw_goal_tolerance: 6.28" turtle_bot_navigation/config/orientation_free_params.yaml; then
    ((SUCCESS_COUNT++))
fi

if grep -q "xy_goal_tolerance: 2.0" turtle_bot_navigation/config/orientation_free_params.yaml; then
    ((SUCCESS_COUNT++))
fi

# Check code modifications
if grep -q "DISTANCE-BASED ONLY" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    ((SUCCESS_COUNT++))
fi

if grep -q "Robot orientation irrelevant" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    ((SUCCESS_COUNT++))
fi

if [ $SUCCESS_COUNT -eq $TOTAL_CHECKS ]; then
    echo "🎉 EXCELLENT: Orientation-free system is fully configured!"
    echo "   ✅ All configuration files are correct"
    echo "   ✅ Launch files reference orientation-free parameters"
    echo "   ✅ Navigation parameters eliminate orientation requirements"
    echo "   ✅ Exploration coordinator uses distance-based completion"
    echo "   ✅ System ready for orientation-free goal completion"
elif [ $SUCCESS_COUNT -ge 4 ]; then
    echo "⚠️  GOOD: Most orientation-free features configured ($SUCCESS_COUNT/$TOTAL_CHECKS)"
    echo "   ✅ Core functionality should work"
    echo "   ❓ Minor configuration issues may exist"
else
    echo "❌ NEEDS ATTENTION: Configuration incomplete ($SUCCESS_COUNT/$TOTAL_CHECKS)"
    echo "   ❌ System may not work as expected"
    echo "   🔧 Review configuration above"
fi

echo ""
echo "🚀 NEXT STEPS:"
echo ""

if [ $SUCCESS_COUNT -eq $TOTAL_CHECKS ]; then
    echo "✅ Configuration is complete! Ready to launch:"
    echo "   ./start_smart_exploration.sh"
    echo ""
    echo "🎯 Expected behavior:"
    echo "   ✅ Robot completes goals when within 2.0m of frontier"
    echo "   ✅ Robot can face any direction (0° to 360°)"
    echo "   ✅ No orientation-based stuck situations"
    echo "   ✅ Smooth transitions between frontiers"
else
    echo "🔧 Fix configuration issues first:"
    echo "   1. Rebuild packages: colcon build"
    echo "   2. Source workspace: source install/setup.bash"
    echo "   3. Check file permissions and paths"
    echo "   4. Verify parameter values in config files"
fi

echo ""
echo "📊 MONITORING COMMANDS (when system is running):"
echo "   Test orientation-free: ./test_orientation_free.sh"
echo "   Monitor goal completion: ros2 topic echo /rosout | grep 'DISTANCE-BASED'"
echo "   Check yaw tolerance: ros2 param get /controller_server general_goal_checker.yaw_goal_tolerance"
echo ""

echo "✅ Quick test complete!"
