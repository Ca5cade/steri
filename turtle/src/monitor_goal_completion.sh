#!/bin/bash

# 🎯 Real-time Goal Completion Monitoring Script
# Monitors goal completion status and provides real-time feedback

echo "🎯 REAL-TIME GOAL COMPLETION MONITORING"
echo "======================================="
echo ""

# Source the workspace
source install/setup.bash 2>/dev/null

# Function to get current robot position
get_robot_position() {
    ros2 topic echo /odom --once 2>/dev/null | grep -A 3 "position:" | grep -E "x:|y:" | awk '{print $2}' | tr '\n' ' '
}

# Function to get current goal
get_current_goal() {
    ros2 topic echo /move_base/goal --once 2>/dev/null | grep -A 3 "position:" | grep -E "x:|y:" | awk '{print $2}' | tr '\n' ' '
}

# Function to calculate distance
calculate_distance() {
    local x1=$1 y1=$2 x2=$3 y2=$4
    echo "scale=2; sqrt(($x1-$x2)*($x1-$x2) + ($y1-$y2)*($y1-$y2))" | bc -l 2>/dev/null || echo "N/A"
}

# Function to check parameter
check_param() {
    local param=$1
    ros2 param get /exploration_coordinator "$param" 2>/dev/null | grep -o '[0-9.]*' | head -1
}

# Function to monitor exploration status
monitor_status() {
    echo "📊 GOAL COMPLETION MONITORING ACTIVE"
    echo "====================================="
    echo ""
    
    # Display current parameters
    echo "🎯 ULTRA-AGGRESSIVE PARAMETERS:"
    echo "   goal_reached_tolerance: $(check_param goal_reached_tolerance)m"
    echo "   goal_timeout: $(check_param goal_timeout)s"
    echo "   force_new_goal_distance: $(check_param force_new_goal_distance)m"
    echo "   goal_tolerance: $(check_param goal_tolerance)m"
    echo ""
    
    local start_time=$(date +%s)
    local goal_start_time=$start_time
    local last_position=""
    local goal_count=0
    local completion_count=0
    
    echo "⏰ Monitoring started at $(date)"
    echo "Press Ctrl+C to stop monitoring"
    echo ""
    echo "📊 REAL-TIME STATUS:"
    echo "==================="
    
    while true; do
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        local goal_elapsed=$((current_time - goal_start_time))
        
        # Get current position
        local position=$(get_robot_position)
        
        # Check if position changed (robot is moving)
        if [ "$position" != "$last_position" ] && [ -n "$position" ]; then
            last_position="$position"
            
            # Get exploration status
            local status=$(ros2 topic echo /exploration_status --once 2>/dev/null | grep "data:" | cut -d'"' -f2)
            
            # Get safety status
            local safety=$(ros2 topic echo /safety_status --once 2>/dev/null | grep "data:" | cut -d'"' -f2)
            
            # Clear screen and show status
            clear
            echo "🎯 REAL-TIME GOAL COMPLETION MONITORING"
            echo "======================================="
            echo ""
            echo "⏰ Monitoring Time: ${elapsed}s"
            echo "🎯 Current Goal Time: ${goal_elapsed}s"
            echo "📊 Goals Attempted: $goal_count"
            echo "✅ Goals Completed: $completion_count"
            echo ""
            echo "🤖 ROBOT STATUS:"
            echo "   Position: $position"
            echo "   Exploration: ${status:-"Unknown"}"
            echo "   Safety: ${safety:-"Unknown"}"
            echo ""
            
            # Check for goal timeout warning
            if [ $goal_elapsed -gt 25 ]; then
                echo "⚠️  WARNING: Goal time approaching 30s timeout!"
                echo "   Goal will auto-complete in $((30 - goal_elapsed))s"
                echo ""
            fi
            
            # Check for goal completion
            if echo "$status" | grep -q -i "complete\|reached\|selecting"; then
                completion_count=$((completion_count + 1))
                goal_start_time=$current_time
                echo "✅ GOAL COMPLETED! (Time: ${goal_elapsed}s)"
                echo ""
            fi
            
            # Check for new goal
            if echo "$status" | grep -q -i "navigating\|exploring"; then
                if [ $goal_elapsed -gt 60 ]; then  # New goal detected
                    goal_count=$((goal_count + 1))
                    goal_start_time=$current_time
                fi
            fi
            
            echo "🎯 ULTRA-AGGRESSIVE COMPLETION CRITERIA:"
            echo "   ✅ Complete when within 2.0m of goal"
            echo "   ⏰ Auto-complete after 30 seconds"
            echo "   🚀 Force complete when within 2.5m and moved"
            echo "   🔄 Enhanced stuck detection active"
            echo ""
            
            # Show recent log messages
            echo "📝 RECENT GOAL COMPLETION LOGS:"
            ros2 topic echo /rosout --once 2>/dev/null | grep -i "goal\|completion\|reached" | tail -3 | while read line; do
                echo "   $line"
            done
            echo ""
            
            echo "🛠️  TROUBLESHOOTING COMMANDS:"
            echo "   Force goal completion: ./fix_stuck_goal.sh"
            echo "   Check parameters: ./validate_goal_completion.sh"
            echo "   View exploration status: ros2 topic echo /exploration_status"
            echo ""
            echo "Press Ctrl+C to stop monitoring"
        fi
        
        sleep 2
    done
}

# Check if system is running
if ! ros2 node list 2>/dev/null | grep -q "exploration_coordinator"; then
    echo "❌ Exploration coordinator not running!"
    echo ""
    echo "🚀 Start the system first:"
    echo "   ./start_smart_exploration.sh"
    echo ""
    echo "🔍 Or check system status:"
    echo "   ./validate_goal_completion.sh"
    exit 1
fi

# Check if ultra-aggressive parameters are active
echo "🔍 Checking ultra-aggressive parameters..."
goal_tolerance=$(check_param goal_reached_tolerance)
timeout=$(check_param goal_timeout)
force_distance=$(check_param force_new_goal_distance)

if [ "$goal_tolerance" = "2.0" ] && [ "$timeout" = "30.0" ] && [ "$force_distance" = "2.5" ]; then
    echo "✅ Ultra-aggressive goal completion parameters confirmed!"
    echo ""
else
    echo "⚠️  Warning: Parameters may not be optimal"
    echo "   goal_reached_tolerance: $goal_tolerance (should be 2.0)"
    echo "   goal_timeout: $timeout (should be 30.0)"
    echo "   force_new_goal_distance: $force_distance (should be 2.5)"
    echo ""
    echo "🔧 Apply fixes with: ./fix_stuck_goal.sh"
    echo ""
fi

# Start monitoring
monitor_status
