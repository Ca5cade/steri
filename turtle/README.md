# TurtleBot ROS2 Humble Robotics Project

A complete ROS2 Humble robotics project featuring a custom TurtleBot-like robot with SLAM and navigation capabilities.

## Project Structure

```
src/
├── turtle_bot_description/     # Robot URDF/Xacro files and meshes
├── turtle_bot_gazebo/          # Gazebo simulation world and launch files
├── turtle_bot_slam/            # SLAM configuration and launch files
├── turtle_bot_navigation/      # Nav2 configuration and launch files
└── turtle_bot_bringup/         # Main launch files to coordinate all systems
```

## Features

- **Custom Robot Design**: TurtleBot-like robot with differential drive, lidar, camera, and IMU
- **Gazebo Simulation**: Realistic physics simulation with custom world environment
- **SLAM Capability**: Real-time mapping using SLAM Toolbox
- **Autonomous Navigation**: Nav2 stack for path planning and obstacle avoidance
- **RViz Visualization**: Complete visualization setup for monitoring robot state

## Prerequisites

- ROS2 Humble
- Gazebo Classic
- Navigation2
- SLAM Toolbox

Install dependencies:
```bash
sudo apt update
sudo apt install ros-humble-desktop-full
sudo apt install ros-humble-gazebo-ros-pkgs
sudo apt install ros-humble-navigation2
sudo apt install ros-humble-nav2-bringup
sudo apt install ros-humble-slam-toolbox
sudo apt install ros-humble-robot-state-publisher
sudo apt install ros-humble-joint-state-publisher
sudo apt install ros-humble-xacro
```

## Building the Project

1. Clone or create the workspace:
```bash
cd ~/turtle
```

2. Build the workspace:
```bash
colcon build
source install/setup.bash
```

## Usage

### 1. Basic Simulation
Launch the robot in Gazebo with RViz:
```bash
ros2 launch turtle_bot_bringup turtle_bot_simulation.launch.py
```

### 2. SLAM Mode
Launch simulation with SLAM for mapping:
```bash
ros2 launch turtle_bot_bringup turtle_bot_slam.launch.py
```

Control the robot to create a map:
```bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

Save the map:
```bash
ros2 run nav2_map_server map_saver_cli -f ~/turtle/src/turtle_bot_navigation/maps/my_map
```

### 3. Navigation Mode
Launch simulation with autonomous navigation:
```bash
ros2 launch turtle_bot_bringup turtle_bot_navigation.launch.py
```

Use RViz to:
- Set initial pose (2D Pose Estimate)
- Set navigation goals (2D Nav Goal)

## Package Details

### turtle_bot_description
- `urdf/turtle_bot.urdf.xacro`: Main robot description
- `urdf/turtle_bot_gazebo.urdf.xacro`: Gazebo plugins and sensors
- Robot includes: differential drive, lidar, camera, IMU

### turtle_bot_gazebo
- `worlds/turtle_world.world`: Custom Gazebo world with obstacles
- Gazebo plugins for realistic sensor simulation

### turtle_bot_slam
- `config/slam_params.yaml`: SLAM Toolbox configuration
- Optimized for indoor mapping scenarios

### turtle_bot_navigation
- `config/nav2_params.yaml`: Nav2 stack parameters
- Configured for differential drive robot
- Includes costmap, planner, and controller settings

### turtle_bot_bringup
- Main launch files for different operation modes
- Coordinates all subsystems with proper timing

## Troubleshooting

1. **Gazebo doesn't start**: Ensure Gazebo Classic is installed
2. **Robot doesn't move**: Check if cmd_vel topic is being published
3. **SLAM not working**: Verify lidar data on /scan topic
4. **Navigation fails**: Ensure map is loaded and initial pose is set

## Customization

- Modify robot dimensions in `turtle_bot.urdf.xacro`
- Adjust sensor parameters in `turtle_bot_gazebo.urdf.xacro`
- Tune navigation parameters in `nav2_params.yaml`
- Create custom worlds in `turtle_bot_gazebo/worlds/`

## Topics and Services

Key topics:
- `/cmd_vel`: Robot velocity commands
- `/scan`: Lidar data
- `/odom`: Odometry data
- `/map`: SLAM-generated map
- `/camera/image_raw`: Camera feed

## License

Apache-2.0
