#!/usr/bin/env python3
"""
Simple UART Test Script
Sends a sequence of commands to test ESP32 RC Car
"""

import serial
import time
import sys

# Configuration
PORT = '/dev/ttyUSB0'  # Change to your port
BAUD = 921600

def send_command(ser, cmd):
    """Send command and wait for response"""
    ser.write((cmd + '\n').encode('utf-8'))
    print(f"Sent: {cmd}")
    time.sleep(0.1)
    
    # Read response
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        if response.strip():
            print(f"Received: {response.strip()}")

def main():
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = PORT
    
    try:
        print(f"Connecting to {port} at {BAUD} baud...")
        ser = serial.Serial(port, BAUD, timeout=1)
        time.sleep(2)  # Wait for ESP32 to initialize
        
        print("\n=== Sending Test Commands ===\n")
        
        # Test sequence
        send_command(ser, 'M:SYS_ARM')
        send_command(ser, 'M:SYS_MODE:1')  # AUTO mode
        send_command(ser, 'C:SET_SPEED:128')
        send_command(ser, 'C:SET_STEER:90')
        time.sleep(0.5)
        
        send_command(ser, 'C:SET_SPEED:200')
        send_command(ser, 'C:SET_STEER:70')
        time.sleep(0.5)
        
        send_command(ser, 'E:BRAKE_NOW')
        time.sleep(0.5)
        
        send_command(ser, 'M:SYS_DISARM')
        
        print("\n=== Test Complete ===")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        print(f"\nTry: python3 {sys.argv[0]} /dev/ttyUSB0")
        print("Or list ports: python3 test_uart_simulator.py --list-ports")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted!")
        if 'ser' in locals():
            ser.close()

if __name__ == '__main__':
    main()

