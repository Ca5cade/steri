#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Package directories
    nav2_bringup_dir = get_package_share_directory('nav2_bringup')
    turtle_bot_nav_dir = get_package_share_directory('turtle_bot_navigation')
    turtle_bot_exp_dir = get_package_share_directory('turtle_bot_exploration')
    
    # Launch arguments
    use_sim_time = LaunchConfiguration('use_sim_time')
    map_yaml_file = LaunchConfiguration('map')
    params_file = LaunchConfiguration('params_file')
    autostart = LaunchConfiguration('autostart')
    use_composition = LaunchConfiguration('use_composition')
    use_respawn = LaunchConfiguration('use_respawn')
    log_level = LaunchConfiguration('log_level')
    
    # Declare launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')
    
    declare_map_yaml_cmd = DeclareLaunchArgument(
        'map',
        default_value='',
        description='Full path to map yaml file to load')
    
    declare_params_file_cmd = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(turtle_bot_nav_dir, 'config', 'nav2_params.yaml'),
        description='Full path to the ROS2 parameters file to use for all launched nodes')
    
    declare_autostart_cmd = DeclareLaunchArgument(
        'autostart', 
        default_value='true',
        description='Automatically startup the nav2 stack')
    
    declare_use_composition_cmd = DeclareLaunchArgument(
        'use_composition',
        default_value='False',
        description='Whether to use composed bringup')
    
    declare_use_respawn_cmd = DeclareLaunchArgument(
        'use_respawn', 
        default_value='False',
        description='Whether to respawn if a node crashes')
    
    declare_log_level_cmd = DeclareLaunchArgument(
        'log_level', 
        default_value='info',
        description='log level')

    # Navigation2 bringup
    nav2_bringup_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav2_bringup_dir, 'launch', 'bringup_launch.py')),
        launch_arguments={
            'map': map_yaml_file,
            'use_sim_time': use_sim_time,
            'params_file': params_file,
            'autostart': autostart,
            'use_composition': use_composition,
            'use_respawn': use_respawn,
        }.items())

    # Frontier detection node with enhanced safety
    frontier_detection_node = Node(
        package='turtle_bot_exploration',
        executable='frontier_detection_node',
        name='frontier_detection_node',
        output='screen',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'frontier_min_size': 15},  # Larger minimum frontier size
            {'frontier_travel_point_distance': 0.8},  # Increased safety distance
            {'frontier_blacklist_radius': 1.2},  # Larger blacklist radius
            {'frontier_search_radius': 8.0},  # Reasonable search radius
            {'frontier_min_distance': 1.0},  # Minimum distance between frontiers
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # Enhanced exploration coordinator with safety features
    exploration_coordinator_node = Node(
        package='turtle_bot_exploration',
        executable='exploration_coordinator_node',
        name='exploration_coordinator_node',
        output='screen',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'exploration_timeout': 1800.0},  # 30 minutes timeout
            {'stuck_timeout': 8.0},  # Reduced stuck timeout for faster detection
            {'recovery_rotation_speed': 0.5},  # Moderate rotation speed
            {'recovery_rotation_duration': 12.0},  # Extended recovery duration
            {'auto_start_exploration': True},
            {'global_frame': 'map'},
            {'robot_base_frame': 'base_link'},
            {'min_frontier_distance': 1.0},  # Minimum distance to frontiers
            {'max_exploration_range': 15.0},  # Maximum exploration range
            {'goal_tolerance': 0.3},  # Increased goal tolerance
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # RViz for visualization
    rviz_config_file = PathJoinSubstitution([
        FindPackageShare('turtle_bot_exploration'),
        'config',
        'exploration.rviz'
    ])
    
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen'
    )

    # Safety monitor node (optional - monitors robot status)
    safety_monitor_node = Node(
        package='turtle_bot_exploration',
        executable='safety_monitor_node',
        name='safety_monitor_node',
        output='screen',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'max_linear_velocity': 0.8},
            {'max_angular_velocity': 1.5},
            {'obstacle_distance_threshold': 0.5},
            {'emergency_stop_distance': 0.3},
        ],
        arguments=['--ros-args', '--log-level', log_level],
        condition=IfCondition('false')  # Disabled by default, enable if needed
    )

    # Create the launch description and populate
    ld = LaunchDescription()

    # Add the commands to the launch description
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_map_yaml_cmd)
    ld.add_action(declare_params_file_cmd)
    ld.add_action(declare_autostart_cmd)
    ld.add_action(declare_use_composition_cmd)
    ld.add_action(declare_use_respawn_cmd)
    ld.add_action(declare_log_level_cmd)

    # Add the actions to launch all nodes
    ld.add_action(nav2_bringup_cmd)
    ld.add_action(frontier_detection_node)
    ld.add_action(exploration_coordinator_node)
    ld.add_action(rviz_node)
    ld.add_action(safety_monitor_node)

    return ld
