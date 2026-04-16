#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument, 
    IncludeLaunchDescription, 
    TimerAction,
    GroupAction
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    # Launch configuration variables
    use_sim_time = LaunchConfiguration('use_sim_time')
    exploration_params_file = LaunchConfiguration('exploration_params_file')
    slam_params_file = LaunchConfiguration('slam_params_file')
    nav_params_file = LaunchConfiguration('nav_params_file')

    # Declare launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    declare_exploration_params_file_cmd = DeclareLaunchArgument(
        'exploration_params_file',
        default_value=os.path.join(get_package_share_directory('turtle_bot_exploration'),
                                   'config', 'exploration_params.yaml'),
        description='Full path to exploration parameters file')

    declare_slam_params_file_cmd = DeclareLaunchArgument(
        'slam_params_file',
        default_value=os.path.join(get_package_share_directory('turtle_bot_slam'),
                                   'config', 'slam_params.yaml'),
        description='Full path to SLAM parameters file')

    declare_nav_params_file_cmd = DeclareLaunchArgument(
        'nav_params_file',
        default_value=os.path.join(get_package_share_directory('turtle_bot_navigation'),
                                   'config', 'nav2_params.yaml'),
        description='Full path to navigation parameters file')

    # 1. Simulation (Gazebo + Robot State Publisher)
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_gazebo'), 'launch'),
            '/gazebo.launch.py'
        ]),
        launch_arguments={'use_sim_time': use_sim_time}.items()
    )

    # Custom RViz with exploration configuration
    rviz_config_file = os.path.join(
        get_package_share_directory('turtle_bot_exploration'),
        'config',
        'exploration.rviz'
    )
    
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen',
        respawn=False
    )

    delayed_rviz = TimerAction(
        period=3.0,  # Faster RViz startup
        actions=[rviz_node]
    )

    # 2. SLAM (optimized timing)
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_slam'), 'launch'),
            '/slam.launch.py'
        ]),
        launch_arguments={
            'slam_params_file': slam_params_file,
            'use_sim_time': use_sim_time
        }.items()
    )

    delayed_slam = TimerAction(
        period=6.0,  # Faster SLAM startup
        actions=[slam_launch]
    )

    # 3. Navigation Stack (optimized timing)
    nav_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('nav2_bringup'), 'launch'),
            '/navigation_launch.py'
        ]),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'params_file': nav_params_file,
            'autostart': 'true'
        }.items()
    )

    delayed_nav = TimerAction(
        period=12.0,  # Faster navigation startup
        actions=[nav_launch]
    )

    # 4. Exploration Nodes (optimized timing)
    frontier_detector_node = Node(
        package='turtle_bot_exploration',
        executable='frontier_detector',
        name='frontier_detector',
        parameters=[
            exploration_params_file,
            {'use_sim_time': use_sim_time}
        ],
        output='screen',
        respawn=True,
        respawn_delay=1.0  # Faster respawn
    )

    exploration_coordinator_node = Node(
        package='turtle_bot_exploration',
        executable='exploration_coordinator',
        name='exploration_coordinator',
        parameters=[
            exploration_params_file,
            {'use_sim_time': use_sim_time}
        ],
        output='screen',
        respawn=True,
        respawn_delay=1.0  # Faster respawn
    )

    exploration_group = GroupAction([
        frontier_detector_node,
        TimerAction(
            period=2.0,  # Faster coordinator startup
            actions=[exploration_coordinator_node]
        )
    ])

    delayed_exploration = TimerAction(
        period=18.0,  # Faster exploration startup
        actions=[exploration_group]
    )

    return LaunchDescription([
        # Declare launch arguments
        declare_use_sim_time_cmd,
        declare_exploration_params_file_cmd,
        declare_slam_params_file_cmd,
        declare_nav_params_file_cmd,

        # Optimized launch sequence for speed
        gazebo_launch,         # t=0: Start Gazebo simulation
        delayed_rviz,          # t=3: Start RViz (faster)
        delayed_slam,          # t=6: Start SLAM (faster)
        delayed_nav,           # t=12: Start navigation (faster)
        delayed_exploration,   # t=18: Start exploration (faster)
    ])
