# Testing ESP32 RC Car Without Jetson

This guide explains how to test the AUTO mode and UART communication without a Jetson board.

## Methods

### Method 1: Python UART Simulator (Recommended)

Use the provided Python script to simulate Jetson communication.

#### Prerequisites

```bash
pip install pyserial
```

#### Usage

1. **List available serial ports:**
   ```bash
   python3 test/test_uart_simulator.py --list-ports
   ```

2. **Interactive mode:**
   ```bash
   python3 test/test_uart_simulator.py -p /dev/ttyUSB0
   # On Windows: python3 test/test_uart_simulator.py -p COM3
   # On macOS: python3 test/test_uart_simulator.py -p /dev/tty.usbserial-*
   ```

3. **Run demo sequence:**
   ```bash
   python3 test/test_uart_simulator.py -p /dev/ttyUSB0 --demo
   ```

#### Interactive Commands

- `arm` - ARM the system
- `disarm` - DISARM the system
- `auto` - Switch to AUTO mode
- `manual` - Switch to MANUAL mode
- `speed <0-255>` - Set motor speed (e.g., `speed 128`)
- `steer <angle>` - Set steering angle 50-135 (e.g., `steer 90`)
- `brake` - Emergency brake
- `stop` - Stop command
- `heartbeat` - Send heartbeat manually
- `quit` - Exit

### Method 2: Serial Terminal (Manual Testing)

Use any serial terminal program to send commands manually.

#### Setup

1. **Connect ESP32:**
   - UART TX (GPIO 9) → Connect to your serial adapter RX
   - UART RX (GPIO 10) → Connect to your serial adapter TX
   - GND → Common ground

2. **Configure serial terminal:**
   - Baud rate: **921600**
   - Data bits: 8
   - Stop bits: 1
   - Parity: None
   - Flow control: None

#### Recommended Serial Terminals

- **Linux/Mac:** `screen`, `minicom`, `picocom`
  ```bash
  screen /dev/ttyUSB0 921600
  # Or
  picocom -b 921600 /dev/ttyUSB0
  ```

- **Windows:** PuTTY, Tera Term, or Arduino Serial Monitor (set baud to 921600)

- **Cross-platform:** 
  - [Serial Monitor](https://github.com/Serial-Studio/Serial-Studio)
  - [CoolTerm](https://freeware.the-meiers.org/)

#### Sending Commands

Type commands and press Enter. Each command should end with a newline:

```
M:SYS_ARM
C:SET_SPEED:128
C:SET_STEER:90
E:BRAKE_NOW
```

See `test/test_commands.txt` for a complete list of test commands.

### Method 3: Arduino/ESP32 Serial Bridge

Use a second Arduino or ESP32 to send commands programmatically.

#### Arduino Example

```cpp
void setup() {
  Serial.begin(115200);  // USB Serial
  Serial1.begin(921600, SERIAL_8N1, 10, 9);  // UART to ESP32
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    Serial1.println(cmd);  // Forward to ESP32
  }
  
  if (Serial1.available()) {
    String response = Serial1.readStringUntil('\n');
    Serial.println("ESP32: " + response);  // Echo back
  }
}
```

### Method 4: Web Interface (MANUAL Mode)

Test MANUAL mode using the web interface (no UART needed):

1. Connect to Wi-Fi AP: `RC-Car-ESP32`
2. Open browser: `http://192.168.4.1`
3. Click "ARM" button
4. Use control buttons (forward, back, left, right)
5. Check status overlay for mode/state

## Test Scenarios

### Scenario 1: Basic UART Communication

**Goal:** Verify UART parsing and command routing

**Steps:**
1. Connect serial terminal to ESP32 UART (GPIO 9/10)
2. Open serial monitor
3. Send: `M:SYS_ARM`
4. Verify serial monitor shows: `[LinkRxTask] SYS_ARM command`
5. Send: `C:SET_SPEED:128`
6. Verify motor responds (if ARMED)

**Expected:** Commands are parsed and routed correctly

### Scenario 2: AUTO Mode Control

**Goal:** Test AUTO mode with UART commands

**Steps:**
1. Use Python simulator or serial terminal
2. Send: `M:SYS_ARM`
3. Send: `M:SYS_MODE:1` (AUTO mode)
4. Send: `C:SET_SPEED:128`
5. Send: `C:SET_STEER:90`
6. Verify motor and steering respond

**Expected:** System responds to UART commands in AUTO mode

### Scenario 3: Watchdog Timeout

**Goal:** Test heartbeat timeout detection

**Steps:**
1. Switch to AUTO mode: `M:SYS_MODE:1`
2. ARM system: `M:SYS_ARM`
3. Send commands for a few seconds (heartbeat)
4. **Stop sending commands for >120ms**
5. Wait and observe

**Expected:** 
- Serial monitor shows: `[SupervisorTask] Watchdog timeout!`
- System enters FAULT state
- Motor stops automatically

### Scenario 4: Emergency Brake

**Goal:** Test emergency brake response time

**Steps:**
1. ARM system and set speed: `M:SYS_ARM`, `C:SET_SPEED:200`
2. Send emergency: `E:BRAKE_NOW`
3. Measure response time (should be <1ms)

**Expected:** Motor stops immediately

### Scenario 5: Mode Switching

**Goal:** Test MANUAL ↔ AUTO switching

**Steps:**
1. Start in MANUAL mode (default)
2. Switch to AUTO: `M:SYS_MODE:1`
3. Send UART commands (should work)
4. Switch back: `M:SYS_MODE:0`
5. Use web interface (should work)

**Expected:** Both modes work independently

### Scenario 6: Heartbeat Timing

**Goal:** Verify heartbeat keeps system alive

**Steps:**
1. AUTO mode + ARMED
2. Send command every 50ms (faster than 120ms timeout)
3. System should stay in RUNNING state
4. Stop sending for 150ms
5. System should enter FAULT

**Expected:** Heartbeat timing works correctly

## Troubleshooting

### No Response from ESP32

- **Check connections:** Verify TX/RX are correct (TX→RX, RX→TX)
- **Check baud rate:** Must be exactly 921600
- **Check serial port:** Use `--list-ports` to find correct port
- **Check ESP32 logs:** Open second serial monitor for debug output (115200 baud on USB)

### Commands Not Parsed

- **Check format:** Commands must end with `\n` (newline)
- **Check channel:** Use `E:`, `C:`, or `M:` prefix
- **Check serial monitor:** Look for `[LinkRxTask] Failed to parse message`

### Watchdog Always Timing Out

- **Heartbeat frequency:** Send commands at least every 100ms (< 120ms timeout)
- **Check UART:** Verify commands are actually being received
- **Check mode:** Watchdog only active in AUTO mode

### Motor Not Responding

- **Check state:** System must be ARMED and in RUNNING state
- **Check mode:** Verify mode is AUTO for UART commands
- **Check mailbox:** Look for `[MotorTask]` logs in serial monitor

## Quick Test Checklist

- [ ] Serial connection established (921600 baud)
- [ ] UART commands parsed correctly
- [ ] System ARMs via UART
- [ ] AUTO mode activated
- [ ] Speed commands work
- [ ] Steering commands work
- [ ] Emergency brake works
- [ ] Watchdog timeout works
- [ ] Mode switching works
- [ ] Status telemetry received

## Example Test Session

```bash
# Terminal 1: Serial monitor for ESP32 debug (USB, 115200 baud)
pio device monitor

# Terminal 2: Python UART simulator
python3 test/test_uart_simulator.py -p /dev/ttyUSB0

# In simulator:
> arm
> auto
> speed 128
> steer 90
> speed 200
> steer 70
> brake
> disarm
> quit
```

## Notes

- **UART pins:** GPIO 9 (TX), GPIO 10 (RX) - different from USB Serial
- **Baud rate:** Must be 921600 (not 115200)
- **Command format:** `CHANNEL:COMMAND[:VALUE]` + newline
- **Heartbeat:** Any UART message updates heartbeat (120ms timeout)
- **Status:** ESP32 sends status via UART automatically

