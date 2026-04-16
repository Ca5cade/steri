#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Package directories
    turtle_bot_nav_dir = get_package_share_directory('turtle_bot_navigation')
    turtle_bot_exp_dir = get_package_share_directory('turtle_bot_exploration')
    
    # Launch arguments
    use_sim_time = LaunchConfiguration('use_sim_time')
    params_file = LaunchConfiguration('params_file')
    log_level = LaunchConfiguration('log_level')
    
    # Declare launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')
    
    declare_params_file_cmd = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(turtle_bot_nav_dir, 'config', 'nav2_params.yaml'),
        description='Full path to the ROS2 parameters file to use')
    
    declare_log_level_cmd = DeclareLaunchArgument(
        'log_level', 
        default_value='info',
        description='log level')

    # Navigation2 individual nodes with enhanced safety parameters
    controller_server_node = Node(
        package='nav2_controller',
        executable='controller_server',
        name='controller_server',
        output='screen',
        parameters=[params_file, {'use_sim_time': use_sim_time}],
        arguments=['--ros-args', '--log-level', log_level]
    )

    planner_server_node = Node(
        package='nav2_planner',
        executable='planner_server',
        name='planner_server',
        output='screen',
        parameters=[params_file, {'use_sim_time': use_sim_time}],
        arguments=['--ros-args', '--log-level', log_level]
    )

    recoveries_server_node = Node(
        package='nav2_recoveries',
        executable='recoveries_server',
        name='recoveries_server',
        output='screen',
        parameters=[params_file, {'use_sim_time': use_sim_time}],
        arguments=['--ros-args', '--log-level', log_level]
    )

    bt_navigator_node = Node(
        package='nav2_bt_navigator',
        executable='bt_navigator',
        name='bt_navigator',
        output='screen',
        parameters=[params_file, {'use_sim_time': use_sim_time}],
        arguments=['--ros-args', '--log-level', log_level]
    )

    waypoint_follower_node = Node(
        package='nav2_waypoint_follower',
        executable='waypoint_follower',
        name='waypoint_follower',
        output='screen',
        parameters=[params_file, {'use_sim_time': use_sim_time}],
        arguments=['--ros-args', '--log-level', log_level]
    )

    velocity_smoother_node = Node(
        package='nav2_velocity_smoother',
        executable='velocity_smoother',
        name='velocity_smoother',
        output='screen',
        parameters=[params_file, {'use_sim_time': use_sim_time}],
        arguments=['--ros-args', '--log-level', log_level]
    )

    lifecycle_manager_node = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_navigation',
        output='screen',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'autostart': True},
            {'node_names': [
                'controller_server',
                'planner_server',
                'recoveries_server',
                'bt_navigator',
                'waypoint_follower',
                'velocity_smoother'
            ]}
        ],
        arguments=['--ros-args', '--log-level', log_level]
    )

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

    # Create the launch description and populate
    ld = LaunchDescription()

    # Add the commands to the launch description
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_params_file_cmd)
    ld.add_action(declare_log_level_cmd)

    # Add navigation nodes
    ld.add_action(controller_server_node)
    ld.add_action(planner_server_node)
    ld.add_action(recoveries_server_node)
    ld.add_action(bt_navigator_node)
    ld.add_action(waypoint_follower_node)
    ld.add_action(velocity_smoother_node)
    ld.add_action(lifecycle_manager_node)

    # Add exploration nodes
    ld.add_action(frontier_detection_node)
    ld.add_action(exploration_coordinator_node)
    ld.add_action(rviz_node)

    return ld
