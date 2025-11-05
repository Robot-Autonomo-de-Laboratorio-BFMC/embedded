# Quick UART Simulation Guide

## Method 1: Python Simulator (Easiest)

### Step 1: Install Dependencies

```bash
pip install pyserial
# Or
pip3 install pyserial
```

### Step 2: Find Your Serial Port

**Linux:**
```bash
python3 test/test_uart_simulator.py --list-ports
# Or manually check:
ls /dev/ttyUSB* /dev/ttyACM*
```

**Windows:**
```bash
python3 test/test_uart_simulator.py --list-ports
# Common ports: COM3, COM4, COM5, etc.
```

**macOS:**
```bash
python3 test/test_uart_simulator.py --list-ports
# Common ports: /dev/tty.usbserial-*, /dev/cu.usbserial-*
```

### Step 3: Connect Hardware

**Important:** You need a USB-to-Serial adapter (FTDI, CP2102, CH340, etc.)

Connect:
- **Adapter TX** → **ESP32 GPIO 10** (RX pin)
- **Adapter RX** → **ESP32 GPIO 9** (TX pin)  
- **Adapter GND** → **ESP32 GND**

**Note:** ESP32 has TWO serial ports:
- **USB Serial** (for programming/debugging): Uses internal USB-to-UART chip
- **UART1** (for Jetson communication): GPIO 9 (TX), GPIO 10 (RX)

### Step 4: Run the Simulator

**Interactive Mode:**
```bash
python3 test/test_uart_simulator.py -p /dev/ttyUSB0
# Replace /dev/ttyUSB0 with your port
```

**Demo Mode (Automated):**
```bash
python3 test/test_uart_simulator.py -p /dev/ttyUSB0 --demo
```

### Step 5: Use Commands

Once in interactive mode, type commands:

```
arm              # ARM the system
auto             # Switch to AUTO mode
speed 128        # Set speed to 128
steer 90         # Set steering to 90 degrees
brake            # Emergency brake
disarm           # Disarm system
quit             # Exit
```

## Method 2: Serial Terminal (Manual)

### Using screen (Linux/Mac)

```bash
# Install if needed: sudo apt install screen
screen /dev/ttyUSB0 921600
```

Then type commands manually:
```
M:SYS_ARM
M:SYS_MODE:1
C:SET_SPEED:128
C:SET_STEER:90
E:BRAKE_NOW
```

Press `Ctrl+A` then `K` to exit screen.

### Using minicom (Linux)

```bash
# Install: sudo apt install minicom
minicom -D /dev/ttyUSB0 -b 921600
```

### Using PuTTY (Windows)

1. Open PuTTY
2. Connection type: Serial
3. Serial line: COM3 (your port)
4. Speed: 921600
5. Click Open
6. Type commands directly

### Using Arduino Serial Monitor

**Note:** Arduino Serial Monitor may not support 921600 baud. Use a dedicated serial terminal instead.

## Method 3: Simple Python Script

Create a file `test_uart_simple.py`:

```python
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 921600, timeout=1)
time.sleep(2)  # Wait for ESP32

# ARM system
ser.write(b'M:SYS_ARM\n')
time.sleep(0.1)

# Switch to AUTO
ser.write(b'M:SYS_MODE:1\n')
time.sleep(0.1)

# Set speed
ser.write(b'C:SET_SPEED:128\n')
time.sleep(0.1)

# Set steering
ser.write(b'C:SET_STEER:90\n')
time.sleep(0.1)

# Read response
if ser.in_waiting:
    print(ser.read(ser.in_waiting).decode())

ser.close()
```

Run: `python3 test_uart_simple.py`

## Testing Scenarios

### Scenario 1: Basic Communication Test

```bash
# Terminal 1: Monitor ESP32 debug output (USB, 115200 baud)
pio device monitor

# Terminal 2: Run UART simulator
python3 test/test_uart_simulator.py -p /dev/ttyUSB0
```

In simulator:
```
> arm
> auto
> speed 128
```

**Expected in Terminal 1:**
```
[LinkRxTask] SYS_ARM command
[SupervisorTask] System ARMED
[LinkRxTask] SYS_MODE: AUTO
[LinkRxTask] SET_SPEED: 128
[MotorTask] Speed set to 128
```

### Scenario 2: Heartbeat Test

The Python simulator automatically sends heartbeats every 100ms. To test timeout:

1. Run simulator and send commands
2. Stop simulator (Ctrl+C)
3. Wait >120ms
4. Check ESP32 serial monitor for watchdog timeout

**Expected:**
```
[SupervisorTask] Watchdog timeout! Heartbeat age: XXX ms
[SupervisorTask] System FAULT
```

### Scenario 3: Emergency Brake Test

```bash
python3 test/test_uart_simulator.py -p /dev/ttyUSB0
```

```
> arm
> auto
> speed 200
> brake
```

**Expected:** Motor stops immediately (<1ms)

## Hardware Setup Diagram

```
ESP32                    USB-to-Serial Adapter
------                   --------------------
GPIO 9  (TX) -------->  RX
GPIO 10 (RX) <--------  TX
GND     ------------  GND

USB-to-Serial Adapter → Computer USB Port
```

## Troubleshooting

### "No such file or directory" (Linux)

The port might need permissions:
```bash
sudo chmod 666 /dev/ttyUSB0
# Or add user to dialout group:
sudo usermod -a -G dialout $USER
# Then logout/login
```

### "Permission denied"

```bash
sudo chmod 666 /dev/ttyUSB0
```

### "Serial port already in use"

Another program is using the port. Close other serial terminals or IDE serial monitors.

### No response from ESP32

1. **Check connections:** TX→RX, RX→TX (crossed!)
2. **Check baud rate:** Must be exactly 921600
3. **Check ESP32 is running:** Look for serial debug output
4. **Check UART pins:** GPIO 9/10, not USB serial

### Wrong port

List all ports:
```bash
python3 test/test_uart_simulator.py --list-ports
```

Or manually:
```bash
# Linux
ls /dev/ttyUSB* /dev/ttyACM*

# Windows
# Check Device Manager → Ports (COM & LPT)

# macOS
ls /dev/tty.usbserial* /dev/cu.usbserial*
```

## Quick Reference Commands

| Command | Format | Example |
|---------|--------|---------|
| ARM | `M:SYS_ARM` | `arm` |
| DISARM | `M:SYS_DISARM` | `disarm` |
| AUTO mode | `M:SYS_MODE:1` | `auto` |
| MANUAL mode | `M:SYS_MODE:0` | `manual` |
| Set speed | `C:SET_SPEED:VALUE` | `speed 128` |
| Set steering | `C:SET_STEER:ANGLE` | `steer 90` |
| Emergency brake | `E:BRAKE_NOW` | `brake` |
| Stop | `E:STOP` | `stop` |

## Example Test Session

```bash
# Start Python simulator
$ python3 test/test_uart_simulator.py -p /dev/ttyUSB0

=== ESP32 RC Car UART Simulator ===
Enter commands:
> arm
Sent: M:SYS_ARM

> auto
Sent: M:SYS_MODE:1

> speed 128
Sent: C:SET_SPEED:128

> steer 90
Sent: C:SET_STEER:90

> speed 200
Sent: C:SET_SPEED:200

> brake
Sent: E:BRAKE_NOW

> disarm
Sent: M:SYS_DISARM

> quit
Exiting...
```

## Tips

1. **Use two terminals:** One for ESP32 debug (USB serial), one for UART simulator
2. **Start slow:** Test one command at a time
3. **Watch serial monitor:** See how ESP32 parses commands
4. **Heartbeat:** Simulator sends automatically, no need to send manually
5. **Test mode switching:** Try MANUAL ↔ AUTO switching

