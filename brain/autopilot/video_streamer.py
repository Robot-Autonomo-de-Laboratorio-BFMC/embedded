"""
Video Streamer Module - Manages camera capture and MJPEG streaming.

Following SRP: This module only handles video capture and streaming.
"""

import cv2
import threading
import sys
import os

# Import camera_config from parent directory
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, parent_dir)
from camera_config import choose_camera_by_OS


class VideoStreamer:
    """Manages camera capture and provides frames for streaming."""
    
    def __init__(self):
        """Initialize the video streamer."""
        self.camera = None
        self.camera_path = None
        self.lock = threading.Lock()
        self.is_running = False
        self.current_frame = None
        self.frame_ready = threading.Event()
    
    def initialize(self) -> bool:
        """
        Initialize camera capture.
        
        Returns:
            True if camera initialized successfully, False otherwise
        """
        try:
            self.camera_path = choose_camera_by_OS()
            self.camera = cv2.VideoCapture(self.camera_path)
            
            if not self.camera.isOpened():
                print(f"[VideoStreamer] Error: Could not open camera at {self.camera_path}")
                return False
            
            # Set camera properties for better performance
            self.camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.camera.set(cv2.CAP_PROP_FPS, 30)
            
            self.is_running = True
            print(f"[VideoStreamer] Camera initialized at {self.camera_path}")
            # Start capture thread
            self.start_capture()
            return True
            
        except Exception as e:
            print(f"[VideoStreamer] Error initializing camera: {e}")
            return False
    
    def start_capture(self):
        """Start capturing frames in a background thread."""
        if not self.is_running or self.camera is None:
            return
        
        def capture_loop():
            import time
            while self.is_running:
                try:
                    # Use grab() + retrieve() for better performance and less blocking
                    grabbed = self.camera.grab()
                    if grabbed:
                        ret, frame = self.camera.retrieve()
                        if ret:
                            # Minimize lock time - copy frame outside lock
                            frame_copy = frame.copy()
                            with self.lock:
                                self.current_frame = frame_copy
                                self.frame_ready.set()
                        else:
                            print("[VideoStreamer] Warning: Failed to retrieve frame")
                    else:
                        # If grab fails, try read() as fallback
                        ret, frame = self.camera.read()
                        if ret:
                            frame_copy = frame.copy()
                            with self.lock:
                                self.current_frame = frame_copy
                                self.frame_ready.set()
                        else:
                            print("[VideoStreamer] Warning: Failed to read frame")
                            time.sleep(0.01)  # Small delay if camera is not ready
                except Exception as e:
                    print(f"[VideoStreamer] Error capturing frame: {e}")
                    time.sleep(0.01)  # Small delay on error to avoid tight loop
        
        thread = threading.Thread(target=capture_loop, daemon=True)
        thread.start()
    
    def get_frame(self, timeout=0.05):
        """
        Get the current frame with optional timeout to avoid blocking.
        
        Args:
            timeout: Maximum time to wait for lock (seconds)
        
        Returns:
            Current frame (numpy array) or None if no frame available or timeout
        """
        import time
        start_time = time.time()
        
        # Try to acquire lock quickly, don't block forever
        frame_ref = None
        if self.lock.acquire(timeout=timeout):
            try:
                if self.current_frame is not None:
                    frame_ref = self.current_frame
            finally:
                self.lock.release()
        
        # Copy outside lock to avoid blocking other requests
        if frame_ref is not None:
            try:
                return frame_ref.copy()
            except Exception as e:
                print(f"[VideoStreamer] Error copying frame: {e}")
                return None
        return None
    
    def generate_mjpeg(self):
        """
        Generate MJPEG stream frames.
        Optimized to avoid blocking other requests.
        
        Yields:
            JPEG-encoded frames as bytes
        """
        import time
        
        last_frame_time = 0
        frame_interval = 0.033  # ~30 FPS (33ms between frames)
        max_wait = 0.1  # Maximum wait time if no frame available
        
        while self.is_running:
            current_time = time.time()
            elapsed = current_time - last_frame_time
            
            # Throttle frame requests to avoid overwhelming the lock
            if elapsed >= frame_interval:
                # Get frame with short timeout to avoid blocking
                frame = self.get_frame(timeout=0.01)
                if frame is not None:
                    try:
                        # Encode frame as JPEG
                        ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
                        if ret:
                            last_frame_time = current_time
                            yield (b'--frame\r\n'
                                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
                        else:
                            time.sleep(0.01)  # Small delay if encoding fails
                    except Exception as e:
                        print(f"[VideoStreamer] Error encoding frame: {e}")
                        time.sleep(0.01)
                else:
                    # No frame available, wait a bit but not too long
                    time.sleep(min(frame_interval, max_wait))
            else:
                # Wait until next frame time
                sleep_time = frame_interval - elapsed
                if sleep_time > 0:
                    time.sleep(sleep_time)
    
    def stop(self):
        """Stop camera capture and release resources."""
        self.is_running = False
        if self.camera is not None:
            self.camera.release()
            self.camera = None
        print("[VideoStreamer] Camera stopped")

