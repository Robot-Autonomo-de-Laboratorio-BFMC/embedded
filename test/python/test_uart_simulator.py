#!/usr/bin/env python3
import sys
import time
import argparse
import threading
from typing import List
from queue import Queue

try:
    import serial  # pyserial
    from serial.tools import list_ports
except ImportError:
    print("pyserial not installed. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)


HELP_TEXT = """
Commands (type and press Enter):
  speed <0-255>       Set motor speed (C:SET_SPEED)
  steer <0-180>       Set steering angle in degrees (C:SET_STEER)
  lights <off|on|auto>  Set lights mode (not handled over UART by firmware)
  emergency           Trigger emergency brake (E:BRAKE_NOW)
  stop                Alias for speed 0
  arm                 Arm the system (M:SYS_ARM) - REQUIRED before control commands
  disarm              Disarm the system (M:SYS_DISARM)
  mode <manual|auto>  Set system mode (M:SYS_MODE)
  demo                Send a short demo sequence
  help                Show this help
  quit / exit         Leave the simulator
""".strip()


def select_port_interactive() -> str:
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found. Connect the ESP32 and try again.")
        sys.exit(1)
    if len(ports) == 1:
        return ports[0].device
    print("Available serial ports:")
    for idx, p in enumerate(ports):
        print(f"  [{idx}] {p.device} - {p.description}")
    while True:
        sel = input("Select port index: ").strip()
        if sel.isdigit() and 0 <= int(sel) < len(ports):
            return ports[int(sel)].device
        print("Invalid selection. Try again.")


def open_serial(port: str, baud: int) -> serial.Serial:
    return serial.Serial(port, baudrate=baud, timeout=0.2)


def write_line(ser: serial.Serial, msg: str) -> None:
    line = (msg.strip() + "\n").encode("utf-8")
    ser.write(line)
    ser.flush()


def read_available(ser: serial.Serial) -> List[str]:
    lines: List[str] = []
    try:
        while True:
            data = ser.readline()
            if not data:
                break
            lines.append(data.decode(errors="ignore").rstrip())
    except Exception:
        pass
    return lines


def serial_reader_thread(ser: serial.Serial, output_queue: Queue) -> None:
    """Background thread that continuously reads from serial and puts lines in queue."""
    while True:
        try:
            data = ser.readline()
            if data:
                line = data.decode(errors="ignore").rstrip()
                if line:
                    output_queue.put(("serial", line))
        except Exception:
            break


def to_mode(value: str) -> int:
    v = value.lower()
    if v in ("off", "0"):
        return 0
    if v in ("on", "1"):
        return 1
    if v in ("auto", "2"):
        return 2
    raise ValueError("lights mode must be off|on|auto")


def run_demo(ser: serial.Serial, delay_s: float = 0.05) -> None:
    """
    Demo: Simulate making a corner
    - Arm the system first
    - Start at max speed (255) going forward
    - Turn degree by degree to the right while moving forward
    - Stop at the end
    """
    print("Starting corner demo...")
    
    # Arm the system first
    print("-> M:SYS_ARM:0")
    write_line(ser, "M:SYS_ARM:0")
    time.sleep(delay_s * 2)
    
    # Set mode to AUTO (so it transitions to RUNNING with heartbeat)
    print("-> M:SYS_MODE:1")
    write_line(ser, "M:SYS_MODE:1")
    time.sleep(delay_s * 2)
    
    # Start with max speed forward
    print("-> C:SET_SPEED:255")
    write_line(ser, "C:SET_SPEED:255")
    time.sleep(delay_s)
    
    # Start centered (105 is center, 50 is left, 135 is right)
    SERVO_CENTER = 105
    SERVO_RIGHT = 135
    
    # Turn right degree by degree (from center to right)
    print("Turning right while moving forward...")
    for angle in range(SERVO_CENTER, SERVO_RIGHT + 1):
        cmd = f"C:SET_STEER:{angle}"
        print(f"-> {cmd}")
        write_line(ser, cmd)
        time.sleep(delay_s)
        
        # Read any responses
        for ln in read_available(ser):
            if ln:
                print(f"<- {ln}")
    
    # Continue forward a bit more
    time.sleep(delay_s * 5)
    
    # Stop
    print("-> E:BRAKE_NOW:0")
    write_line(ser, "E:BRAKE_NOW:0")
    time.sleep(delay_s)
    
    print("Demo completed!")


def interactive_loop(ser: serial.Serial) -> None:
    print("Interactive UART simulator. Type 'help' for commands. Ctrl+C to quit.")
    print("(All serial output from ESP32 will be shown in real-time)")
    print(HELP_TEXT)
    
    # Start background thread to read serial continuously
    output_queue: Queue = Queue()
    reader_thread = threading.Thread(target=serial_reader_thread, args=(ser, output_queue), daemon=True)
    reader_thread.start()
    
    while True:
        # Check for serial output
        try:
            while True:
                source, line = output_queue.get_nowait()
                if source == "serial":
                    print(f"<- {line}")
        except:
            pass
        
        try:
            # Use timeout for input to allow checking queue periodically
            # Note: This requires a more complex approach on Windows
            # For now, we'll check queue before each input
            raw = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not raw:
            # show any pending output
            try:
                while True:
                    source, line = output_queue.get_nowait()
                    if source == "serial":
                        print(f"<- {line}")
            except:
                pass
            continue
        parts = raw.split()
        cmd = parts[0].lower()
        try:
            if cmd in ("quit", "exit"):
                break
            elif cmd == "help":
                print(HELP_TEXT)
            elif cmd == "speed":
                if len(parts) != 2:
                    print("Usage: speed <0-255>")
                    continue
                val = int(parts[1])
                if not (0 <= val <= 255):
                    print("Value must be 0..255")
                    continue
                msg = f"C:SET_SPEED:{val}"
                print(f"-> {msg}")
                write_line(ser, msg)
            elif cmd == "steer":
                if len(parts) != 2:
                    print("Usage: steer <0-180>")
                    continue
                val = int(parts[1])
                if not (0 <= val <= 180):
                    print("Value must be 0..180")
                    continue
                msg = f"C:SET_STEER:{val}"
                print(f"-> {msg}")
                write_line(ser, msg)
            elif cmd == "lights":
                if len(parts) != 2:
                    print("Usage: lights <off|on|auto>")
                    continue
                mode = to_mode(parts[1])
                print("(note) Lights over UART not handled by firmware; command skipped")
            elif cmd == "emergency":
                msg = "E:BRAKE_NOW:0"
                print(f"-> {msg}")
                write_line(ser, msg)
            elif cmd == "stop":
                msg = "C:SET_SPEED:0"
                print(f"-> {msg}")
                write_line(ser, msg)
            elif cmd == "arm":
                msg = "M:SYS_ARM:0"
                print(f"-> {msg}")
                write_line(ser, msg)
                print("(note) System must be ARMED before control commands work")
            elif cmd == "disarm":
                msg = "M:SYS_DISARM:0"
                print(f"-> {msg}")
                write_line(ser, msg)
            elif cmd == "mode":
                if len(parts) != 2:
                    print("Usage: mode <manual|auto>")
                    continue
                mode_val = parts[1].lower()
                if mode_val == "auto":
                    msg = "M:SYS_MODE:1"
                elif mode_val == "manual":
                    msg = "M:SYS_MODE:0"
                else:
                    print("Mode must be 'manual' or 'auto'")
                    continue
                print(f"-> {msg}")
                write_line(ser, msg)
            elif cmd == "demo":
                run_demo(ser)
            else:
                # Allow raw full protocol lines
                if ":" in raw:
                    print(f"-> {raw}")
                    write_line(ser, raw)
                else:
                    print("Unknown command. Type 'help'.")
            # Check for any serial output after sending command
            time.sleep(0.05)
            try:
                while True:
                    source, line = output_queue.get_nowait()
                    if source == "serial":
                        print(f"<- {line}")
            except:
                pass
        except ValueError as ve:
            print(f"Error: {ve}")
        except Exception as ex:
            print(f"Serial error: {ex}")


def send_batch(port: str, baud: int, commands: List[str], delay_s: float) -> None:
    with open_serial(port, baud) as ser:
        for cmd in commands:
            print(f"-> {cmd}")
            write_line(ser, cmd)
            time.sleep(delay_s)
            for ln in read_available(ser):
                if ln:
                    print(f"<- {ln}")


def main() -> None:
    parser = argparse.ArgumentParser(description="ESP32 UART command simulator (interactive by default)")
    parser.add_argument("port", nargs="?", help="Serial port (e.g., /dev/ttyUSB0, COM5). If omitted, you'll be prompted.")
    parser.add_argument("command", nargs="*", help="Optional raw command(s) to send and exit")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200 for USB; use 921600 for Serial1)")
    parser.add_argument("--sleep", type=float, default=0.05, help="Delay between commands in seconds")
    parser.add_argument("--non-interactive", action="store_true", help="Do not run REPL; just send provided commands")
    args = parser.parse_args()

    port = args.port or select_port_interactive()

    if args.non_interactive and args.command:
        send_batch(port, args.baud, args.command, args.sleep)
        return

    try:
        with open_serial(port, args.baud) as ser:
            interactive_loop(ser)
    except serial.SerialException as se:
        print(f"Failed to open {port}: {se}")
        sys.exit(1)


if __name__ == "__main__":
    main()


