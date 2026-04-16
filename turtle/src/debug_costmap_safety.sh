#!/bin/bash

# 🛡️ Costmap Safety Diagnostic Script
# Monitors and debugs costmap issues for enhanced safety

echo "🛡️ COSTMAP SAFETY DIAGNOSTICS"
echo "============================="
echo ""

# Source the workspace
source install/setup.bash

echo "📊 CHECKING COSTMAP PARAMETERS..."
echo ""

# Check local costmap parameters
echo "🗺️ LOCAL COSTMAP CONFIGURATION:"
echo "  Width: $(ros2 param get /local_costmap/local_costmap width 2>/dev/null || echo 'Not available')"
echo "  Height: $(ros2 param get /local_costmap/local_costmap height 2>/dev/null || echo 'Not available')"
echo "  Resolution: $(ros2 param get /local_costmap/local_costmap resolution 2>/dev/null || echo 'Not available')"
echo "  Robot Radius: $(ros2 param get /local_costmap/local_costmap robot_radius 2>/dev/null || echo 'Not available')"
echo "  Inflation Radius: $(ros2 param get /local_costmap/local_costmap inflation_layer.inflation_radius 2>/dev/null || echo 'Not available')"
echo "  Cost Scaling: $(ros2 param get /local_costmap/local_costmap inflation_layer.cost_scaling_factor 2>/dev/null || echo 'Not available')"
echo ""

# Check emergency stop controller parameters
echo "🚨 EMERGENCY STOP CONTROLLER:"
echo "  Emergency Distance: $(ros2 param get /emergency_stop_controller emergency_stop_distance 2>/dev/null || echo 'Not available')"
echo "  Warning Distance: $(ros2 param get /emergency_stop_controller warning_distance 2>/dev/null || echo 'Not available')"
echo "  Safety Frequency: $(ros2 param get /emergency_stop_controller safety_check_frequency 2>/dev/null || echo 'Not available')"
echo ""

# Check controller server parameters
echo "🎮 CONTROLLER SERVER:"
echo "  Controller Frequency: $(ros2 param get /controller_server controller_frequency 2>/dev/null || echo 'Not available')"
echo "  Max Linear Velocity: $(ros2 param get /controller_server FollowPath.max_vel_x 2>/dev/null || echo 'Not available')"
echo "  Obstacle Scale: $(ros2 param get /controller_server FollowPath.BaseObstacle.scale 2>/dev/null || echo 'Not available')"
echo ""

echo "📡 MONITORING REAL-TIME STATUS..."
echo ""

# Function to monitor topics with timeout
monitor_topic() {
    local topic=$1
    local description=$2
    local timeout=3
    
    echo "📊 $description:"
    timeout $timeout ros2 topic echo $topic --once 2>/dev/null || echo "  ❌ Topic not available or no data"
    echo ""
}

# Monitor safety status
monitor_topic "/safety_status" "Safety Status"
monitor_topic "/emergency_stop" "Emergency Stop Status"

# Check if nodes are running
echo "🔍 CHECKING CRITICAL NODES..."
echo ""

check_node() {
    local node_name=$1
    local description=$2
    
    if ros2 node list 2>/dev/null | grep -q "$node_name"; then
        echo "  ✅ $description: Running"
    else
        echo "  ❌ $description: NOT RUNNING"
    fi
}

check_node "emergency_stop_controller" "Emergency Stop Controller"
check_node "controller_server" "Controller Server"
check_node "local_costmap" "Local Costmap"
check_node "global_costmap" "Global Costmap"

echo ""
echo "🚨 CHECKING FOR COSTMAP WARNINGS..."
echo ""

# Monitor costmap warnings for 10 seconds
echo "Monitoring costmap logs for out-of-bounds warnings (10 seconds)..."
timeout 10 ros2 topic echo /rosout --once 2>/dev/null | grep -i "out of map bounds" || echo "No out-of-bounds warnings detected in the last message"

echo ""
echo "📋 DIAGNOSTIC SUMMARY:"
echo ""

# Check if emergency stop controller is working
if ros2 node list 2>/dev/null | grep -q "emergency_stop_controller"; then
    echo "  ✅ Emergency Stop Controller: Active"
else
    echo "  ❌ Emergency Stop Controller: MISSING - Critical safety system not running!"
fi

# Check costmap size
width=$(ros2 param get /local_costmap/local_costmap width 2>/dev/null | grep -o '[0-9]*' || echo "0")
if [ "$width" -ge 8 ]; then
    echo "  ✅ Local Costmap Size: Adequate (${width}x${width})"
else
    echo "  ⚠️  Local Costmap Size: May be too small (${width}x${width}) - recommend 8x8 or larger"
fi

# Check inflation radius
inflation=$(ros2 param get /local_costmap/local_costmap inflation_layer.inflation_radius 2>/dev/null | grep -o '[0-9.]*' || echo "0")
if (( $(echo "$inflation >= 0.6" | bc -l 2>/dev/null || echo "0") )); then
    echo "  ✅ Inflation Radius: Good (${inflation}m)"
else
    echo "  ⚠️  Inflation Radius: May be too small (${inflation}m) - recommend 0.6m or larger"
fi

echo ""
echo "🛠️ TROUBLESHOOTING RECOMMENDATIONS:"
echo ""

if ! ros2 node list 2>/dev/null | grep -q "emergency_stop_controller"; then
    echo "  🔧 Start Emergency Stop Controller:"
    echo "     ros2 run turtle_bot_navigation emergency_stop_controller_node"
    echo ""
fi

if [ "$width" -lt 8 ]; then
    echo "  🔧 Increase Local Costmap Size:"
    echo "     Edit robust_safety_params.yaml and set width/height to 10"
    echo ""
fi

echo "  🔧 Monitor Real-time Safety:"
echo "     ros2 topic echo /safety_status"
echo ""
echo "  🔧 Check Emergency Stop:"
echo "     ros2 topic echo /emergency_stop"
echo ""
echo "  🔧 Monitor Costmap Updates:"
echo "     ros2 topic echo /local_costmap/costmap_updates"
echo ""

echo "🎯 For continuous monitoring, run:"
echo "   watch -n 1 'ros2 topic echo /safety_status --once'"
echo ""
echo "✅ Diagnostic complete!"
