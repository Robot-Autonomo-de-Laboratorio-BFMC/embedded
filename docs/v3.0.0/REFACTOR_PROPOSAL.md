# Architectural Refactor Proposal

**Date:** November 30, 2025  
**From:** Embedded System Team  
**To:** Technical Lead / Brain Team  
**Subject:** Transition from Time-Triggered (v2.0) to Event-Driven (v3.0) Architecture

---

## 1. Current Architecture Overview (As of Nov 2025)

The current Vehicle Control Unit (VCU) operates on **Version 2.0.0**, which implements a **Time-Triggered Architecture (TTA)**. The system prioritizes loop stability and deterministic execution cycles over raw reaction speed for standard operations.

### Core Philosophy
- **Synchronous Execution:** Control tasks (Motor, Steering) run at a fixed frequency (100Hz / 10ms period).
- **Decoupling:** Producers (UART/WiFi) and Consumers (Motor/Steer) are completely decoupled via Mailboxes.
- **Hybrid Safety:** Standard commands use Polling; Emergency commands use Interrupt-style notifications.

### Components & Flow

1.  **Data Plane (Mailboxes):**
    - Uses a "Last-Writer-Wins" strategy protected by Mutexes.
    - Stores the latest valid command (`Speed`, `Angle`, `State`) with a timestamp.

2.  **Control Flow (Polling):**
    - **Producers (`LinkRxTask`, `WebTask`):** Write to the mailbox and immediately continue. They do *not* signal the consumer.
    - **Consumers (`MotorTask`, `SteerTask`):** Sleep for a fixed duration (`vTaskDelay(10ms)`). Upon waking, they check the mailbox for *any* change.

### Current "Fast-Path" Exception
The system acknowledges the limitations of polling for safety-critical events. Therefore, **Emergency Brakes** (`E:BRAKE_NOW`) bypass the mailbox entirely, using `xTaskNotify` to wake the Motor Task immediately (<1ms).

---

## 2. Problem Statement

While the current v2.0 architecture is stable and robust against "interrupt storms," it presents specific limitations that hinder high-performance autonomous racing:

### 2.1 Variable Latency (Jitter)
Because the consumer wakes up periodically, there is a random delay between the arrival of a command and its execution.
- **Best Case:** Command arrives just before the task wakes up (~0ms latency).
- **Worst Case:** Command arrives just after the task sleeps (~10ms latency).
- **Impact:** This variable jitter creates inconsistency in high-speed control loops where milliseconds matter.

### 2.2 CPU Inefficiency
Tasks wake up every 10ms regardless of activity.
- If no new command exists, the task wakes up, locks a mutex, checks a boolean, unlocks, and sleeps.
- This wastes CPU cycles that could be used for heavier processing tasks or allows the CPU to enter lower power states (if configured).

### 2.3 Architectural Inconsistency
Currently, the codebase maintains two separate mechanisms for activating the motor:
1.  **Standard:** Mailbox + Polling.
2.  **Emergency:** Direct Notification.
This increases code complexity and maintenance burden.

---

## 3. Proposed Architecture (v3.0.0)

We propose shifting to an **Event-Driven Real-Time Architecture**. This model adopts the **"Shared State with Lightweight Notification"** pattern.

### Core Philosophy
- **Reactive Execution:** Control tasks sleep indefinitely until an event occurs (Command received or Timeout).
- **Unified Notification:** Both Standard and Emergency commands use the notification mechanism.

### Proposed Structure

#### 3.1 The "Notify-on-Write" Protocol
The responsibility for timing shifts from the *Consumer* to the *Producer*.

**Step 1: Data Plane (Unchanged)**
The Producer (`LinkRx`) writes the complex data struct to the Mailbox (Thread-safe).

**Step 2: Control Plane (New)**
The Producer explicitly signals the specific consumer task that data is ready.

**Step 3: Consumption (New)**
The Consumer waits on a signal rather than a timer.

### Code Comparison

**Current (v2.0 - Polling):**
```cpp
// Consumer (MotorTask)
while(1) {
    vTaskDelay(10); // Always wait 10ms
    if (mailbox_read(...)) {
        apply_speed();
    }
}
```

**Proposed (v3.0 - Event Driven):**
```cpp
// Producer (LinkRxTask)
mailbox_write(...);
xTaskNotify(motor_task_handle, ...); // Wake up consumer!

// Consumer (MotorTask)
while(1) {
    // Sleep until notified OR 100ms timeout (safety fallback)
    xTaskNotifyWait(..., portMAX_DELAY); 
    if (mailbox_read(...)) {
        apply_speed();
    }
}
```

### 3.2 Implementation Requirements

To achieve this, we need to refactor the dependency injection logic in `main.cpp`:

1.  **Expose Task Handles:** The producers need to know *who* to notify.
    - *Action:* Add `TaskHandle_t` fields to `link_rx_params_t`, `web_task_params_t`, etc.
2.  **Register Handles:** Pass the created handles of `MotorTask` and `SteerTask` to the `LinkRxTask`.
3.  **Update Consumers:** Replace `vTaskDelay` with `xTaskNotifyWait`.

---

## 4. Benefits of v3.0.0

| Feature | v2.0 (Current) | v3.0 (Proposed) | Improvement |
| :--- | :--- | :--- | :--- |
| **Command Latency** | 0 - 10 ms (Variable) | < 1 ms (Deterministic) | **10x Faster Response** |
| **CPU Usage** | Fixed Load (Always Polls) | Dynamic (Idle when stopped) | **Higher Efficiency** |
| **Consistency** | Hybrid (Poll + Notify) | Unified (Notify only) | **Cleaner Codebase** |
| **Safety** | Fast-Path for Emergencies | Fast-Path for **EVERYTHING** | **Inherently Safer** |

### Conclusion
Migrating to v3.0.0 prepares the embedded system for higher speeds and tighter control loops required by the Brain team, removing the artificial 10ms delay floor existing in the current implementation.

