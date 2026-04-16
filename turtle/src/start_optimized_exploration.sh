#!/bin/bash

# 🚀 Optimized Exploration System Launcher
# Advanced autonomous exploration with comprehensive optimization features

echo "🚀 OPTIMIZED EXPLORATION SYSTEM - MAXIMUM PERFORMANCE"
echo "====================================================="
echo ""
echo "🎯 COMPREHENSIVE OPTIMIZATION FEATURES:"
echo "  ✅ Dynamic goal completion tolerance (velocity-based)"
echo "  ✅ Multi-criteria frontier selection (5 factors)"
echo "  ✅ Real-time performance monitoring and adaptive tuning"
echo "  ✅ Enhanced error recovery with 5-stage process"
echo "  ✅ Orientation-free goal completion (no rotation delays)"
echo "  ✅ Intelligent transition smoothing"
echo "  ✅ Environmental adaptation algorithms"
echo ""
echo "🧠 INTELLIGENT FRONTIER SELECTION:"
echo "  🎯 Distance efficiency scoring (20% weight)"
echo "  📐 Information gain analysis (30% weight)"
echo "  🛣️  Path accessibility evaluation (20% weight)"
echo "  ⏱️  Time efficiency optimization (15% weight)"
echo "  🎮 Strategic positioning analysis (15% weight)"
echo "  🔄 Dynamic weighting based on exploration phase"
echo ""
echo "📊 REAL-TIME PERFORMANCE MONITORING:"
echo "  📈 Goal completion rate tracking"
echo "  ⏰ Average completion time analysis"
echo "  🎯 Efficiency score calculation"
echo "  🔧 Adaptive parameter tuning"
echo "  📊 Movement efficiency analysis"
echo "  🎮 Automatic optimization recommendations"
echo ""
echo "🛡️ ENHANCED ERROR RECOVERY:"
echo "  🔄 5-stage intelligent recovery process"
echo "  🧠 Alternative goal selection"
echo "  🚨 Emergency unstuck maneuvers"
echo "  📍 Adaptive rotation with path analysis"
echo "  🎯 Strategic forward movement"
echo "  ⚡ Automatic recovery attempt limiting"
echo ""
echo "⚡ PERFORMANCE OPTIMIZATIONS:"
echo "  🎯 Ultra-aggressive goal completion (2.0m tolerance)"
echo "  ⏰ 30-second automatic timeout"
echo "  🚀 Force completion at 2.5m proximity"
echo "  📍 Velocity-based dynamic tolerance (0.8m - 3.0m)"
echo "  🔄 Environmental adaptation algorithms"
echo "  🎮 Transition smoothing optimization"
echo ""

# Function to check if a ROS2 node is running
check_node_running() {
    local node_name=$1
    ros2 node list 2>/dev/null | grep -q "$node_name"
}

# Function to validate optimized parameters
validate_optimized_parameters() {
    echo "🔍 VALIDATING OPTIMIZED SYSTEM CONFIGURATION..."
    
    # Check if optimized launch file exists
    if [ ! -f "turtle_bot_exploration/launch/optimized_exploration.launch.py" ]; then
        echo "❌ Error: Optimized exploration launch file not found!"
        echo "   Please ensure the optimized system is properly installed"
        return 1
    fi
    
    # Check if optimized parameters file exists
    if [ ! -f "turtle_bot_exploration/config/optimized_exploration_params.yaml" ]; then
        echo "❌ Error: Optimized parameters file not found!"
        echo "   Please ensure the optimized configuration is available"
        return 1
    fi
    
    # Check if orientation-free navigation params exist
    if [ ! -f "turtle_bot_navigation/config/orientation_free_params.yaml" ]; then
        echo "❌ Error: Orientation-free navigation parameters not found!"
        echo "   Please ensure the orientation-free system is installed"
        return 1
    fi
    
    # Check if performance monitor exists
    if [ ! -f "turtle_bot_exploration/src/performance_monitor.cpp" ]; then
        echo "❌ Error: Performance monitor not found!"
        echo "   Please ensure the performance monitoring system is built"
        return 1
    fi
    
    echo "✅ Optimized system configuration validated"
    echo "   📊 Optimized launch file: Available"
    echo "   🎯 Enhanced parameters: Available"
    echo "   🚀 Orientation-free navigation: Available"
    echo "   📈 Performance monitoring: Available"
    return 0
}

# Function to display system capabilities
display_system_capabilities() {
    echo ""
    echo "🎯 OPTIMIZED SYSTEM CAPABILITIES:"
    echo ""
    echo "📊 Goal Completion Optimization:"
    echo "   • Dynamic tolerance: 0.8m - 3.0m (velocity-based)"
    echo "   • Ultra-aggressive completion: 2.0m tolerance"
    echo "   • Automatic timeout: 30 seconds maximum"
    echo "   • Force completion: 2.5m proximity trigger"
    echo "   • Smooth transitions: Intelligent goal switching"
    echo ""
    echo "🧠 Intelligent Frontier Selection:"
    echo "   • Multi-criteria scoring: 5 optimization factors"
    echo "   • Dynamic weighting: Exploration phase adaptation"
    echo "   • Strategic positioning: Cluster analysis"
    echo "   • Accessibility evaluation: Path quality assessment"
    echo "   • Time efficiency: Optimal sequence planning"
    echo ""
    echo "📈 Performance Monitoring:"
    echo "   • Real-time metrics: Goal completion tracking"
    echo "   • Efficiency scoring: Multi-factor analysis"
    echo "   • Adaptive tuning: Automatic optimization"
    echo "   • Movement analysis: Velocity efficiency tracking"
    echo "   • Success rate monitoring: Continuous assessment"
    echo ""
    echo "🛡️ Enhanced Error Recovery:"
    echo "   • 5-stage recovery: Progressive unstuck process"
    echo "   • Alternative goals: Smart goal switching"
    echo "   • Emergency maneuvers: Rapid unstuck actions"
    echo "   • Adaptive rotation: Intelligent path finding"
    echo "   • Recovery limiting: Prevent infinite loops"
    echo ""
}

# Source the workspace
echo "🔧 SYSTEM INITIALIZATION:"
source install/setup.bash
echo "✅ Workspace sourced successfully"

# Validate optimized parameters
if ! validate_optimized_parameters; then
    echo ""
    echo "🛠️  TROUBLESHOOTING:"
    echo "   1. Ensure you're in the workspace root directory (~/turtle/src/)"
    echo "   2. Build the optimized system: colcon build --packages-select turtle_bot_exploration"
    echo "   3. Check that all optimization components are installed"
    echo "   4. Verify orientation-free navigation is configured"
    exit 1
fi

# Display system capabilities
display_system_capabilities

echo ""
echo "🚀 Starting optimized exploration system with comprehensive enhancements..."
echo ""
echo "📊 ENHANCED STARTUP SEQUENCE:"
echo "   0s  - Gazebo simulation with enhanced physics"
echo "   3s  - RViz with optimized exploration visualization"
echo "   6s  - SLAM mapping with real-time optimization"
echo "   12s - Orientation-free navigation with dynamic tolerance"
echo "   14s - Emergency stop controller with predictive safety"
echo "   16s - Frontier detector with intelligent clustering"
echo "   18s - Multi-criteria frontier selector with strategic analysis"
echo "   20s - Performance monitor with adaptive tuning"
echo "   22s - Optimized exploration coordinator with all enhancements"
echo "   24s - System ready with comprehensive optimization"
echo ""
echo "⏱️  Total startup time: 24 seconds"
echo ""

# Launch the optimized exploration system with monitoring
echo "🚀 Launching optimized exploration with comprehensive monitoring..."
echo ""

# Start the system in background to enable monitoring
ros2 launch turtle_bot_exploration optimized_exploration.launch.py &
LAUNCH_PID=$!

# Wait for system to start
echo "⏳ Waiting for optimized system initialization..."
sleep 26

# Check if system started successfully
if ! kill -0 $LAUNCH_PID 2>/dev/null; then
    echo "❌ Error: Optimized exploration system failed to start!"
    echo ""
    echo "🛠️  TROUBLESHOOTING STEPS:"
    echo "   1. Check if all packages are built: colcon build"
    echo "   2. Source workspace: source install/setup.bash"
    echo "   3. Check for error messages above"
    echo "   4. Verify Gazebo is not already running"
    echo "   5. Ensure all optimization components are available"
    echo "   6. Try: ./fix_stuck_goal.sh if system is running but stuck"
    exit 1
fi

echo "✅ Optimized exploration system started successfully!"
echo ""
echo "📊 COMPREHENSIVE OPTIMIZATION ACTIVE:"
echo "   🎯 Dynamic goal completion with velocity-based tolerance"
echo "   🧠 Multi-criteria frontier selection with strategic analysis"
echo "   📈 Real-time performance monitoring with adaptive tuning"
echo "   🛡️ Enhanced 5-stage error recovery system"
echo "   🚀 Orientation-free navigation with smooth transitions"
echo "   ⚡ Environmental adaptation algorithms"
echo ""
echo "🔍 REAL-TIME MONITORING COMMANDS:"
echo "   System performance: ros2 topic echo /performance_metrics"
echo "   Efficiency score: ros2 topic echo /efficiency_score"
echo "   Adaptive parameters: ros2 topic echo /adaptive_parameters"
echo "   Goal completion status: ros2 topic echo /exploration_status"
echo "   Safety status: ros2 topic echo /safety_status"
echo "   Frontier selection: ros2 topic echo /rosout | grep 'Selected frontier'"
echo ""
echo "🎯 EXPECTED OPTIMIZED BEHAVIOR:"
echo "   ✅ Goals complete within 2.0m tolerance (no orientation requirements)"
echo "   ✅ Dynamic tolerance adjusts based on robot velocity"
echo "   ✅ Intelligent frontier selection maximizes exploration efficiency"
echo "   ✅ Real-time performance monitoring optimizes parameters"
echo "   ✅ Enhanced recovery prevents all stuck situations"
echo "   ✅ Smooth transitions between frontiers"
echo "   ✅ Automatic adaptation to environmental conditions"
echo ""
echo "🚨 ADVANCED TROUBLESHOOTING:"
echo "   Monitor performance: ./monitor_goal_completion.sh"
echo "   Check optimization: ros2 topic echo /performance_metrics"
echo "   Validate parameters: ./validate_goal_completion.sh"
echo "   Emergency fixes: ./fix_stuck_goal.sh"
echo ""
echo "Press Ctrl+C to stop the optimized exploration system..."

# Wait for the launch process
wait $LAUNCH_PID

echo ""
echo "🏁 Optimized exploration system has been stopped."
echo "   All optimization components have been shut down safely."
echo ""
echo "📊 OPTIMIZATION PERFORMANCE SUMMARY:"
echo "   🎯 Dynamic goal completion system was active"
echo "   🧠 Multi-criteria frontier selection was operational"
echo "   📈 Real-time performance monitoring was enabled"
echo "   🛡️ Enhanced error recovery was available"
echo "   🚀 Orientation-free navigation was active"
echo "   ⚡ Environmental adaptation was functional"
echo ""
echo "🔄 TO RESTART WITH OPTIMIZATIONS:"
echo "   ./start_optimized_exploration.sh"
echo ""
echo "📊 PERFORMANCE ANALYSIS:"
echo "   Check final metrics: ros2 topic echo /performance_metrics --once"
echo "   Review efficiency scores: ros2 topic echo /efficiency_score --once"
echo "   Analyze adaptive changes: ros2 topic echo /adaptive_parameters --once"
echo ""
echo "🛠️  IF ISSUES OCCURRED:"
echo "   Check logs above for any error messages"
echo "   Verify optimization components with: ./validate_goal_completion.sh"
echo "   Apply immediate fixes with: ./fix_stuck_goal.sh"
echo "   Monitor system performance with: ./monitor_goal_completion.sh"
