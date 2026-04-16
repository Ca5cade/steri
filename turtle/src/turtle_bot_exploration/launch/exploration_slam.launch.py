#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():

    # Include the simulation launch file
    simulation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_bringup'), 'launch'),
            '/turtle_bot_simulation.launch.py'
        ])
    )

    # Include the SLAM launch file
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_slam'), 'launch'),
            '/slam.launch.py'
        ])
    )

    # Include the exploration launch file
    exploration_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_exploration'), 'launch'),
            '/exploration.launch.py'
        ])
    )

    # Delay SLAM startup to allow simulation to initialize
    delayed_slam = TimerAction(
        period=10.0,
        actions=[slam_launch]
    )

    # Delay exploration startup to allow SLAM to initialize and create initial map
    delayed_exploration = TimerAction(
        period=20.0,
        actions=[exploration_launch]
    )

    return LaunchDescription([
        simulation_launch,
        delayed_slam,
        delayed_exploration,
    ])
