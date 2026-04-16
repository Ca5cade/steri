#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


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

    # Delay SLAM startup to allow simulation to initialize
    delayed_slam = TimerAction(
        period=10.0,
        actions=[slam_launch]
    )

    return LaunchDescription([
        simulation_launch,
        delayed_slam,
    ])
