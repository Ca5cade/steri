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
        period=5.0,
        actions=[rviz_node]
    )

    # 2. SLAM (delayed to allow simulation to start)
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
        period=8.0,
        actions=[slam_launch]
    )

    # 3. Navigation Stack (delayed to allow SLAM to initialize)
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
        period=18.0,  # Extra time for SLAM to build initial map
        actions=[nav_launch]
    )

    # 4. Exploration Nodes (delayed to allow navigation to be ready)
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
        respawn_delay=2.0
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
        respawn_delay=2.0
    )

    exploration_group = GroupAction([
        frontier_detector_node,
        TimerAction(
            period=5.0,  # Extra delay for frontier detector to initialize
            actions=[exploration_coordinator_node]
        )
    ])

    delayed_exploration = TimerAction(
        period=30.0,  # Extra time for navigation to be fully ready
        actions=[exploration_group]
    )

    return LaunchDescription([
        # Declare launch arguments
        declare_use_sim_time_cmd,
        declare_exploration_params_file_cmd,
        declare_slam_params_file_cmd,
        declare_nav_params_file_cmd,

        # Launch sequence with extra timing for precision
        gazebo_launch,         # t=0: Start Gazebo simulation
        delayed_rviz,          # t=5: Start RViz with exploration config
        delayed_slam,          # t=8: Start SLAM
        delayed_nav,           # t=18: Start navigation (extra time for SLAM)
        delayed_exploration,   # t=30: Start exploration (extra time for nav)
    ])
