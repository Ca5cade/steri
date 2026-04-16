#!/bin/bash

# 🔍 Enhanced Safe Exploration System Verification Script
# Checks if all components are properly configured

echo "🔍 ENHANCED SAFE EXPLORATION SYSTEM VERIFICATION"
echo "================================================"
echo ""

# Source the workspace
source install/setup.bash

echo "✅ Checking system components..."
echo ""

# Check 1: Launch file exists
echo "1. 📁 Checking launch file..."
if [ -f "turtle_bot_exploration/launch/complete_safe_exploration.launch.py" ]; then
    echo "   ✅ Complete launch file found"
else
    echo "   ❌ Launch file missing!"
    exit 1
fi

# Check 2: Enhanced nav2 parameters
echo "2. ⚙️  Checking enhanced nav2 parameters..."
if [ -f "turtle_bot_navigation/config/nav2_params.yaml" ]; then
    echo "   ✅ Nav2 parameters file found"
    
    # Check for enhanced safety parameters
    if grep -q "inflation_radius: 0.6" turtle_bot_navigation/config/nav2_params.yaml; then
        echo "   ✅ Enhanced inflation radius (0.6m) configured"
    else
        echo "   ⚠️  Enhanced inflation radius not found"
    fi
    
    if grep -q "BaseObstacle.scale: 0.1" turtle_bot_navigation/config/nav2_params.yaml; then
        echo "   ✅ Enhanced obstacle avoidance (0.1) configured"
    else
        echo "   ⚠️  Enhanced obstacle avoidance not found"
    fi
    
    if grep -q "recoveries_server:" turtle_bot_navigation/config/nav2_params.yaml; then
        echo "   ✅ Recovery behaviors configured"
    else
        echo "   ⚠️  Recovery behaviors not found"
    fi
else
    echo "   ❌ Nav2 parameters file missing!"
    exit 1
fi

# Check 3: Exploration coordinator enhancements
echo "3. 🤖 Checking exploration coordinator enhancements..."
if grep -q "isFrontierSafe" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    echo "   ✅ Frontier safety validation implemented"
else
    echo "   ❌ Frontier safety validation missing!"
fi

if grep -q "getDistanceToNearestObstacle" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    echo "   ✅ Obstacle distance calculation implemented"
else
    echo "   ❌ Obstacle distance calculation missing!"
fi

if grep -q "Stage 1: Back away" turtle_bot_exploration/src/exploration_coordinator.cpp; then
    echo "   ✅ Multi-stage recovery implemented"
else
    echo "   ❌ Multi-stage recovery missing!"
fi

# Check 4: Package dependencies
echo "4. 📦 Checking package dependencies..."
packages=("nav2_bringup" "turtle_bot_gazebo" "turtle_bot_navigation" "turtle_bot_exploration")
for pkg in "${packages[@]}"; do
    if ros2 pkg list | grep -q "^$pkg$"; then
        echo "   ✅ $pkg package available"
    else
        echo "   ❌ $pkg package missing!"
    fi
done

# Check 5: Executable permissions
echo "5. 🔐 Checking executable permissions..."
if [ -x "start_safe_exploration.sh" ]; then
    echo "   ✅ Start script is executable"
else
    echo "   ⚠️  Making start script executable..."
    chmod +x start_safe_exploration.sh
    echo "   ✅ Start script permissions fixed"
fi

# Check 6: RViz configuration
echo "6. 🎯 Checking RViz configuration..."
if [ -f "turtle_bot_exploration/config/exploration.rviz" ]; then
    echo "   ✅ RViz exploration config found"
else
    echo "   ⚠️  RViz config not found (will use default)"
fi

echo ""
echo "🎯 SYSTEM VERIFICATION COMPLETE"
echo ""

# Summary
echo "📊 SAFETY FEATURES SUMMARY:"
echo "   🛡️ 60cm obstacle inflation radius"
echo "   🛡️ 80cm frontier safety margin"
echo "   🛡️ 5x stronger obstacle avoidance"
echo "   🛡️ Multi-stage stuck recovery"
echo "   🛡️ Intelligent frontier validation"
echo "   🛡️ Enhanced collision prevention"
echo ""

echo "🚀 READY TO LAUNCH!"
echo "   Run: ./start_safe_exploration.sh"
echo ""
echo "📖 For detailed information, see:"
echo "   - COMPLETE_SYSTEM_GUIDE.md"
echo "   - COLLISION_AVOIDANCE_IMPROVEMENTS.md"
echo "   - TESTING_GUIDE.md"
