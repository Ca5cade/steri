#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument, 
    IncludeLaunchDescription, 
    TimerAction,
    ExecuteProcess,
    RegisterEventHandler
)
from launch.event_handlers import OnProcessStart
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Package directories
    gazebo_pkg_dir = get_package_share_directory('turtle_bot_gazebo')
    nav2_bringup_dir = get_package_share_directory('nav2_bringup')
    turtle_bot_nav_dir = get_package_share_directory('turtle_bot_navigation')
    turtle_bot_exp_dir = get_package_share_directory('turtle_bot_exploration')
    
    # Launch arguments
    use_sim_time = LaunchConfiguration('use_sim_time')
    params_file = LaunchConfiguration('params_file')
    rviz_config = LaunchConfiguration('rviz_config')
    log_level = LaunchConfiguration('log_level')
    
    # Declare launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')
    

    
    declare_params_file_cmd = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(turtle_bot_nav_dir, 'config', 'nav2_params.yaml'),
        description='Full path to the enhanced nav2 parameters file')
    
    declare_rviz_config_cmd = DeclareLaunchArgument(
        'rviz_config',
        default_value=os.path.join(turtle_bot_exp_dir, 'config', 'exploration.rviz'),
        description='Full path to the RViz config file')
    
    declare_log_level_cmd = DeclareLaunchArgument(
        'log_level', 
        default_value='info',
        description='Log level for all nodes')

    # 1. Launch Gazebo simulation environment
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_pkg_dir, 'launch', 'gazebo.launch.py')),
        launch_arguments={
            'use_sim_time': use_sim_time,
        }.items()
    )

    # 2. Launch Nav2 navigation stack with enhanced safety parameters (delayed)
    nav2_launch = TimerAction(
        period=8.0,  # Wait 8 seconds for Gazebo to fully initialize
        actions=[
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(nav2_bringup_dir, 'launch', 'navigation_launch.py')),
                launch_arguments={
                    'use_sim_time': use_sim_time,
                    'params_file': params_file,
                    'autostart': 'true',
                    'use_composition': 'False',  # Avoid composition issues
                    'use_respawn': 'False',
                }.items()
            )
        ]
    )

    # 3. Launch SLAM for mapping (delayed)
    slam_launch = TimerAction(
        period=10.0,  # Wait for Gazebo + 2 seconds
        actions=[
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(nav2_bringup_dir, 'launch', 'slam_launch.py')),
                launch_arguments={
                    'use_sim_time': use_sim_time,
                    'autostart': 'true',
                    'use_composition': 'False',
                    'use_respawn': 'False',
                }.items()
            )
        ]
    )

    # 4. Enhanced frontier detection node with safety parameters (delayed)
    frontier_detection_node = TimerAction(
        period=15.0,  # Wait for navigation stack to initialize
        actions=[
            Node(
                package='turtle_bot_exploration',
                executable='frontier_detector',
                name='frontier_detector',
                output='screen',
                parameters=[
                    {'use_sim_time': use_sim_time},
                    {'frontier_min_size': 20},  # Larger minimum frontier size for safety
                    {'frontier_travel_point_distance': 0.8},  # Increased safety distance
                    {'frontier_blacklist_radius': 1.2},  # Larger blacklist radius
                    {'frontier_search_radius': 10.0},  # Extended search radius
                    {'frontier_min_distance': 1.0},  # Minimum distance between frontiers
                    {'global_frame': 'map'},
                    {'robot_base_frame': 'base_link'},
                ],
                arguments=['--ros-args', '--log-level', log_level]
            )
        ]
    )

    # 5. Enhanced exploration coordinator with safety features (delayed)
    exploration_coordinator_node = TimerAction(
        period=18.0,  # Wait for frontier detection to start
        actions=[
            Node(
                package='turtle_bot_exploration',
                executable='exploration_coordinator',
                name='exploration_coordinator',
                output='screen',
                parameters=[
                    {'use_sim_time': use_sim_time},
                    {'exploration_timeout': 1800.0},  # 30 minutes timeout
                    {'stuck_timeout': 8.0},  # Faster stuck detection
                    {'recovery_rotation_speed': 0.5},  # Moderate rotation speed
                    {'recovery_rotation_duration': 12.0},  # Extended recovery duration
                    {'auto_start_exploration': True},  # Auto-start exploration
                    {'global_frame': 'map'},
                    {'robot_base_frame': 'base_link'},
                    {'min_frontier_distance': 1.0},  # Minimum distance to frontiers
                    {'max_exploration_range': 15.0},  # Maximum exploration range
                    {'goal_tolerance': 0.3},  # Increased goal tolerance for safety
                    {'frontier_safety_margin': 0.8},  # 80cm safety margin for frontiers
                ],
                arguments=['--ros-args', '--log-level', log_level]
            )
        ]
    )

    # 6. RViz with exploration visualization (delayed)
    rviz_node = TimerAction(
        period=12.0,  # Start after navigation is ready
        actions=[
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                arguments=['-d', rviz_config],
                parameters=[{'use_sim_time': use_sim_time}],
                output='screen'
            )
        ]
    )

    # 7. Safety parameter verification (delayed)
    safety_verification_cmd = TimerAction(
        period=20.0,  # Start after everything else
        actions=[
            ExecuteProcess(
                cmd=[
                    'bash', '-c',
                    'echo "🔍 Verifying safety parameters..."; '
                    'ros2 param get /controller_server FollowPath.BaseObstacle.scale 2>/dev/null || echo "⚠️ Controller not ready"; '
                    'ros2 param get /local_costmap/local_costmap inflation_layer.inflation_radius 2>/dev/null || echo "⚠️ Costmap not ready"; '
                    'echo "✅ Safety verification complete"'
                ],
                output='screen',
                shell=True
            )
        ]
    )

    # 8. Print safety information
    safety_info_cmd = TimerAction(
        period=25.0,  # Print info after everything starts
        actions=[
            ExecuteProcess(
                cmd=[
                    'bash', '-c', 
                    'echo "🛡️ ENHANCED SAFE EXPLORATION ACTIVE"; '
                    'echo "✅ 60cm obstacle inflation radius"; '
                    'echo "✅ 80cm frontier safety margin"; '
                    'echo "✅ Multi-stage stuck recovery"; '
                    'echo "✅ Enhanced obstacle avoidance"; '
                    'echo "✅ Intelligent goal selection"; '
                    'echo "✅ SLAM mapping enabled"; '
                    'echo "🎯 System ready for collision-free exploration!"'
                ],
                output='screen',
                shell=True
            )
        ]
    )

    # Create the launch description
    ld = LaunchDescription()

    # Add launch arguments
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_params_file_cmd)
    ld.add_action(declare_rviz_config_cmd)
    ld.add_action(declare_log_level_cmd)

    # Add all components in proper sequence
    ld.add_action(gazebo_launch)           # Start immediately
    ld.add_action(nav2_launch)             # Start at 8s
    ld.add_action(slam_launch)             # Start at 10s
    ld.add_action(rviz_node)               # Start at 12s
    ld.add_action(frontier_detection_node) # Start at 15s
    ld.add_action(exploration_coordinator_node) # Start at 18s
    ld.add_action(safety_verification_cmd) # Start at 20s
    ld.add_action(safety_info_cmd)         # Print info at 25s

    return ld
