#!/bin/bash

# Test script for TurtleBot ROS2 project
echo "Testing TurtleBot ROS2 Humble Project..."

# Source the workspace
cd /home/haythem/turtle
source install/setup.bash

echo "1. Checking if packages are available..."
ros2 pkg list | grep turtle_bot

echo -e "\n2. Testing URDF parsing..."
xacro src/turtle_bot_description/urdf/turtle_bot_gazebo.urdf.xacro > /tmp/robot.urdf
if [ $? -eq 0 ]; then
    echo "✓ URDF parsing successful"
else
    echo "✗ URDF parsing failed"
fi

echo -e "\n3. Checking launch files..."
if [ -f "src/turtle_bot_bringup/launch/turtle_bot_simulation.launch.py" ]; then
    echo "✓ Main simulation launch file exists"
else
    echo "✗ Main simulation launch file missing"
fi

if [ -f "src/turtle_bot_bringup/launch/turtle_bot_slam.launch.py" ]; then
    echo "✓ SLAM launch file exists"
else
    echo "✗ SLAM launch file missing"
fi

if [ -f "src/turtle_bot_bringup/launch/turtle_bot_navigation.launch.py" ]; then
    echo "✓ Navigation launch file exists"
else
    echo "✗ Navigation launch file missing"
fi

echo -e "\n4. Checking configuration files..."
if [ -f "src/turtle_bot_slam/config/slam_params.yaml" ]; then
    echo "✓ SLAM parameters file exists"
else
    echo "✗ SLAM parameters file missing"
fi

if [ -f "src/turtle_bot_navigation/config/nav2_params.yaml" ]; then
    echo "✓ Nav2 parameters file exists"
else
    echo "✗ Nav2 parameters file missing"
fi

echo -e "\n5. Checking map files..."
if [ -f "src/turtle_bot_navigation/maps/map.yaml" ]; then
    echo "✓ Map YAML file exists"
else
    echo "✗ Map YAML file missing"
fi

if [ -f "src/turtle_bot_navigation/maps/map.pgm" ]; then
    echo "✓ Map PGM file exists"
else
    echo "✗ Map PGM file missing"
fi

echo -e "\nProject structure test completed!"
echo -e "\nTo run the project:"
echo "1. Basic simulation: ros2 launch turtle_bot_bringup turtle_bot_simulation.launch.py"
echo "2. SLAM mode: ros2 launch turtle_bot_bringup turtle_bot_slam.launch.py"
echo "3. Navigation mode: ros2 launch turtle_bot_bringup turtle_bot_navigation.launch.py"
