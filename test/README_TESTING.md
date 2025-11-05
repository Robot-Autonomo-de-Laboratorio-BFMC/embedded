# Quick Testing Guide

## Testing Without Jetson

### Option 1: Python Simulator (Easiest)

```bash
# Install dependencies
pip install pyserial

# Run interactive simulator
python3 test/test_uart_simulator.py -p /dev/ttyUSB0

# Or run demo sequence
python3 test/test_uart_simulator.py -p /dev/ttyUSB0 --demo
```

### Option 2: Serial Terminal

1. Connect serial adapter to GPIO 9 (TX) and GPIO 10 (RX)
2. Open serial terminal at **921600 baud**
3. Send commands from `test/test_commands.txt`

### Option 3: Web Interface (MANUAL Mode)

1. Connect to Wi-Fi: `RC-Car-ESP32`
2. Open: `http://192.168.4.1`
3. Click ARM and use controls

## Quick Test Commands

```
M:SYS_ARM           # ARM system
M:SYS_MODE:1        # Switch to AUTO
C:SET_SPEED:128     # Set speed
C:SET_STEER:90      # Set steering
E:BRAKE_NOW         # Emergency brake
M:SYS_DISARM        # Disarm
```

See `docs/TESTING_WITHOUT_JETSON.md` for complete guide.

