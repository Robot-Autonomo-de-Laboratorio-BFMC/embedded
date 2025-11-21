"""
Camera Module - Handles camera configuration and video streaming.

Following SRP: Each module has a single responsibility.
"""

from .video_streamer import VideoStreamer
from .camera_config import choose_camera_by_OS

__all__ = ['VideoStreamer', 'choose_camera_by_OS']

