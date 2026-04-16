#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():

    # Include the Gazebo launch file
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_gazebo'), 'launch'),
            '/gazebo.launch.py'
        ])
    )

    # Launch RViz
    rviz_config_file = os.path.join(
        get_package_share_directory('turtle_bot_bringup'),
        'launch',
        'turtle_bot.rviz'
    )
    
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file] if os.path.exists(rviz_config_file) else [],
        parameters=[{'use_sim_time': True}],
        output='screen',
        respawn=False
    )

    # Delay RViz startup to allow Gazebo to initialize
    delayed_rviz = TimerAction(
        period=5.0,
        actions=[rviz_node]
    )

    return LaunchDescription([
        gazebo_launch,
        delayed_rviz,
    ])
