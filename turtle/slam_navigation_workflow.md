# SLAM → Navigation Workflow

## Current Status: SLAM is Running ✅

Your SLAM system is currently active and building a map in real-time!

## Step 1: Control the Robot to Build the Map

Open a **new terminal** and run:

```bash
cd ~/turtle
source install/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

**Controls:**
- `i` - Move forward
- `,` - Move backward  
- `j` - Turn left
- `l` - Turn right
- `k` - Stop
- `q/z` - Increase/decrease linear speed
- `w/x` - Increase/decrease angular speed

**Drive around the environment** to explore and build a complete map. You'll see the map being built in real-time in the SLAM toolbox.

## Step 2: Monitor Map Building

You can check the map topic:
```bash
ros2 topic echo /map --once
```

Or visualize with RViz2 (if display issues are resolved):
```bash
rviz2 -d ~/turtle/install/turtle_bot_bringup/share/turtle_bot_bringup/launch/turtle_bot.rviz
```

## Step 3: Save the Generated Map

Once you're satisfied with the map coverage, save it:

```bash
cd ~/turtle
source install/setup.bash
ros2 run nav2_map_server map_saver_cli -f src/turtle_bot_navigation/maps/my_slam_map
```

This will create:
- `my_slam_map.pgm` - The map image
- `my_slam_map.yaml` - The map metadata

## Step 4: Stop SLAM

In the SLAM terminal, press `Ctrl+C` to stop the SLAM launch.

## Step 5: Update Navigation to Use Your Map

Edit the navigation launch to use your new map:

```bash
# Option 1: Use the new map directly
ros2 launch turtle_bot_navigation navigation.launch.py map:=$HOME/turtle/src/turtle_bot_navigation/maps/my_slam_map.yaml

# Option 2: Update the default map and rebuild
cp src/turtle_bot_navigation/maps/my_slam_map.yaml src/turtle_bot_navigation/maps/map.yaml
cp src/turtle_bot_navigation/maps/my_slam_map.pgm src/turtle_bot_navigation/maps/map.pgm
colcon build --packages-select turtle_bot_navigation
```

## Step 6: Run Navigation

```bash
# Start simulation (in one terminal)
ros2 launch turtle_bot_gazebo gazebo.launch.py

# Start navigation (in another terminal)  
ros2 launch turtle_bot_navigation navigation.launch.py

# Or use your custom map directly
ros2 launch turtle_bot_navigation navigation.launch.py map:=$HOME/turtle/src/turtle_bot_navigation/maps/my_slam_map.yaml
```

## Step 7: Set Initial Pose and Navigate

1. **Set initial pose**: Use RViz2 "2D Pose Estimate" tool to set robot's initial position
2. **Set navigation goal**: Use RViz2 "2D Nav Goal" tool to command the robot to navigate

## Tips

- **Map Quality**: Drive slowly and ensure good lidar coverage of all areas
- **Loop Closure**: Return to starting areas to improve map accuracy  
- **Save Multiple Maps**: Save maps with different names for different environments
- **Backup**: Keep your generated maps safe - they represent your environment!

## Troubleshooting

- **No map data**: Ensure lidar is working (`ros2 topic echo /scan`)
- **Poor map quality**: Drive slower, ensure good lighting
- **Navigation issues**: Verify transforms (`ros2 run tf2_tools view_frames`)
