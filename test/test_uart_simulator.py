#!/usr/bin/env python3
"""
UART Command Simulator for ESP32 RC Car
Simulates Jetson communication via serial port for testing AUTO mode
"""

import serial
import time
import sys
import argparse

# UART Configuration
DEFAULT_BAUD = 921600
DEFAULT_PORT = '/dev/ttyUSB0'  # Linux
# DEFAULT_PORT = 'COM3'  # Windows
# DEFAULT_PORT = '/dev/tty.usbserial-*'  # macOS

# Command templates
COMMANDS = {
    'arm': 'M:SYS_ARM',
    'disarm': 'M:SYS_DISARM',
    'auto': 'M:SYS_MODE:1',  # 1 = AUTO mode
    'manual': 'M:SYS_MODE:0',  # 0 = MANUAL mode
    'speed': 'C:SET_SPEED:{value}',
    'steer': 'C:SET_STEER:{value}',
    'brake': 'E:BRAKE_NOW',
    'stop': 'E:STOP',
}

def send_command(ser, command):
    """Send a command via UART"""
    cmd_bytes = (command + '\n').encode('utf-8')
    ser.write(cmd_bytes)
    print(f"Sent: {command}")
    time.sleep(0.01)  # Small delay

def read_response(ser, timeout=0.1):
    """Read response from ESP32"""
    ser.timeout = timeout
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        return response.strip()
    return None

def interactive_mode(ser):
    """Interactive command mode"""
    print("\n=== ESP32 RC Car UART Simulator ===")
    print("Commands:")
    print("  arm              - ARM the system")
    print("  disarm           - DISARM the system")
    print("  auto             - Switch to AUTO mode")
    print("  manual           - Switch to MANUAL mode")
    print("  speed <0-255>    - Set motor speed")
    print("  steer <angle>    - Set steering angle (50-135)")
    print("  brake            - Emergency brake")
    print("  stop             - Stop command")
    print("  status           - Request status")
    print("  heartbeat        - Send heartbeat (auto mode)")
    print("  q/quit           - Exit")
    print("\nEnter commands (or 'help' for this menu):\n")
    
    heartbeat_interval = 0.1  # 100ms = 10 Hz (faster than 120ms timeout)
    last_heartbeat = 0
    
    while True:
        try:
            # Auto-send heartbeat in AUTO mode every 100ms
            if time.time() - last_heartbeat >= heartbeat_interval:
                send_command(ser, 'M:SYS_ARM')  # Use ARM as heartbeat
                last_heartbeat = time.time()
            
            # Check for user input
            if sys.stdin in __import__('select').select([sys.stdin], [], [], 0)[0]:
                cmd = input().strip().lower()
                
                if cmd in ['q', 'quit', 'exit']:
                    break
                elif cmd == 'help':
                    print("\nCommands: arm, disarm, auto, manual, speed <val>, steer <val>, brake, stop, status, heartbeat, quit\n")
                elif cmd == 'arm':
                    send_command(ser, COMMANDS['arm'])
                elif cmd == 'disarm':
                    send_command(ser, COMMANDS['disarm'])
                elif cmd == 'auto':
                    send_command(ser, COMMANDS['auto'])
                elif cmd == 'manual':
                    send_command(ser, COMMANDS['manual'])
                elif cmd.startswith('speed '):
                    try:
                        value = int(cmd.split()[1])
                        send_command(ser, COMMANDS['speed'].format(value=value))
                    except (IndexError, ValueError):
                        print("Usage: speed <0-255>")
                elif cmd.startswith('steer '):
                    try:
                        value = int(cmd.split()[1])
                        send_command(ser, COMMANDS['steer'].format(value=value))
                    except (IndexError, ValueError):
                        print("Usage: steer <50-135>")
                elif cmd == 'brake':
                    send_command(ser, COMMANDS['brake'])
                elif cmd == 'stop':
                    send_command(ser, COMMANDS['stop'])
                elif cmd == 'status':
                    send_command(ser, 'M:SYS_ARM')  # Trigger status response
                elif cmd == 'heartbeat':
                    send_command(ser, COMMANDS['arm'])
                    last_heartbeat = time.time()
                else:
                    print(f"Unknown command: {cmd}. Type 'help' for commands.")
            
            # Read responses
            response = read_response(ser)
            if response:
                print(f"Received: {response}")
                
        except KeyboardInterrupt:
            break
    
    print("\nExiting...")

def demo_sequence(ser):
    """Run a demo sequence"""
    print("Running demo sequence...")
    
    # ARM system
    send_command(ser, COMMANDS['arm'])
    time.sleep(0.5)
    
    # Switch to AUTO mode
    send_command(ser, COMMANDS['auto'])
    time.sleep(0.5)
    
    # Send heartbeats and control commands
    for i in range(10):
        # Heartbeat
        send_command(ser, COMMANDS['arm'])
        time.sleep(0.1)
        
        # Set speed
        speed = 128 + (i % 2) * 64  # Alternate between 128 and 192
        send_command(ser, COMMANDS['speed'].format(value=speed))
        time.sleep(0.1)
        
        # Set steering
        angle = 90 + (i % 3 - 1) * 20  # Alternate: 70, 90, 110
        send_command(ser, COMMANDS['steer'].format(value=angle))
        time.sleep(0.1)
    
    # Emergency brake
    send_command(ser, COMMANDS['brake'])
    time.sleep(0.5)
    
    # Disarm
    send_command(ser, COMMANDS['disarm'])
    print("Demo sequence complete!")

def main():
    parser = argparse.ArgumentParser(description='ESP32 RC Car UART Simulator')
    parser.add_argument('-p', '--port', default=DEFAULT_PORT, help='Serial port')
    parser.add_argument('-b', '--baud', type=int, default=DEFAULT_BAUD, help='Baud rate')
    parser.add_argument('-d', '--demo', action='store_true', help='Run demo sequence')
    parser.add_argument('--list-ports', action='store_true', help='List available serial ports')
    
    args = parser.parse_args()
    
    if args.list_ports:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        print("Available serial ports:")
        for port in ports:
            print(f"  {port.device} - {port.description}")
        return
    
    try:
        print(f"Connecting to {args.port} at {args.baud} baud...")
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)  # Wait for ESP32 to initialize
        
        if args.demo:
            demo_sequence(ser)
        else:
            interactive_mode(ser)
        
        ser.close()
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        print(f"\nTry listing available ports: {sys.argv[0]} --list-ports")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted!")
        if 'ser' in locals():
            ser.close()

if __name__ == '__main__':
    main()

