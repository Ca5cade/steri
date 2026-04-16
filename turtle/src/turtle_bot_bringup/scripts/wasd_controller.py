#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from sensor_msgs.msg import LaserScan
import sys
import select
import termios
import tty
import threading
import time
import math
import numpy as np

class IncrementalWASDController(Node):
    def __init__(self):
        super().__init__('incremental_wasd_controller')

        # Publisher for cmd_vel - High frequency for smooth control
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)

        # Subscriber for lidar data - Obstacle detection
        self.scan_sub = self.create_subscription(
            LaserScan,
            '/scan',
            self.scan_callback,
            10
        )

        # Movement parameters - Optimized for smooth control
        self.max_linear_speed = 0.8   # m/s - Match navigation stack
        self.max_angular_speed = 1.5  # rad/s - Match navigation stack
        self.speed_increment = 0.05   # Finer speed control

        # Velocity smoothing parameters
        self.linear_acceleration = 1.5   # m/s² - Match URDF optimization
        self.angular_acceleration = 2.0  # rad/s² - Match URDF optimization
        self.deceleration_factor = 1.2   # Faster deceleration for responsiveness
        self.dead_zone_threshold = 0.02  # Minimum velocity to eliminate drift

        # Obstacle detection parameters
        self.obstacle_distance_threshold = 0.5  # meters - Stop distance from obstacles
        self.side_obstacle_threshold = 0.3      # meters - Side clearance for turning
        self.obstacle_check_angle = 45.0        # degrees - Forward obstacle detection cone
        self.emergency_stop_distance = 0.25     # meters - Emergency stop distance

        # Safety state variables
        self.obstacle_detected = False
        self.obstacle_direction = None  # 'front', 'left', 'right', 'back'
        self.last_scan_time = time.time()
        self.scan_timeout = 2.0  # seconds - Consider sensors failed if no data
        self.safety_override = False  # Emergency override flag

        # Control loop parameters - Match navigation frequency
        self.control_frequency = 50.0  # Hz - Match navigation stack
        self.control_period = 1.0 / self.control_frequency

        # Incremental movement parameters
        self.linear_increment = 0.3   # meters - Distance per key press
        self.angular_increment = 0.5  # radians - Rotation per key press (~28.6 degrees)
        self.movement_speed = 0.4     # m/s - Speed during incremental movement
        self.rotation_speed = 0.8     # rad/s - Speed during incremental rotation

        # Current movement state
        self.current_linear = 0.0
        self.current_angular = 0.0
        self.target_linear = 0.0
        self.target_angular = 0.0

        # Movement tracking for incremental control
        self.movement_queue = []  # Queue of pending movements
        self.current_movement = None  # Currently executing movement
        self.movement_start_time = 0.0
        self.movement_lock = threading.Lock()

        # Terminal settings
        self.settings = termios.tcgetattr(sys.stdin)

        # Control flags
        self.running = True

        # Start high-frequency control loop
        self.control_timer = self.create_timer(self.control_period, self.control_loop)

        # Start keyboard input thread
        self.input_thread = threading.Thread(target=self.keyboard_input_loop)
        self.input_thread.daemon = True
        self.input_thread.start()

        self.get_logger().info("🎮 Smooth WASD Controller with Obstacle Detection Started!")
        self.get_logger().info(f"Control frequency: {self.control_frequency} Hz")
        self.get_logger().info(f"Obstacle detection: {self.obstacle_distance_threshold}m threshold")
        self.print_instructions()
        
    def print_instructions(self):
        print("\n" + "="*60)
        print("🎮 INCREMENTAL WASD CONTROLLER WITH OBSTACLE DETECTION")
        print("="*60)
        print("Incremental Movement Controls:")
        print(f"  W - Move Forward {self.linear_increment}m")
        print(f"  S - Move Backward {self.linear_increment}m")
        print(f"  A - Turn Left {math.degrees(self.angular_increment):.1f}°")
        print(f"  D - Turn Right {math.degrees(self.angular_increment):.1f}°")
        print("  X - Stop Current Movement")
        print("  Z - Clear Movement Queue")
        print("  SPACE - Emergency Stop (All Movement)")
        print("\nSpeed Controls:")
        print("  Q - Increase Linear Speed")
        print("  E - Decrease Linear Speed")
        print("  R - Increase Angular Speed")
        print("  F - Decrease Angular Speed")
        print("\nSafety Controls:")
        print("  O - Toggle safety override (advanced)")
        print("  C - Clear obstacle detection")
        print("\nFeatures:")
        print("  ✅ Precise incremental movement")
        print("  ✅ Automatic obstacle stopping")
        print("  ✅ Smooth acceleration/deceleration")
        print("  ✅ Movement queuing and sequencing")
        print("  ✅ 50Hz control frequency")
        print("  ✅ Lidar-based collision avoidance")
        print("\nOther:")
        print("  H - Show this help")
        print("  I - Show current status")
        print("  ESC or Ctrl+C - Exit")
        print("="*60)
        print(f"Max Speeds: Linear={self.max_linear_speed:.1f} m/s, Angular={self.max_angular_speed:.1f} rad/s")
        print(f"Control Rate: {self.control_frequency} Hz")
        print("Ready for smooth input...")

    def scan_callback(self, msg):
        """Process lidar scan data for obstacle detection"""
        self.last_scan_time = time.time()

        if len(msg.ranges) == 0:
            return

        # Convert scan data to numpy array for processing
        ranges = np.array(msg.ranges)

        # Replace inf and nan values with max range
        ranges = np.where(np.isfinite(ranges), ranges, msg.range_max)

        # Calculate angle step
        angle_step = (msg.angle_max - msg.angle_min) / len(ranges)

        # Define detection sectors (in radians)
        front_angle = math.radians(self.obstacle_check_angle)

        # Check for obstacles in different directions
        obstacles = {
            'front': False,
            'left': False,
            'right': False,
            'back': False
        }

        # Front obstacle detection (forward cone)
        front_indices = []
        for i, range_val in enumerate(ranges):
            angle = msg.angle_min + i * angle_step
            if abs(angle) <= front_angle:  # Front cone
                front_indices.append(i)

        if front_indices:
            front_min_distance = np.min(ranges[front_indices])
            obstacles['front'] = front_min_distance < self.obstacle_distance_threshold

        # Left side detection (90 degrees left, ±15 degrees)
        left_angle_center = math.pi / 2
        left_indices = []
        for i, range_val in enumerate(ranges):
            angle = msg.angle_min + i * angle_step
            if abs(angle - left_angle_center) <= math.radians(15):
                left_indices.append(i)

        if left_indices:
            left_min_distance = np.min(ranges[left_indices])
            obstacles['left'] = left_min_distance < self.side_obstacle_threshold

        # Right side detection (90 degrees right, ±15 degrees)
        right_angle_center = -math.pi / 2
        right_indices = []
        for i, range_val in enumerate(ranges):
            angle = msg.angle_min + i * angle_step
            if abs(angle - right_angle_center) <= math.radians(15):
                right_indices.append(i)

        if right_indices:
            right_min_distance = np.min(ranges[right_indices])
            obstacles['right'] = right_min_distance < self.side_obstacle_threshold

        # Back detection (180 degrees, ±30 degrees)
        back_indices = []
        for i, range_val in enumerate(ranges):
            angle = msg.angle_min + i * angle_step
            if abs(abs(angle) - math.pi) <= math.radians(30):
                back_indices.append(i)

        if back_indices:
            back_min_distance = np.min(ranges[back_indices])
            obstacles['back'] = back_min_distance < self.obstacle_distance_threshold

        # Update obstacle state
        self.obstacle_detected = any(obstacles.values())

        # Determine primary obstacle direction
        if obstacles['front']:
            self.obstacle_direction = 'front'
        elif obstacles['left']:
            self.obstacle_direction = 'left'
        elif obstacles['right']:
            self.obstacle_direction = 'right'
        elif obstacles['back']:
            self.obstacle_direction = 'back'
        else:
            self.obstacle_direction = None

        # Emergency stop check (very close obstacles)
        if front_indices:
            emergency_distance = np.min(ranges[front_indices])
            if emergency_distance < self.emergency_stop_distance and not self.safety_override:
                self.emergency_stop_due_to_obstacle()

    def keyboard_input_loop(self):
        """Continuous keyboard input monitoring thread"""
        while self.running:
            try:
                # Check if input is available
                if select.select([sys.stdin], [], [], 0.01)[0]:
                    key = self.get_key().lower()
                    self.process_key(key)
                else:
                    # Small sleep to prevent excessive CPU usage
                    time.sleep(0.001)

            except Exception as e:
                self.get_logger().error(f"Error in keyboard input loop: {e}")
                break

    def get_key(self):
        """Get a single keypress without blocking"""
        tty.setraw(sys.stdin.fileno())
        key = sys.stdin.read(1)
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key

    def control_loop(self):
        """High-frequency incremental movement control loop - 50Hz"""
        # Check sensor timeout
        if time.time() - self.last_scan_time > self.scan_timeout:
            self.get_logger().warn("⚠️ Lidar sensor timeout - Operating without obstacle detection")

        # Process movement queue and execute current movement
        self.process_movement_queue()

        # Update current movement if one is active
        self.update_current_movement()

        # Apply obstacle detection safety checks
        self.apply_obstacle_safety()

        # Smooth velocity interpolation
        self.smooth_velocity_update()

        # Apply dead zone filtering
        self.apply_dead_zone()

        # Create and publish twist message
        twist = Twist()
        twist.linear.x = self.current_linear
        twist.angular.z = self.current_angular

        self.cmd_vel_pub.publish(twist)

    def process_movement_queue(self):
        """Process the movement queue and start new movements"""
        with self.movement_lock:
            # If no current movement and queue has items, start next movement
            if self.current_movement is None and self.movement_queue:
                self.current_movement = self.movement_queue.pop(0)
                self.movement_start_time = time.time()

                # Set target velocities for the new movement
                if self.current_movement['type'] == 'linear':
                    direction = 1 if self.current_movement['direction'] == 'forward' else -1
                    self.target_linear = direction * self.movement_speed
                    self.target_angular = 0.0
                elif self.current_movement['type'] == 'angular':
                    direction = 1 if self.current_movement['direction'] == 'left' else -1
                    self.target_linear = 0.0
                    self.target_angular = direction * self.rotation_speed

                print(f"🎯 Starting {self.current_movement['type']} movement: {self.current_movement['direction']}")

    def update_current_movement(self):
        """Update progress of current incremental movement"""
        if self.current_movement is None:
            # No movement active, ensure targets are zero
            self.target_linear = 0.0
            self.target_angular = 0.0
            return

        # Calculate movement progress
        elapsed_time = time.time() - self.movement_start_time

        if self.current_movement['type'] == 'linear':
            # Check if linear movement is complete
            distance_traveled = abs(self.movement_speed * elapsed_time)
            if distance_traveled >= self.linear_increment:
                self.complete_current_movement()
        elif self.current_movement['type'] == 'angular':
            # Check if angular movement is complete
            angle_traveled = abs(self.rotation_speed * elapsed_time)
            if angle_traveled >= self.angular_increment:
                self.complete_current_movement()

    def complete_current_movement(self):
        """Complete the current movement and stop"""
        if self.current_movement:
            print(f"✅ Completed {self.current_movement['type']} movement")

        with self.movement_lock:
            self.current_movement = None
            self.target_linear = 0.0
            self.target_angular = 0.0

    def add_movement_to_queue(self, movement_type, direction):
        """Add a new movement to the queue"""
        movement = {
            'type': movement_type,  # 'linear' or 'angular'
            'direction': direction,  # 'forward', 'backward', 'left', 'right'
            'timestamp': time.time()
        }

        with self.movement_lock:
            self.movement_queue.append(movement)
            print(f"📝 Queued {movement_type} movement: {direction} (Queue: {len(self.movement_queue)})")

    def apply_obstacle_safety(self):
        """Apply obstacle detection safety to incremental movements"""
        if self.safety_override:
            return  # Skip safety checks if override is active

        # Check if current movement should be stopped by obstacles
        movement_blocked = False
        block_reason = ""

        if self.obstacle_detected and self.obstacle_direction and self.current_movement:
            # Block forward movement if front obstacle detected
            if (self.target_linear > 0 and self.obstacle_direction == 'front' and
                self.current_movement['direction'] == 'forward'):
                self.target_linear = 0.0
                movement_blocked = True
                block_reason = "Front obstacle detected"
                self.complete_current_movement()

            # Block backward movement if back obstacle detected
            elif (self.target_linear < 0 and self.obstacle_direction == 'back' and
                  self.current_movement['direction'] == 'backward'):
                self.target_linear = 0.0
                movement_blocked = True
                block_reason = "Back obstacle detected"
                self.complete_current_movement()

            # Block left turn if left obstacle detected
            elif (self.target_angular > 0 and self.obstacle_direction == 'left' and
                  self.current_movement['direction'] == 'left'):
                self.target_angular = 0.0
                movement_blocked = True
                block_reason = "Left obstacle detected"
                self.complete_current_movement()

            # Block right turn if right obstacle detected
            elif (self.target_angular < 0 and self.obstacle_direction == 'right' and
                  self.current_movement['direction'] == 'right'):
                self.target_angular = 0.0
                movement_blocked = True
                block_reason = "Right obstacle detected"
                self.complete_current_movement()

        # Provide feedback when movement is blocked
        if movement_blocked and hasattr(self, '_last_block_message'):
            if time.time() - self._last_block_message > 2.0:  # Throttle messages
                print(f"🛑 Movement blocked: {block_reason}")
                self._last_block_message = time.time()
        elif movement_blocked:
            print(f"🛑 Movement blocked: {block_reason}")
            self._last_block_message = time.time()

    def smooth_velocity_update(self):
        """Smooth velocity interpolation with acceleration limits"""
        dt = self.control_period

        # Linear velocity smoothing
        linear_error = self.target_linear - self.current_linear
        if abs(linear_error) > 0.001:  # Avoid tiny adjustments
            if linear_error > 0:
                # Accelerating
                max_change = self.linear_acceleration * dt
                self.current_linear += min(linear_error, max_change)
            else:
                # Decelerating
                max_change = self.linear_acceleration * self.deceleration_factor * dt
                self.current_linear += max(linear_error, -max_change)
        else:
            self.current_linear = self.target_linear

        # Angular velocity smoothing
        angular_error = self.target_angular - self.current_angular
        if abs(angular_error) > 0.001:  # Avoid tiny adjustments
            if angular_error > 0:
                # Accelerating
                max_change = self.angular_acceleration * dt
                self.current_angular += min(angular_error, max_change)
            else:
                # Decelerating
                max_change = self.angular_acceleration * self.deceleration_factor * dt
                self.current_angular += max(angular_error, -max_change)
        else:
            self.current_angular = self.target_angular

    def apply_dead_zone(self):
        """Apply dead zone to eliminate drift and micro-movements"""
        if abs(self.current_linear) < self.dead_zone_threshold:
            self.current_linear = 0.0
        if abs(self.current_angular) < self.dead_zone_threshold:
            self.current_angular = 0.0

    def emergency_stop_due_to_obstacle(self):
        """Emergency stop triggered by obstacle detection"""
        print("🚨 EMERGENCY STOP: Obstacle too close!")
        self.emergency_stop()

    def process_key(self, key):
        """Process keyboard input for incremental movement control"""
        if key == '\x1b':  # ESC key
            self.emergency_stop()
            self.running = False
            return

        elif key == '\x03':  # Ctrl+C
            self.emergency_stop()
            self.running = False
            return

        # Incremental movement keys - Add movements to queue
        elif key == 'w':
            if not self.check_obstacle_before_movement('forward'):
                self.add_movement_to_queue('linear', 'forward')
                print(f"🔼 Forward {self.linear_increment}m queued")
            else:
                print("🛑 Forward movement blocked by obstacle")

        elif key == 's':
            if not self.check_obstacle_before_movement('backward'):
                self.add_movement_to_queue('linear', 'backward')
                print(f"🔽 Backward {self.linear_increment}m queued")
            else:
                print("🛑 Backward movement blocked by obstacle")

        elif key == 'a':
            if not self.check_obstacle_before_movement('left'):
                self.add_movement_to_queue('angular', 'left')
                print(f"◀️ Left turn {math.degrees(self.angular_increment):.1f}° queued")
            else:
                print("🛑 Left turn blocked by obstacle")

        elif key == 'd':
            if not self.check_obstacle_before_movement('right'):
                self.add_movement_to_queue('angular', 'right')
                print(f"▶️ Right turn {math.degrees(self.angular_increment):.1f}° queued")
            else:
                print("🛑 Right turn blocked by obstacle")

        elif key == ' ':  # Space bar - Emergency stop
            self.safety_override = False  # Reset override
            self.emergency_stop()
            print("🛑 EMERGENCY STOP")

        # Manual stop controls
        elif key == 'x':  # Stop current movement
            self.complete_current_movement()
            print("⏹️ Current movement STOPPED")

        elif key == 'z':  # Clear movement queue
            with self.movement_lock:
                self.movement_queue.clear()
            print("🗑️ Movement queue CLEARED")

        # Safety override (for advanced users)
        elif key == 'o':  # Override safety
            self.safety_override = not self.safety_override
            status = "ENABLED" if self.safety_override else "DISABLED"
            print(f"⚠️ Safety override {status}")
            if self.safety_override:
                print("   WARNING: Obstacle detection disabled!")

        # Clear obstacle detection (if stuck)
        elif key == 'c':
            self.obstacle_detected = False
            self.obstacle_direction = None
            print("🔄 Obstacle detection cleared")

        # Movement parameter adjustment keys
        elif key == 'q':
            self.linear_increment = min(1.0, self.linear_increment + 0.1)
            print(f"⬆️ Linear increment: {self.linear_increment:.1f}m")

        elif key == 'e':
            self.linear_increment = max(0.1, self.linear_increment - 0.1)
            print(f"⬇️ Linear increment: {self.linear_increment:.1f}m")

        elif key == 'r':
            self.angular_increment = min(math.pi, self.angular_increment + 0.1)
            print(f"⬆️ Angular increment: {math.degrees(self.angular_increment):.1f}°")

        elif key == 'f':
            self.angular_increment = max(0.1, self.angular_increment - 0.1)
            print(f"⬇️ Angular increment: {math.degrees(self.angular_increment):.1f}°")

        elif key == 'h':
            self.print_instructions()

        elif key == 'i':  # Info
            self.print_status()

    def check_obstacle_before_movement(self, direction):
        """Check if movement in given direction would hit an obstacle"""
        if self.safety_override or not self.obstacle_detected:
            return False

        # Check direction-specific obstacles
        if direction == 'forward' and self.obstacle_direction == 'front':
            return True
        elif direction == 'backward' and self.obstacle_direction == 'back':
            return True
        elif direction == 'left' and self.obstacle_direction == 'left':
            return True
        elif direction == 'right' and self.obstacle_direction == 'right':
            return True

        return False

    def emergency_stop(self):
        """Emergency stop - Clear all movements and stop immediately"""
        with self.movement_lock:
            self.movement_queue.clear()
            self.current_movement = None
        self.target_linear = 0.0
        self.target_angular = 0.0
        self.current_linear = 0.0
        self.current_angular = 0.0

    def print_status(self):
        """Print current status information including incremental movement state"""
        print(f"\n📊 Current Status:")
        print(f"  Linear: {self.current_linear:.3f} m/s (target: {self.target_linear:.3f})")
        print(f"  Angular: {self.current_angular:.3f} rad/s (target: {self.target_angular:.3f})")
        print(f"  Movement Increments: {self.linear_increment:.1f}m, {math.degrees(self.angular_increment):.1f}°")
        print(f"  Movement Speeds: {self.movement_speed:.1f} m/s, {self.rotation_speed:.1f} rad/s")
        print(f"  Control Rate: {self.control_frequency} Hz")

        # Movement queue status
        with self.movement_lock:
            queue_status = f"{len(self.movement_queue)} queued" if self.movement_queue else "Empty"
            current_status = f"Executing {self.current_movement['type']} {self.current_movement['direction']}" if self.current_movement else "Idle"
        print(f"  Movement Queue: {queue_status}")
        print(f"  Current Movement: {current_status}")

        # Obstacle detection status
        obstacle_status = "🟢 Clear" if not self.obstacle_detected else f"🔴 Detected ({self.obstacle_direction})"
        print(f"  Obstacles: {obstacle_status}")
        print(f"  Safety Override: {'🟡 ENABLED' if self.safety_override else '🟢 Normal'}")

        # Sensor status
        sensor_age = time.time() - self.last_scan_time
        sensor_status = "🟢 Active" if sensor_age < 1.0 else f"🟡 Delayed ({sensor_age:.1f}s)"
        print(f"  Lidar: {sensor_status}")

    def cleanup(self):
        """Cleanup when shutting down"""
        self.emergency_stop()

        # Send final stop command
        stop_twist = Twist()
        self.cmd_vel_pub.publish(stop_twist)

        # Restore terminal settings
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        self.get_logger().info("🛑 Smooth WASD Controller Stopped")

def main(args=None):
    rclpy.init(args=args)

    try:
        controller = IncrementalWASDController()

        # Keep the node alive - ROS2 timer handles control loop
        rclpy.spin(controller)

    except KeyboardInterrupt:
        print("\n🛑 Shutting down incremental controller...")

    finally:
        if 'controller' in locals():
            controller.cleanup()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
