#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, GroupAction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node, SetParameter
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Get package directories
    turtle_bot_exploration_dir = get_package_share_directory('turtle_bot_exploration')
    turtle_bot_navigation_dir = get_package_share_directory('turtle_bot_navigation')
    turtle_bot_slam_dir = get_package_share_directory('turtle_bot_slam')
    turtle_bot_gazebo_dir = get_package_share_directory('turtle_bot_gazebo')

    # Launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    declare_world_cmd = DeclareLaunchArgument(
        'world',
        default_value=os.path.join(turtle_bot_gazebo_dir, 'worlds', 'exploration_world.world'),
        description='Full path to world file to load')

    declare_map_yaml_cmd = DeclareLaunchArgument(
        'map',
        default_value='',
        description='Full path to map yaml file to load')

    declare_params_file_cmd = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(turtle_bot_exploration_dir, 'config', 'optimized_exploration_params.yaml'),
        description='Full path to optimized exploration parameters file')

    declare_nav_params_file_cmd = DeclareLaunchArgument(
        'nav_params_file',
        default_value=os.path.join(turtle_bot_navigation_dir, 'config', 'orientation_free_params.yaml'),
        description='Full path to orientation-free navigation parameters file')

    declare_slam_params_file_cmd = DeclareLaunchArgument(
        'slam_params_file',
        default_value=os.path.join(turtle_bot_slam_dir, 'config', 'slam_params.yaml'),
        description='Full path to SLAM parameters file')

    declare_rviz_config_file_cmd = DeclareLaunchArgument(
        'rviz_config_file',
        default_value=os.path.join(turtle_bot_exploration_dir, 'config', 'optimized_exploration.rviz'),
        description='Full path to RViz config file')

    declare_autostart_cmd = DeclareLaunchArgument(
        'autostart', 
        default_value='true',
        description='Automatically startup the nav2 stack')

    declare_log_level_cmd = DeclareLaunchArgument(
        'log_level', 
        default_value='info',
        description='log level')

    # Launch configurations
    use_sim_time = LaunchConfiguration('use_sim_time')
    world = LaunchConfiguration('world')
    map_yaml_file = LaunchConfiguration('map')
    params_file = LaunchConfiguration('params_file')
    nav_params_file = LaunchConfiguration('nav_params_file')
    slam_params_file = LaunchConfiguration('slam_params_file')
    rviz_config_file = LaunchConfiguration('rviz_config_file')
    autostart = LaunchConfiguration('autostart')
    log_level = LaunchConfiguration('log_level')

    # Set use_sim_time parameter
    set_use_sim_time = SetParameter(name='use_sim_time', value=use_sim_time)

    # Gazebo launch
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('turtle_bot_gazebo'),
                'launch',
                'gazebo.launch.py'
            ])
        ]),
        launch_arguments={
            'world': world,
            'use_sim_time': use_sim_time,
        }.items()
    )

    # Robot state publisher
    robot_state_publisher_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('turtle_bot_description'),
                'launch',
                'robot_state_publisher.launch.py'
            ])
        ]),
        launch_arguments={
            'use_sim_time': use_sim_time,
        }.items()
    )

    # SLAM launch (using existing slam.launch.py)
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('turtle_bot_slam'),
                'launch',
                'slam.launch.py'
            ])
        ]),
        launch_arguments={
            'use_sim_time': use_sim_time,
        }.items()
    )

    # Navigation launch with orientation-free parameters
    navigation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('turtle_bot_navigation'),
                'launch',
                'navigation.launch.py'
            ])
        ]),
        launch_arguments={
            'params_file': nav_params_file,
            'use_sim_time': use_sim_time,
            'autostart': autostart,
            'map': map_yaml_file,
        }.items()
    )

    # Enhanced Exploration Coordinator with optimization features
    exploration_coordinator_node = Node(
        package='turtle_bot_exploration',
        executable='exploration_coordinator',
        name='exploration_coordinator',
        output='screen',
        parameters=[
            params_file,
            {
                'use_sim_time': use_sim_time,
                # Enhanced optimization parameters
                'enable_dynamic_tolerance': True,
                'velocity_based_tolerance': True,
                'min_dynamic_tolerance': 0.8,
                'max_dynamic_tolerance': 3.0,
                'velocity_tolerance_factor': 2.0,
                'environmental_adaptation': True,
                'performance_monitoring': True,
                'adaptive_parameter_tuning': True,
                'goal_completion_smoothing': True,
                'transition_optimization': True,
                # Ultra-aggressive goal completion
                'goal_reached_tolerance': 2.0,
                'goal_timeout': 30.0,
                'force_new_goal_distance': 2.5,
                'goal_tolerance': 1.5,
                'stuck_distance_threshold': 0.15,
            }
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # Performance Monitor
    performance_monitor_node = Node(
        package='turtle_bot_exploration',
        executable='performance_monitor',
        name='performance_monitor',
        output='screen',
        parameters=[
            {
                'use_sim_time': use_sim_time,
                'monitoring_frequency': 2.0,
                'performance_window_size': 10,
                'efficiency_threshold': 0.7,
                'enable_adaptive_tuning': True,
                'publish_detailed_metrics': True,
            }
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # Frontier Detector with enhanced parameters
    frontier_detector_node = Node(
        package='turtle_bot_exploration',
        executable='frontier_detector',
        name='frontier_detector',
        output='screen',
        parameters=[
            params_file,
            {
                'use_sim_time': use_sim_time,
                'min_frontier_size': 8,
                'max_frontier_distance': 12.0,
                'frontier_travel_point_distance': 0.8,
                'information_radius': 2.0,
            }
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # Intelligent Frontier Selector
    intelligent_frontier_selector_node = Node(
        package='turtle_bot_exploration',
        executable='intelligent_frontier_selector',
        name='intelligent_frontier_selector',
        output='screen',
        parameters=[
            params_file,
            {
                'use_sim_time': use_sim_time,
                'selection_frequency': 2.0,
                'enable_multi_frontier_planning': True,
                'lookahead_distance': 8.0,
                'clustering_distance_threshold': 3.0,
                'time_efficiency_weight': 0.3,
                'exploration_value_weight': 0.4,
                'accessibility_weight': 0.3,
            }
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # Emergency Stop Controller with enhanced safety
    emergency_stop_node = Node(
        package='turtle_bot_exploration',
        executable='emergency_stop_controller',
        name='emergency_stop_controller',
        output='screen',
        parameters=[
            {
                'use_sim_time': use_sim_time,
                'emergency_stop_distance': 0.15,
                'warning_distance': 0.3,
                'monitoring_frequency': 50.0,
                'recovery_distance': 0.5,
                'enable_predictive_stopping': True,
            }
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

    # RViz with optimized exploration visualization
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        condition=IfCondition(LaunchConfiguration('use_rviz', default='true'))
    )

    # Create the launch description and populate
    ld = LaunchDescription()

    # Add launch arguments
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_world_cmd)
    ld.add_action(declare_map_yaml_cmd)
    ld.add_action(declare_params_file_cmd)
    ld.add_action(declare_nav_params_file_cmd)
    ld.add_action(declare_slam_params_file_cmd)
    ld.add_action(declare_rviz_config_file_cmd)
    ld.add_action(declare_autostart_cmd)
    ld.add_action(declare_log_level_cmd)

    # Add parameter setting
    ld.add_action(set_use_sim_time)

    # Add launch includes
    ld.add_action(gazebo_launch)
    ld.add_action(robot_state_publisher_launch)
    ld.add_action(slam_launch)
    ld.add_action(navigation_launch)

    # Add exploration nodes
    ld.add_action(exploration_coordinator_node)
    ld.add_action(performance_monitor_node)
    ld.add_action(frontier_detector_node)
    ld.add_action(intelligent_frontier_selector_node)
    ld.add_action(emergency_stop_node)
    ld.add_action(rviz_node)

    return ld
