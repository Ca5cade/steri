#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, TimerAction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    # Get the launch directory
    exploration_params_file = LaunchConfiguration('exploration_params_file')
    use_sim_time = LaunchConfiguration('use_sim_time')

    declare_exploration_params_file_cmd = DeclareLaunchArgument(
        'exploration_params_file',
        default_value=os.path.join(get_package_share_directory("turtle_bot_exploration"),
                                   'config', 'exploration_params.yaml'),
        description='Full path to the ROS2 parameters file to use for exploration nodes')

    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    # Frontier Detection Node
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

    # Exploration Coordinator Node
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

    # Delay exploration coordinator to allow frontier detector to initialize
    delayed_exploration_coordinator = TimerAction(
        period=3.0,
        actions=[exploration_coordinator_node]
    )

    return LaunchDescription([
        # Declare the launch options
        declare_exploration_params_file_cmd,
        declare_use_sim_time_cmd,

        # Add the nodes
        frontier_detector_node,
        delayed_exploration_coordinator,
    ])
