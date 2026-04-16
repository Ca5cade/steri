# TurtleBot Autonomous Exploration

This package provides autonomous exploration functionality for TurtleBot-like robots using frontier-based exploration algorithms. The system integrates with ROS2 Navigation Stack (Nav2) and SLAM to enable robots to autonomously explore and map unknown environments.

## Features

- **Frontier Detection**: Identifies unexplored boundaries between known free space and unknown areas
- **Intelligent Goal Selection**: Chooses optimal exploration targets based on information gain and distance
- **SLAM Integration**: Works seamlessly with slam_toolbox for real-time mapping
- **Navigation Integration**: Uses Nav2 action servers for robust path planning and execution
- **Visualization**: Real-time RViz visualization of frontiers, exploration goals, and progress
- **Recovery Behaviors**: Handles stuck situations and edge cases
- **Completion Detection**: Automatically detects when exploration is complete

## Package Structure

```
turtle_bot_exploration/
├── src/
│   ├── frontier_detector.cpp          # Frontier detection algorithm
│   └── exploration_coordinator.cpp    # Main exploration state machine
├── include/turtle_bot_exploration/
│   ├── frontier_detector.hpp
│   └── exploration_coordinator.hpp
├── launch/
│   ├── exploration.launch.py          # Basic exploration nodes
│   ├── exploration_slam.launch.py     # Exploration + SLAM
│   ├── autonomous_exploration.launch.py # Complete system
│   └── test_exploration.launch.py     # Testing configuration
├── config/
│   ├── exploration_params.yaml        # Exploration parameters
│   └── exploration.rviz               # RViz configuration
├── scripts/
│   └── validate_exploration.py        # Validation script
└── README.md
```

## Dependencies

- ROS2 (tested with Humble)
- Nav2 Navigation Stack
- slam_toolbox
- OpenCV
- tf2
- visualization_msgs

## Building the Package

1. **Clone and build the workspace:**
   ```bash
   cd /home/haythem/turtle/src
   colcon build --packages-select turtle_bot_exploration
   source install/setup.bash
   ```

2. **Check for build errors:**
   ```bash
   colcon build --packages-select turtle_bot_exploration --cmake-args -DCMAKE_BUILD_TYPE=Debug
   ```

## Usage

### Quick Start - Complete Autonomous Exploration

Launch the complete system with simulation, SLAM, navigation, and exploration:

```bash
ros2 launch turtle_bot_exploration autonomous_exploration.launch.py
```

This will:
1. Start Gazebo simulation with TurtleBot
2. Launch RViz with exploration visualization
3. Start SLAM for mapping
4. Initialize Nav2 navigation stack
5. Begin autonomous exploration

### Individual Components

**Basic exploration nodes only:**
```bash
ros2 launch turtle_bot_exploration exploration.launch.py
```

**Exploration with SLAM (no navigation):**
```bash
ros2 launch turtle_bot_exploration exploration_slam.launch.py
```

### Testing and Validation

**Run exploration validation:**
```bash
# In one terminal - launch exploration
ros2 launch turtle_bot_exploration autonomous_exploration.launch.py

# In another terminal - run validation
ros2 run turtle_bot_exploration validate_exploration.py
```

**Test with specific duration:**
```bash
ros2 launch turtle_bot_exploration test_exploration.launch.py test_duration:=600
```

## Configuration

### Exploration Parameters

Edit `config/exploration_params.yaml` to tune exploration behavior:

```yaml
frontier_detector:
  ros__parameters:
    min_frontier_size: 15.0              # Minimum frontier size
    max_frontier_distance: 8.0           # Maximum exploration range
    information_radius: 1.5              # Information gain calculation radius

exploration_coordinator:
  ros__parameters:
    exploration_timeout: 600.0           # Maximum exploration time
    stuck_timeout: 20.0                  # Stuck detection timeout
    auto_start_exploration: true         # Auto-start when ready
```

### RViz Visualization

The package includes a custom RViz configuration (`config/exploration.rviz`) that displays:
- Map being built by SLAM
- Laser scan data
- Robot model and pose
- Detected frontiers (colored by information gain)
- Current exploration goal
- Navigation path

## Monitoring Exploration

### Topics

- `/exploration/frontiers` - Frontier markers for visualization
- `/exploration/status` - Current exploration state
- `/exploration/markers` - Exploration goal markers
- `/map` - SLAM-generated map
- `/scan` - Laser scan data

### Exploration States

- `IDLE` - Waiting to start
- `DETECTING_ENVIRONMENT` - Analyzing environment
- `EXPLORING` - Active exploration
- `NAVIGATING_TO_FRONTIER` - Moving to exploration goal
- `STUCK_RECOVERY` - Recovering from stuck situation
- `EXPLORATION_COMPLETE` - Finished exploring
- `ERROR` - Error state

## Troubleshooting

### Common Issues

1. **No frontiers detected:**
   - Check if SLAM is running and publishing map
   - Verify laser scan data is available
   - Adjust `min_frontier_size` parameter

2. **Robot not moving:**
   - Ensure Nav2 navigation stack is running
   - Check if navigation goals are being published
   - Verify robot localization is working

3. **Exploration not starting:**
   - Check `auto_start_exploration` parameter
   - Verify minimum map coverage is reached
   - Ensure all required nodes are running

### Debug Commands

```bash
# Check node status
ros2 node list | grep exploration

# Monitor exploration status
ros2 topic echo /exploration/status

# View frontier count
ros2 topic echo /exploration/frontiers --field markers

# Check navigation goals
ros2 topic echo /navigate_to_pose/_action/goal
```

## Validation Criteria

The validation script checks:
- ✓ Map coverage improvement (>20%)
- ✓ Frontier detection working
- ✓ Regular map updates
- ✓ Successful exploration completion

## Performance Tips

1. **Adjust exploration parameters** based on environment size
2. **Tune SLAM parameters** for better map quality
3. **Optimize navigation parameters** for smoother movement
4. **Use appropriate world files** for testing different scenarios

## Contributing

When modifying the exploration algorithms:
1. Test with the validation script
2. Verify performance in different environments
3. Update parameters documentation
4. Run full system tests

## License

Apache 2.0 License
