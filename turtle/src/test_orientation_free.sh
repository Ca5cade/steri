#!/bin/bash

# 🎯 Orientation-Free Goal Completion Test Script
# Validates that the system eliminates orientation requirements

echo "🎯 ORIENTATION-FREE GOAL COMPLETION VALIDATION"
echo "=============================================="
echo ""

# Source the workspace
source install/setup.bash 2>/dev/null

# Function to check parameter value
check_param() {
    local node=$1
    local param=$2
    local expected=$3
    local description=$4
    
    local actual=$(ros2 param get "$node" "$param" 2>/dev/null | grep -o '[0-9.]*' | head -1)
    
    if [ -n "$actual" ]; then
        if [ "$actual" = "$expected" ]; then
            echo "✅ $description: $actual (CORRECT)"
            return 0
        else
            echo "❌ $description: $actual (expected: $expected)"
            return 1
        fi
    else
        echo "❓ $description: NOT AVAILABLE"
        return 2
    fi
}

# Function to check if critics list excludes orientation critics
check_critics() {
    local critics=$(ros2 param get /controller_server FollowPath.critics 2>/dev/null)
    
    if echo "$critics" | grep -q "RotateToGoal\|GoalAlign"; then
        echo "❌ Orientation critics still present: $critics"
        return 1
    else
        echo "✅ Orientation critics removed: No RotateToGoal or GoalAlign"
        return 0
    fi
}

# Function to test goal completion behavior
test_goal_completion() {
    echo "🔍 Testing goal completion behavior..."
    
    # Monitor goal completion messages for 30 seconds
    timeout 30s ros2 topic echo /rosout --once 2>/dev/null | grep -i "distance-based\|orientation" | head -5 | while read line; do
        echo "📝 $line"
    done
}

echo "🔧 SYSTEM VALIDATION:"
echo ""

# Check if system is running
if ! ros2 node list 2>/dev/null | grep -q "controller_server"; then
    echo "❌ Navigation system not running!"
    echo ""
    echo "🚀 Start the system first:"
    echo "   ./start_smart_exploration.sh"
    echo ""
    exit 1
fi

echo "✅ Navigation system is running"
echo ""

echo "📊 ORIENTATION-FREE PARAMETER VALIDATION:"
echo ""

# Check exploration coordinator parameters
echo "🎯 Exploration Coordinator Parameters:"
check_param "/exploration_coordinator" "goal_reached_tolerance" "2.0" "Goal reached tolerance"
check_param "/exploration_coordinator" "goal_timeout" "30.0" "Goal timeout"
check_param "/exploration_coordinator" "force_new_goal_distance" "2.5" "Force completion distance"

echo ""

# Check navigation controller parameters
echo "🧭 Navigation Controller Parameters:"
check_param "/controller_server" "general_goal_checker.xy_goal_tolerance" "2.0" "Position tolerance"

# Check yaw tolerance (should be 6.28 for full circle)
yaw_tolerance=$(ros2 param get /controller_server general_goal_checker.yaw_goal_tolerance 2>/dev/null | grep -o '[0-9.]*' | head -1)
if [ -n "$yaw_tolerance" ]; then
    if (( $(echo "$yaw_tolerance >= 6.0" | bc -l) )); then
        echo "✅ Yaw goal tolerance: $yaw_tolerance (ORIENTATION-FREE)"
    else
        echo "❌ Yaw goal tolerance: $yaw_tolerance (still has orientation requirement)"
    fi
else
    echo "❓ Yaw goal tolerance: NOT AVAILABLE"
fi

echo ""

# Check DWB controller parameters
echo "🎮 DWB Controller Parameters:"
check_param "/controller_server" "FollowPath.xy_goal_tolerance" "2.0" "DWB position tolerance"

dwb_yaw_tolerance=$(ros2 param get /controller_server FollowPath.yaw_goal_tolerance 2>/dev/null | grep -o '[0-9.]*' | head -1)
if [ -n "$dwb_yaw_tolerance" ]; then
    if (( $(echo "$dwb_yaw_tolerance >= 6.0" | bc -l) )); then
        echo "✅ DWB yaw tolerance: $dwb_yaw_tolerance (ORIENTATION-FREE)"
    else
        echo "❌ DWB yaw tolerance: $dwb_yaw_tolerance (still has orientation requirement)"
    fi
else
    echo "❓ DWB yaw tolerance: NOT AVAILABLE"
fi

echo ""

# Check critics configuration
echo "🎯 Navigation Critics Configuration:"
check_critics

echo ""

# Check if orientation-free params file is being used
echo "📁 CONFIGURATION FILE VALIDATION:"
echo ""

if [ -f "turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
    echo "✅ Orientation-free params file exists"
    
    # Check if launch files reference the correct params
    if grep -q "orientation_free_params.yaml" turtle_bot_exploration/launch/smart_exploration.launch.py; then
        echo "✅ Smart exploration uses orientation-free params"
    else
        echo "❌ Smart exploration not using orientation-free params"
    fi
    
    if grep -q "orientation_free_params.yaml" turtle_bot_exploration/launch/fast_safe_exploration.launch.py; then
        echo "✅ Fast safe exploration uses orientation-free params"
    else
        echo "❌ Fast safe exploration not using orientation-free params"
    fi
else
    echo "❌ Orientation-free params file missing"
fi

echo ""

# Test goal completion behavior
echo "🧪 GOAL COMPLETION BEHAVIOR TEST:"
echo ""

if ros2 node list 2>/dev/null | grep -q "exploration_coordinator"; then
    echo "🔍 Monitoring goal completion messages..."
    echo "   Looking for distance-based completion logs..."
    echo ""
    
    # Check recent logs for orientation-free completion
    recent_logs=$(ros2 topic echo /rosout --once 2>/dev/null | grep -i "distance-based\|orientation.*irrelevant" | head -3)
    
    if [ -n "$recent_logs" ]; then
        echo "✅ Found orientation-free completion logs:"
        echo "$recent_logs" | while read line; do
            echo "   📝 $line"
        done
    else
        echo "❓ No recent orientation-free completion logs found"
        echo "   This is normal if no goals have completed recently"
    fi
else
    echo "❓ Exploration coordinator not running - cannot test goal completion"
fi

echo ""

# Overall assessment
echo "🎯 ORIENTATION-FREE READINESS ASSESSMENT:"
echo ""

# Count successful validations
SUCCESS_COUNT=0
TOTAL_CHECKS=6

# Check key parameters
if ros2 param get /exploration_coordinator goal_reached_tolerance 2>/dev/null | grep -q "2.0"; then
    ((SUCCESS_COUNT++))
fi

if ros2 param get /controller_server general_goal_checker.xy_goal_tolerance 2>/dev/null | grep -q "2.0"; then
    ((SUCCESS_COUNT++))
fi

yaw_val=$(ros2 param get /controller_server general_goal_checker.yaw_goal_tolerance 2>/dev/null | grep -o '[0-9.]*' | head -1)
if [ -n "$yaw_val" ] && (( $(echo "$yaw_val >= 6.0" | bc -l) )); then
    ((SUCCESS_COUNT++))
fi

if ! ros2 param get /controller_server FollowPath.critics 2>/dev/null | grep -q "RotateToGoal\|GoalAlign"; then
    ((SUCCESS_COUNT++))
fi

if [ -f "turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
    ((SUCCESS_COUNT++))
fi

if grep -q "orientation_free_params.yaml" turtle_bot_exploration/launch/smart_exploration.launch.py; then
    ((SUCCESS_COUNT++))
fi

# Assessment based on success rate
if [ $SUCCESS_COUNT -eq $TOTAL_CHECKS ]; then
    echo "🎉 EXCELLENT: Orientation-free goal completion is fully configured!"
    echo "   ✅ All orientation requirements eliminated"
    echo "   ✅ Robot can face any direction at frontiers"
    echo "   ✅ Goal completion is purely distance-based"
    echo "   ✅ Navigation system configured correctly"
    echo "   ✅ Launch files use orientation-free parameters"
elif [ $SUCCESS_COUNT -ge 4 ]; then
    echo "⚠️  GOOD: Most orientation-free features configured ($SUCCESS_COUNT/$TOTAL_CHECKS)"
    echo "   ✅ Core functionality should work"
    echo "   ❓ Some parameters may need adjustment"
elif [ $SUCCESS_COUNT -ge 2 ]; then
    echo "⚠️  PARTIAL: Some orientation-free features configured ($SUCCESS_COUNT/$TOTAL_CHECKS)"
    echo "   ❓ System may still have orientation requirements"
    echo "   🔧 Review parameters above"
else
    echo "❌ NEEDS ATTENTION: Orientation requirements still present ($SUCCESS_COUNT/$TOTAL_CHECKS)"
    echo "   ❌ System likely still enforces orientation constraints"
    echo "   🔧 Configuration needs updates"
fi

echo ""
echo "🛠️  TROUBLESHOOTING RECOMMENDATIONS:"
echo ""

if [ $SUCCESS_COUNT -lt $TOTAL_CHECKS ]; then
    echo "🔧 Configuration Issues:"
    echo "   1. Rebuild system: colcon build --packages-select turtle_bot_exploration"
    echo "   2. Restart system: ./start_smart_exploration.sh"
    echo "   3. Check parameter files are correctly referenced"
    echo ""
fi

echo "📊 Monitoring Commands:"
echo "   Watch goal completion: ros2 topic echo /rosout | grep 'DISTANCE-BASED'"
echo "   Check yaw tolerance: ros2 param get /controller_server general_goal_checker.yaw_goal_tolerance"
echo "   Monitor exploration: ros2 topic echo /exploration_status"
echo ""

echo "🎯 Expected Behavior:"
echo "   ✅ Goals complete when robot within 2.0m of frontier"
echo "   ✅ Robot can face any direction (0° to 360°)"
echo "   ✅ No rotation delays at goal completion"
echo "   ✅ Smooth transitions between frontiers"
echo ""

echo "✅ Validation complete!"
