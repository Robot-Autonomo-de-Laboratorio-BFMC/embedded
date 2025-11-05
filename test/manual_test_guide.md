# Manual Testing Guide

This guide helps you manually test the RC car system functionality when automated tests aren't sufficient or for hardware validation.

## Prerequisites

- ESP32 board connected via USB
- Serial monitor open (115200 baud)
- Wi-Fi capable device (phone/laptop) for web interface testing
- Optional: UART device (Jetson or serial terminal) for AUTO mode testing

## Test Procedures

### 1. System Initialization Test

**Objective**: Verify system boots correctly and all tasks start

**Steps**:
1. Upload firmware to ESP32
2. Open serial monitor
3. Verify you see:
   ```
   ========================================
   ESP32 RC Car FreeRTOS System Starting...
   ========================================
   [main] Mailboxes initialized
   [main] LinkRxTask created on Core 1, Priority 4
   [main] MotorTask created on Core 0, Priority 4
   [main] SteerTask created on Core 0, Priority 3
   [main] LightsTask created on Core 1, Priority 1
   [main] SupervisorTask created on Core 1, Priority 2
   [main] LinkTxTask created on Core 1, Priority 2
   [main] WebTask created on Core 1, Priority 2
   [main] All tasks created. FreeRTOS scheduler running...
   [main] System ready!
   ```

**Expected Result**: All tasks start without errors

---

### 2. Wi-Fi AP Test

**Objective**: Verify Wi-Fi Access Point is created

**Steps**:
1. Check available Wi-Fi networks on your device
2. Look for network named "RC-Car-ESP32"
3. Connect to it (no password)

**Expected Result**: 
- Wi-Fi network visible
- Can connect successfully
- Serial monitor shows: `[WebTask] Wi-Fi AP started. SSID: RC-Car-ESP32, IP: 192.168.4.1`

---

### 3. Web Interface Test

**Objective**: Verify web server responds and HTML loads

**Steps**:
1. Connect to Wi-Fi AP "RC-Car-ESP32"
2. Open browser and navigate to `http://192.168.4.1`
3. Verify you see the control interface with:
   - Mode selector buttons (MANUAL/AUTO)
   - ARM/DISARM buttons
   - BRAKE button
   - Status overlay showing mode and state
   - Control buttons (forward, back, left, right)
   - Lights controls
   - Speed slider

**Expected Result**: Web page loads completely, all buttons visible

---

### 4. Manual Mode - ARM/DISARM Test

**Objective**: Verify system state machine transitions

**Steps**:
1. Open web interface
2. Check status overlay shows "DISARMED"
3. Click "ARM" button
4. Verify status changes to "ARMED"
5. Click "DISARM" button
6. Verify status changes back to "DISARMED"

**Expected Result**: 
- State transitions work correctly
- Serial monitor shows: `[SupervisorTask] System ARMED` / `[SupervisorTask] System DISARMED`

---

### 5. Manual Mode - Motor Control Test

**Objective**: Verify motor control in MANUAL mode

**Steps**:
1. Set mode to MANUAL (if not already)
2. ARM the system
3. Press and hold "Forward" button
4. Verify motor moves forward
5. Release button (should trigger driveStop)
6. Press and hold "Back" button
7. Verify motor moves backward
8. Release button

**Expected Result**:
- Motor responds to forward/back commands
- Motor stops when button released
- Reverse lights turn on when going backward

---

### 6. Manual Mode - Steering Control Test

**Objective**: Verify steering control in MANUAL mode

**Steps**:
1. System in MANUAL mode and ARMED
2. Press and hold "Left" button
3. Verify steering turns left
4. Release button (should center)
5. Press and hold "Right" button
6. Verify steering turns right
7. Release button (should center)

**Expected Result**:
- Steering responds to left/right commands
- Steering centers when button released

---

### 7. Manual Mode - Speed Control Test

**Objective**: Verify speed slider works

**Steps**:
1. System in MANUAL mode and ARMED
2. Move speed slider to minimum (60)
3. Press forward button
4. Note motor speed
5. Move speed slider to maximum (255)
6. Press forward button again
7. Note motor speed (should be faster)

**Expected Result**: Motor speed changes based on slider position

---

### 8. Lights Control Test

**Objective**: Verify lights work in all modes

**Steps**:
1. Click "Lights On" button
2. Verify headlights turn on
3. Click "Lights Off" button
4. Verify headlights turn off
5. Click "Lights Auto" button
6. Cover LDR sensor (GPIO 35)
7. Verify headlights turn on
8. Uncover LDR sensor
9. Verify headlights turn off

**Expected Result**: Lights respond to all control modes

---

### 9. Emergency Brake Test

**Objective**: Verify emergency brake functionality

**Steps**:
1. System in MANUAL mode and ARMED
2. Motor running forward
3. Click "BRAKE" button
4. Verify motor stops immediately (< 1ms response)

**Expected Result**: Motor stops instantly, no delay

---

### 10. E-STOP GPIO Test

**Objective**: Verify hardware E-STOP functionality

**Steps**:
1. System in RUNNING state
2. Motor running
3. Connect GPIO 4 to GND (simulating E-STOP press)
4. Verify motor stops immediately
5. Verify state changes to FAULT
6. Disconnect GPIO 4 from GND
7. Verify E-STOP releases

**Expected Result**: 
- Motor stops immediately on E-STOP
- State changes to FAULT
- Serial shows: `[SupervisorTask] E-STOP triggered via GPIO!`

---

### 11. Mode Switching Test

**Objective**: Verify MANUAL ↔ AUTO mode switching

**Steps**:
1. System in MANUAL mode
2. Click "AUTO" button
3. Verify status overlay shows "AUTO"
4. Click "MANUAL" button
5. Verify status overlay shows "MANUAL"

**Expected Result**: Mode switches correctly, status updates

---

### 12. UART Communication Test (AUTO Mode)

**Objective**: Verify UART communication with Jetson

**Prerequisites**: UART device connected to GPIO 9 (TX) and GPIO 10 (RX)

**Steps**:
1. Set mode to AUTO
2. ARM the system
3. Send UART message: `M:SYS_ARM\n`
4. Verify system ARMs
5. Send: `C:SET_SPEED:128\n`
6. Verify motor speed changes
7. Send: `C:SET_STEER:90\n`
8. Verify steering moves
9. Send: `E:BRAKE_NOW\n`
10. Verify emergency brake triggers

**Expected Result**: All UART commands are parsed and executed correctly

---

### 13. Watchdog Timeout Test

**Objective**: Verify watchdog timeout in AUTO mode

**Steps**:
1. Set mode to AUTO
2. ARM the system
3. Start sending heartbeat messages via UART (< 120ms interval)
4. Stop sending messages
5. Wait > 120ms
6. Verify system enters FAULT state
7. Verify motor stops and steering centers

**Expected Result**: 
- System detects heartbeat timeout
- Enters FAULT state
- Serial shows: `[SupervisorTask] Watchdog timeout! Heartbeat age: XXX ms`

---

### 14. Status API Test

**Objective**: Verify status endpoint returns correct JSON

**Steps**:
1. Open browser
2. Navigate to `http://192.168.4.1/status`
3. Verify JSON response: `{"mode":"MANUAL","state":"DISARMED"}`
4. Change mode and state
5. Refresh status endpoint
6. Verify JSON updates

**Expected Result**: Status endpoint returns correct JSON with current mode and state

---

## Test Checklist

- [ ] System initialization
- [ ] Wi-Fi AP creation
- [ ] Web interface loads
- [ ] ARM/DISARM functionality
- [ ] Motor control (forward/back)
- [ ] Steering control (left/right)
- [ ] Speed control
- [ ] Lights control (ON/OFF/AUTO)
- [ ] Emergency brake
- [ ] E-STOP GPIO
- [ ] Mode switching
- [ ] UART communication (if hardware available)
- [ ] Watchdog timeout
- [ ] Status API

## Troubleshooting

### Wi-Fi AP not visible
- Check serial monitor for Wi-Fi initialization errors
- Verify board has Wi-Fi capability
- Try power cycling the board

### Web interface not loading
- Verify you're connected to "RC-Car-ESP32" network
- Check IP address in serial monitor
- Try accessing `http://192.168.4.1` directly

### Motor not responding
- Verify system is ARMED
- Check mode is MANUAL
- Verify GPIO connections (IN3, IN4, ENB)
- Check serial monitor for error messages

### UART not working
- Verify baud rate is 921600
- Check GPIO 9 (TX) and GPIO 10 (RX) connections
- Verify UART device is configured correctly
- Check serial monitor for parsing errors

