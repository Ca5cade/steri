#!/usr/bin/env python3

"""
Exploration Validation Script

This script provides validation procedures to ensure the exploration system
works correctly in different environments.
"""

import rclpy
from rclpy.node import Node
from nav_msgs.msg import OccupancyGrid
from std_msgs.msg import String
from visualization_msgs.msg import MarkerArray
import time
import math


class ExplorationValidator(Node):
    def __init__(self):
        super().__init__('exploration_validator')
        
        # Subscribers
        self.map_sub = self.create_subscription(
            OccupancyGrid, '/map', self.map_callback, 10)
        self.status_sub = self.create_subscription(
            String, '/exploration/status', self.status_callback, 10)
        self.frontiers_sub = self.create_subscription(
            MarkerArray, '/exploration/frontiers', self.frontiers_callback, 10)
        
        # Validation state
        self.initial_map_coverage = None
        self.current_map_coverage = 0.0
        self.exploration_status = "UNKNOWN"
        self.frontier_count = 0
        self.start_time = time.time()
        self.last_coverage_update = time.time()
        
        # Validation criteria
        self.min_coverage_improvement = 0.2  # 20% improvement
        self.max_exploration_time = 300  # 5 minutes
        self.min_frontiers_detected = 1
        
        # Timer for periodic validation
        self.timer = self.create_timer(10.0, self.validate_exploration)
        
        self.get_logger().info("Exploration validator started")

    def map_callback(self, msg):
        """Process map updates and calculate coverage"""
        total_cells = len(msg.data)
        known_cells = sum(1 for cell in msg.data if cell != -1)
        
        if total_cells > 0:
            coverage = known_cells / total_cells
            
            if self.initial_map_coverage is None:
                self.initial_map_coverage = coverage
                self.get_logger().info(f"Initial map coverage: {coverage:.2%}")
            
            self.current_map_coverage = coverage
            self.last_coverage_update = time.time()

    def status_callback(self, msg):
        """Process exploration status updates"""
        self.exploration_status = msg.data

    def frontiers_callback(self, msg):
        """Process frontier updates"""
        # Count active frontier markers
        active_frontiers = sum(1 for marker in msg.markers 
                             if marker.ns == "frontiers" and 
                                marker.action == marker.ADD)
        self.frontier_count = active_frontiers

    def validate_exploration(self):
        """Perform periodic validation checks"""
        current_time = time.time()
        elapsed_time = current_time - self.start_time
        
        self.get_logger().info(f"=== Exploration Validation Report ===")
        self.get_logger().info(f"Elapsed time: {elapsed_time:.1f}s")
        self.get_logger().info(f"Status: {self.exploration_status}")
        self.get_logger().info(f"Current coverage: {self.current_map_coverage:.2%}")
        self.get_logger().info(f"Active frontiers: {self.frontier_count}")
        
        # Validation checks
        validation_results = []
        
        # Check 1: Map coverage improvement
        if self.initial_map_coverage is not None:
            coverage_improvement = self.current_map_coverage - self.initial_map_coverage
            if coverage_improvement >= self.min_coverage_improvement:
                validation_results.append("✓ Coverage improvement sufficient")
            else:
                validation_results.append(f"✗ Coverage improvement insufficient: {coverage_improvement:.2%}")
        
        # Check 2: Frontier detection
        if self.frontier_count >= self.min_frontiers_detected:
            validation_results.append("✓ Frontiers being detected")
        else:
            validation_results.append("✗ No frontiers detected")
        
        # Check 3: Exploration progress
        time_since_update = current_time - self.last_coverage_update
        if time_since_update < 30:  # Recent map update
            validation_results.append("✓ Map being updated regularly")
        else:
            validation_results.append("✗ No recent map updates")
        
        # Check 4: Exploration completion
        if self.exploration_status == "EXPLORATION_COMPLETE":
            validation_results.append("✓ Exploration completed successfully")
        elif elapsed_time > self.max_exploration_time:
            validation_results.append("✗ Exploration timeout exceeded")
        else:
            validation_results.append("○ Exploration in progress")
        
        # Print validation results
        for result in validation_results:
            self.get_logger().info(result)
        
        # Overall assessment
        passed_checks = sum(1 for result in validation_results if result.startswith("✓"))
        total_checks = len([r for r in validation_results if not r.startswith("○")])
        
        if total_checks > 0:
            success_rate = passed_checks / total_checks
            if success_rate >= 0.75:
                self.get_logger().info(f"🎉 Validation PASSED ({passed_checks}/{total_checks} checks)")
            else:
                self.get_logger().warn(f"⚠️  Validation PARTIAL ({passed_checks}/{total_checks} checks)")
        
        self.get_logger().info("=====================================")


def main(args=None):
    rclpy.init(args=args)
    
    validator = ExplorationValidator()
    
    try:
        rclpy.spin(validator)
    except KeyboardInterrupt:
        validator.get_logger().info("Validation interrupted by user")
    finally:
        validator.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
