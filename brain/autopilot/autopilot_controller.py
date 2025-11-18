"""
AutoPilot Controller Module - Orchestrates lane detection, angle conversion, and command sending.

Following SRP: This module orchestrates the interaction between detector, converter, and sender.
"""

import threading
import time
import sys
import os

# Import lane_detector module from same package
from .lane_detector import MarcosLaneDetector_Advanced

# Import autopilot modules (relative imports within package)
from .angle_converter import AngleConverter
from .command_sender import CommandSender
from .video_streamer import VideoStreamer


class AutoPilotController:
    """Orchestrates lane detection, angle conversion, and command sending."""
    
    def __init__(self, video_streamer: VideoStreamer, command_sender: CommandSender,
                 pid_kp: float = 0.06, pid_ki: float = 0.002, pid_kd: float = 0.02,
                 pid_tolerance: int = 40, threshold: int = 180):
        """
        Initialize the auto-pilot controller.
        
        Args:
            video_streamer: VideoStreamer instance for getting frames
            command_sender: CommandSender instance for sending commands
            pid_kp: PID proportional gain
            pid_ki: PID integral gain
            pid_kd: PID derivative gain
            pid_tolerance: PID tolerance for "straight" detection
            threshold: Image processing threshold
        """
        self.video_streamer = video_streamer
        self.command_sender = command_sender
        self.angle_converter = AngleConverter()
        
        # Initialize lane detector
        self.detector = MarcosLaneDetector_Advanced(
            pid_kp=pid_kp,
            pid_ki=pid_ki,
            pid_kd=pid_kd,
            pid_tolerance=pid_tolerance,
            threshold=threshold
        )
        
        self.is_running = False
        self.thread = None
        self.lock = threading.Lock()
        
        # Store PID parameters for later updates
        self.pid_kp = pid_kp
        self.pid_ki = pid_ki
        self.pid_kd = pid_kd
        self.pid_tolerance = pid_tolerance
        
        # Statistics
        self.last_pid_angle = None
        self.last_servo_angle = None
        self.command_count = 0
        self.error_count = 0
        
        # Debug images storage
        self.last_debug_images = None
    
    def start(self):
        """Start the auto-pilot controller."""
        with self.lock:
            if self.is_running:
                return False
            
            self.is_running = True
            self.thread = threading.Thread(target=self._control_loop, daemon=True)
            self.thread.start()
            print("[AutoPilotController] Started")
            return True
    
    def stop(self):
        """Stop the auto-pilot controller."""
        with self.lock:
            if not self.is_running:
                return False
            
            self.is_running = False
            print("[AutoPilotController] Stopped")
            return True
    
    def _control_loop(self):
        """Main control loop running in background thread."""
        while self.is_running:
            try:
                # Get frame from video streamer
                frame = self.video_streamer.get_frame()
                
                if frame is None:
                    time.sleep(0.033)  # Wait ~30ms if no frame
                    continue
                
                # Detect lane and get steering angle from curvature
                steering_angle, debug_images = self.detector.get_steering_angle(frame)
                
                # Store debug images for streaming
                with self.lock:
                    if debug_images is not None:
                        self.last_debug_images = debug_images
                    # Keep previous debug images if new ones are None (don't overwrite with None)
                
                # Convert steering angle to servo angle
                servo_angle = self.angle_converter.convert(steering_angle)
                
                # Send command to ESP32
                success = self.command_sender.send_steering_command(servo_angle)
                
                # Update statistics
                with self.lock:
                    self.last_pid_angle = steering_angle  # Keep name for compatibility, but it's now curvature angle
                    self.last_servo_angle = servo_angle
                    if success:
                        self.command_count += 1
                    else:
                        self.error_count += 1
                
                # Control loop rate (~30 FPS)
                time.sleep(0.033)
                
            except Exception as e:
                print(f"[AutoPilotController] Error in control loop: {e}")
                self.error_count += 1
                time.sleep(0.1)
    
    def get_status(self) -> dict:
        """Get current status of the auto-pilot controller."""
        with self.lock:
            return {
                'is_running': self.is_running,
                'last_pid_angle': self.last_pid_angle,
                'last_servo_angle': self.last_servo_angle,
                'command_count': self.command_count,
                'error_count': self.error_count
            }
    
    def update_pid_parameters(self, kp: float = None, ki: float = None, kd: float = None):
        """
        Update PID parameters dynamically.
        
        Args:
            kp: New proportional gain (None to keep current)
            ki: New integral gain (None to keep current)
            kd: New derivative gain (None to keep current)
        """
        with self.lock:
            if kp is not None:
                self.pid_kp = kp
                self.detector.pid_controller.Kp = kp
            if ki is not None:
                self.pid_ki = ki
                self.detector.pid_controller.Ki = ki
            if kd is not None:
                self.pid_kd = kd
                self.detector.pid_controller.Kd = kd
            
            # Reset PID state when parameters change (using the new reset method)
            self.detector.pid_controller.reset()
    
    def get_pid_parameters(self) -> dict:
        """Get current PID parameters."""
        with self.lock:
            return {
                'kp': self.pid_kp,
                'ki': self.pid_ki,
                'kd': self.pid_kd,
                'tolerance': self.pid_tolerance
            }
    
    def get_debug_image(self, image_key: str):
        """
        Get a debug image by key.
        
        Args:
            image_key: Key of the debug image ('bird_view_lines', 'sliding_windows', 'final_result', etc.)
            
        Returns:
            Image (numpy array) or None if not available
        """
        with self.lock:
            if self.last_debug_images and image_key in self.last_debug_images:
                return self.last_debug_images[image_key].copy()
        return None

