#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument, 
    IncludeLaunchDescription, 
    TimerAction,
    ExecuteProcess
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    # Launch configuration variables
    use_sim_time = LaunchConfiguration('use_sim_time')
    test_duration = LaunchConfiguration('test_duration')
    world_file = LaunchConfiguration('world_file')

    # Declare launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    declare_test_duration_cmd = DeclareLaunchArgument(
        'test_duration',
        default_value='300',  # 5 minutes
        description='Test duration in seconds')

    declare_world_file_cmd = DeclareLaunchArgument(
        'world_file',
        default_value=os.path.join(get_package_share_directory('turtle_bot_gazebo'),
                                   'worlds', 'turtle_world.world'),
        description='Full path to world file to load')

    # Main exploration launch
    exploration_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('turtle_bot_exploration'), 'launch'),
            '/autonomous_exploration.launch.py'
        ]),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'world_file': world_file
        }.items()
    )

    # Test monitoring node
    test_monitor_node = Node(
        package='turtle_bot_exploration',
        executable='exploration_test_monitor',
        name='exploration_test_monitor',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'test_duration': test_duration}
        ],
        output='screen',
        condition=lambda context: False  # Disable for now, implement if needed
    )

    # Automatic test termination
    test_termination = ExecuteProcess(
        cmd=['timeout', test_duration, 'sleep', 'inf'],
        output='screen',
        condition=lambda context: False  # Disable automatic termination for manual testing
    )

    return LaunchDescription([
        # Declare launch arguments
        declare_use_sim_time_cmd,
        declare_test_duration_cmd,
        declare_world_file_cmd,

        # Launch exploration system
        exploration_launch,
        
        # Optional test monitoring (disabled for now)
        # test_monitor_node,
        # test_termination,
    ])
