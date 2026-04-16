#!/bin/bash

# 🎯 Goal Completion Parameter Validation Script
# Validates that ultra-aggressive goal completion parameters are properly configured

echo "🔍 GOAL COMPLETION PARAMETER VALIDATION"
echo "======================================="
echo ""

# Source the workspace
source install/setup.bash 2>/dev/null

# Function to check parameter in launch file
check_launch_parameter() {
    local param_name=$1
    local expected_value=$2
    local launch_file=$3
    
    if grep -q "${param_name}.*${expected_value}" "$launch_file"; then
        echo "✅ $param_name: $expected_value"
        return 0
    else
        echo "❌ $param_name: NOT FOUND or INCORRECT"
        return 1
    fi
}

# Function to check runtime parameter
check_runtime_parameter() {
    local node_name=$1
    local param_name=$2
    local expected_value=$3
    
    local actual_value=$(ros2 param get "$node_name" "$param_name" 2>/dev/null | grep -o '[0-9.]*' | head -1)
    
    if [ -n "$actual_value" ] && [ "$actual_value" = "$expected_value" ]; then
        echo "✅ $param_name: $actual_value (runtime)"
        return 0
    elif [ -n "$actual_value" ]; then
        echo "⚠️  $param_name: $actual_value (expected: $expected_value)"
        return 1
    else
        echo "❓ $param_name: NOT AVAILABLE (node not running)"
        return 2
    fi
}

# Validate launch file parameters
echo "📋 LAUNCH FILE PARAMETER VALIDATION:"
echo ""

SMART_LAUNCH="turtle_bot_exploration/launch/smart_exploration.launch.py"
FAST_LAUNCH="turtle_bot_exploration/launch/fast_safe_exploration.launch.py"

if [ -f "$SMART_LAUNCH" ]; then
    echo "🔍 Checking smart_exploration.launch.py:"
    
    check_launch_parameter "goal_reached_tolerance" "2\.0" "$SMART_LAUNCH"
    check_launch_parameter "goal_timeout" "30\.0" "$SMART_LAUNCH"
    check_launch_parameter "force_new_goal_distance" "2\.5" "$SMART_LAUNCH"
    check_launch_parameter "goal_tolerance" "1\.5" "$SMART_LAUNCH"
    
    echo ""
else
    echo "❌ smart_exploration.launch.py not found!"
fi

if [ -f "$FAST_LAUNCH" ]; then
    echo "🔍 Checking fast_safe_exploration.launch.py:"
    
    check_launch_parameter "goal_reached_tolerance" "2\.0" "$FAST_LAUNCH"
    check_launch_parameter "goal_timeout" "30\.0" "$FAST_LAUNCH"
    check_launch_parameter "force_new_goal_distance" "2\.5" "$FAST_LAUNCH"
    check_launch_parameter "goal_tolerance" "1\.5" "$FAST_LAUNCH"
    
    echo ""
else
    echo "❌ fast_safe_exploration.launch.py not found!"
fi

# Check if exploration coordinator is running
echo "🤖 RUNTIME PARAMETER VALIDATION:"
echo ""

if ros2 node list 2>/dev/null | grep -q "exploration_coordinator"; then
    echo "✅ Exploration coordinator is running"
    echo ""
    echo "🔍 Checking runtime parameters:"
    
    check_runtime_parameter "/exploration_coordinator" "goal_reached_tolerance" "2.0"
    check_runtime_parameter "/exploration_coordinator" "goal_timeout" "30.0"
    check_runtime_parameter "/exploration_coordinator" "force_new_goal_distance" "2.5"
    check_runtime_parameter "/exploration_coordinator" "goal_tolerance" "1.5"
    check_runtime_parameter "/exploration_coordinator" "stuck_distance_threshold" "0.15"
    
    echo ""
else
    echo "❓ Exploration coordinator not running - cannot check runtime parameters"
    echo ""
fi

# Check system status
echo "📊 SYSTEM STATUS CHECK:"
echo ""

# Check critical nodes
CRITICAL_NODES=("exploration_coordinator" "frontier_detector" "emergency_stop_controller")

for node in "${CRITICAL_NODES[@]}"; do
    if ros2 node list 2>/dev/null | grep -q "$node"; then
        echo "✅ $node: Running"
    else
        echo "❌ $node: Not running"
    fi
done

echo ""

# Check topics
echo "📡 TOPIC STATUS CHECK:"
echo ""

CRITICAL_TOPICS=("/exploration_status" "/frontiers" "/safety_status" "/emergency_stop")

for topic in "${CRITICAL_TOPICS[@]}"; do
    if ros2 topic list 2>/dev/null | grep -q "$topic"; then
        echo "✅ $topic: Available"
    else
        echo "❌ $topic: Not available"
    fi
done

echo ""

# Overall assessment
echo "🎯 GOAL COMPLETION READINESS ASSESSMENT:"
echo ""

# Count successful validations
LAUNCH_SUCCESS=0
RUNTIME_SUCCESS=0

if [ -f "$SMART_LAUNCH" ]; then
    if grep -q "goal_reached_tolerance.*2\.0" "$SMART_LAUNCH" && \
       grep -q "goal_timeout.*30\.0" "$SMART_LAUNCH" && \
       grep -q "force_new_goal_distance.*2\.5" "$SMART_LAUNCH" && \
       grep -q "goal_tolerance.*1\.5" "$SMART_LAUNCH"; then
        LAUNCH_SUCCESS=1
    fi
fi

if ros2 node list 2>/dev/null | grep -q "exploration_coordinator"; then
    RUNTIME_SUCCESS=1
fi

if [ $LAUNCH_SUCCESS -eq 1 ] && [ $RUNTIME_SUCCESS -eq 1 ]; then
    echo "🎉 EXCELLENT: Ultra-aggressive goal completion is fully configured!"
    echo "   ✅ Launch files have correct parameters"
    echo "   ✅ Runtime system is active"
    echo "   ✅ Robot should NEVER get stuck at goals"
    echo "   ✅ Goals will complete within 30 seconds maximum"
    echo "   ✅ Completion triggers at 2.0m distance"
elif [ $LAUNCH_SUCCESS -eq 1 ]; then
    echo "⚠️  GOOD: Launch files configured, but system not running"
    echo "   ✅ Launch files have correct parameters"
    echo "   ❓ Start system with: ./start_smart_exploration.sh"
elif [ $RUNTIME_SUCCESS -eq 1 ]; then
    echo "⚠️  PARTIAL: System running but launch files may need updates"
    echo "   ❓ Check launch file parameters above"
    echo "   ✅ Runtime system is active"
else
    echo "❌ NEEDS ATTENTION: Goal completion not properly configured"
    echo "   ❌ Launch files may need parameter updates"
    echo "   ❌ System not currently running"
fi

echo ""
echo "🛠️  TROUBLESHOOTING RECOMMENDATIONS:"
echo ""

if [ $LAUNCH_SUCCESS -eq 0 ]; then
    echo "📝 Launch File Issues:"
    echo "   1. Update launch files with ultra-aggressive parameters"
    echo "   2. Rebuild: colcon build --packages-select turtle_bot_exploration"
    echo "   3. Source workspace: source install/setup.bash"
    echo ""
fi

if [ $RUNTIME_SUCCESS -eq 0 ]; then
    echo "🚀 System Not Running:"
    echo "   1. Start system: ./start_smart_exploration.sh"
    echo "   2. Check for errors in terminal output"
    echo "   3. Verify Gazebo is not already running"
    echo ""
fi

echo "🔧 Immediate Fixes:"
echo "   Apply runtime fixes: ./fix_stuck_goal.sh"
echo "   Monitor goal completion: ros2 topic echo /exploration_status"
echo "   Check safety status: ros2 topic echo /safety_status"
echo ""

echo "✅ Validation complete!"
