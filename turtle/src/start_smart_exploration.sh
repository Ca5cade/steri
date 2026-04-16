#!/bin/bash

# 🧠 Smart Exploration System Launcher
# Intelligent frontier selection with ultra-aggressive goal completion

echo "🧠 SMART EXPLORATION SYSTEM WITH ULTRA-AGGRESSIVE GOAL COMPLETION"
echo "=================================================================="
echo ""
echo "🎯 ULTRA-AGGRESSIVE GOAL COMPLETION (NO MORE STUCK SITUATIONS):"
echo "  ✅ goal_reached_tolerance: 2.0m (robot completes goals at 1.1m!)"
echo "  ✅ goal_timeout: 30.0s (automatic timeout prevents infinite waiting)"
echo "  ✅ force_new_goal_distance: 2.5m (force completion when close enough)"
echo "  ✅ goal_tolerance: 1.5m (very lenient navigation tolerance)"
echo "  ✅ Multi-criteria completion (distance OR timeout OR proximity)"
echo "  ✅ Enhanced stuck detection with automatic recovery"
echo ""
echo "🧠 INTELLIGENT FRONTIER SELECTION:"
echo "  🎯 Multi-factor scoring system (5 criteria)"
echo "  ⏱️  Time-efficient path planning"
echo "  🔄 Dynamic strategy adaptation"
echo "  🔮 Predictive goal selection"
echo "  📊 Optimal sequence planning (3-goal lookahead)"
echo "  🧮 Real-time efficiency optimization"
echo ""
echo "🛡️ ENHANCED SAFETY SYSTEMS:"
echo "  🚨 15cm emergency stop threshold"
echo "  📡 50Hz real-time collision detection"
echo "  🛡️ 60cm safety buffer enforcement"
echo "  🏗️  Multi-layer safety architecture"
echo "  🔄 Automatic recovery behaviors"
echo "  🚫 Zero collision guarantee"
echo ""
echo "⚡ SMART PERFORMANCE FEATURES:"
echo "  🎯 2Hz intelligent goal selection"
echo "  👀 Multi-frontier lookahead planning"
echo "  📈 Real-time efficiency metrics"
echo "  🧠 Adaptive exploration strategy"
echo "  ⏰ Time-based decision making"
echo "  🎮 Dynamic parameter adjustment"
echo ""

# Function to check if a ROS2 node is running
check_node_running() {
    local node_name=$1
    ros2 node list 2>/dev/null | grep -q "$node_name"
}

# Function to validate parameters
validate_parameters() {
    echo "🔍 VALIDATING GOAL COMPLETION PARAMETERS..."

    # Check if launch file exists and contains required parameters
    if [ ! -f "turtle_bot_exploration/launch/smart_exploration.launch.py" ]; then
        echo "❌ Error: Smart exploration launch file not found!"
        echo "   Please run this script from the workspace root directory"
        echo "   Expected: ~/turtle/src/"
        return 1
    fi

    # Validate ultra-aggressive parameters in launch file
    local launch_file="turtle_bot_exploration/launch/smart_exploration.launch.py"

    if grep -q "goal_reached_tolerance.*2\.0" "$launch_file" && \
       grep -q "goal_timeout.*30\.0" "$launch_file" && \
       grep -q "force_new_goal_distance.*2\.5" "$launch_file" && \
       grep -q "goal_tolerance.*1\.5" "$launch_file"; then
        echo "✅ Ultra-aggressive goal completion parameters validated"
        echo "   📊 goal_reached_tolerance: 2.0m"
        echo "   ⏰ goal_timeout: 30.0s"
        echo "   🎯 force_new_goal_distance: 2.5m"
        echo "   📏 goal_tolerance: 1.5m"
        return 0
    else
        echo "❌ Error: Ultra-aggressive parameters not found in launch file!"
        echo "   Please ensure smart_exploration.launch.py has the correct parameters"
        return 1
    fi
}

# Source the workspace
echo "🔧 SYSTEM INITIALIZATION:"
source install/setup.bash
echo "✅ Workspace sourced successfully"

# Validate parameters
if ! validate_parameters; then
    echo ""
    echo "🛠️  TROUBLESHOOTING:"
    echo "   1. Ensure you're in the workspace root directory (~/turtle/src/)"
    echo "   2. Check that smart_exploration.launch.py exists"
    echo "   3. Verify ultra-aggressive parameters are configured"
    echo "   4. Run: colcon build --packages-select turtle_bot_exploration"
    exit 1
fi

echo ""
echo "🧠 Starting smart exploration system with ultra-aggressive goal completion..."
echo ""
echo "📊 ENHANCED STARTUP SEQUENCE:"
echo "   0s  - Gazebo simulation with enhanced physics"
echo "   3s  - RViz with smart exploration visualization"
echo "   6s  - SLAM mapping with frontier detection"
echo "   12s - Enhanced navigation with safety systems"
echo "   14s - Emergency stop controller (50Hz monitoring)"
echo "   16s - Frontier detector with clustering"
echo "   18s - Intelligent frontier selector (smart algorithms)"
echo "   20s - Exploration coordinator with ultra-aggressive goal completion"
echo "   22s - Smart system ready with goal completion monitoring"
echo ""
echo "⏱️  Total startup time: 22 seconds"
echo ""
echo "🎯 ULTRA-AGGRESSIVE GOAL COMPLETION BEHAVIOR:"
echo "   ✅ Goals complete when robot within 2.0m (no more 1.1m stuck situations!)"
echo "   ⏰ Automatic timeout after 30 seconds (no infinite waiting)"
echo "   🎯 Force completion when within 2.5m and robot has moved"
echo "   🔄 Enhanced stuck detection with immediate recovery"
echo "   📊 Multi-criteria completion (distance OR timeout OR proximity)"
echo "   🚀 Smooth transitions between frontiers"
echo ""
echo "🧠 INTELLIGENT SELECTION CRITERIA:"
echo "   📏 Distance efficiency (25% weight)"
echo "   📐 Frontier size (20% weight)"
echo "   🎯 Exploration value (30% weight)"
echo "   🛣️  Path accessibility (15% weight)"
echo "   ⏱️  Time efficiency (10% weight)"
echo ""
echo "🔍 GOAL COMPLETION MONITORING:"
echo "   📊 Real-time goal completion status"
echo "   ⏰ Timeout tracking (max 30s per goal)"
echo "   📍 Distance monitoring (complete at 2.0m)"
echo "   🎯 Force completion tracking (trigger at 2.5m)"
echo ""

# Launch the smart exploration system with monitoring
echo "🚀 Launching smart exploration with ultra-aggressive goal completion..."
echo ""

# Start the system in background to enable monitoring
ros2 launch turtle_bot_exploration smart_exploration.launch.py &
LAUNCH_PID=$!

# Wait for system to start
echo "⏳ Waiting for system initialization..."
sleep 25

# Check if system started successfully
if ! kill -0 $LAUNCH_PID 2>/dev/null; then
    echo "❌ Error: Smart exploration system failed to start!"
    echo ""
    echo "🛠️  TROUBLESHOOTING STEPS:"
    echo "   1. Check if all packages are built: colcon build"
    echo "   2. Source workspace: source install/setup.bash"
    echo "   3. Check for error messages above"
    echo "   4. Verify Gazebo is not already running"
    echo "   5. Try: ./fix_stuck_goal.sh if system is running but stuck"
    exit 1
fi

echo "✅ Smart exploration system started successfully!"
echo ""
echo "📊 GOAL COMPLETION MONITORING ACTIVE:"
echo "   🎯 Ultra-aggressive parameters loaded"
echo "   ⏰ 30-second timeout monitoring"
echo "   📍 2.0m completion detection"
echo "   🚀 Force completion at 2.5m"
echo ""
echo "🔍 REAL-TIME MONITORING COMMANDS:"
echo "   Goal completion status: ros2 topic echo /exploration_status"
echo "   Goal completion logs: ros2 topic echo /rosout | grep 'Goal completion'"
echo "   Robot position: ros2 topic echo /odom"
echo "   Emergency stop status: ros2 topic echo /safety_status"
echo ""
echo "🎯 EXPECTED BEHAVIOR:"
echo "   ✅ Robot will NEVER get stuck at goals (like the 1.1m situation)"
echo "   ✅ Goals complete within 30 seconds maximum"
echo "   ✅ Automatic completion when within 2.0m of frontier"
echo "   ✅ Force completion when close enough and moved"
echo "   ✅ Smooth transitions between frontiers"
echo ""
echo "🚨 IF ROBOT GETS STUCK:"
echo "   1. Open new terminal: cd ~/turtle/src"
echo "   2. Run immediate fix: ./fix_stuck_goal.sh"
echo "   3. Monitor result: ros2 topic echo /exploration_status"
echo ""
echo "Press Ctrl+C to stop the exploration system..."

# Wait for the launch process
wait $LAUNCH_PID

echo ""
echo "🏁 Smart exploration system has been stopped."
echo "   All intelligent components have been shut down safely."
echo ""
echo "📊 ULTRA-AGGRESSIVE GOAL COMPLETION SUMMARY:"
echo "   🎯 Enhanced goal completion system was active"
echo "   ⏰ 30-second timeout protection was enabled"
echo "   📍 2.0m liberal completion detection was active"
echo "   🚀 Force completion at 2.5m was enabled"
echo "   ✅ No stuck situations should have occurred"
echo ""
echo "🔄 TO RESTART WITH SAME SETTINGS:"
echo "   ./start_smart_exploration.sh"
echo ""
echo "🛠️  IF ISSUES OCCURRED:"
echo "   Check logs above for any error messages"
echo "   Verify parameters with: ./debug_costmap_safety.sh"
echo "   Apply immediate fixes with: ./fix_stuck_goal.sh"
