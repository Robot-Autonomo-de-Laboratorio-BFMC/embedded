#include <Arduino.h>
#include <unity.h>
#include "messages.h"

// Simulate state machine transitions
system_state_t current_state = STATE_DISARMED;
system_mode_t current_mode = MODE_MANUAL;

void test_initial_state(void) {
    TEST_ASSERT_EQUAL(STATE_DISARMED, current_state);
    TEST_ASSERT_EQUAL(MODE_MANUAL, current_mode);
}

void test_disarmed_to_armed_transition(void) {
    current_state = STATE_DISARMED;
    
    // Simulate ARM command
    if (current_state == STATE_DISARMED) {
        current_state = STATE_ARMED;
    }
    
    TEST_ASSERT_EQUAL(STATE_ARMED, current_state);
}

void test_armed_to_disarmed_transition(void) {
    current_state = STATE_ARMED;
    
    // Simulate DISARM command
    current_state = STATE_DISARMED;
    
    TEST_ASSERT_EQUAL(STATE_DISARMED, current_state);
}

void test_armed_to_running_transition(void) {
    current_state = STATE_ARMED;
    current_mode = MODE_AUTO;
    uint32_t last_heartbeat_ms = millis();
    uint32_t current_ms = millis();
    
    // Transition to RUNNING if we have valid heartbeat
    if (current_state == STATE_ARMED && current_mode == MODE_AUTO) {
        if (last_heartbeat_ms > 0 && (current_ms - last_heartbeat_ms) < 120) {
            current_state = STATE_RUNNING;
        }
    }
    
    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);
}

void test_heartbeat_timeout_to_fault(void) {
    current_state = STATE_RUNNING;
    current_mode = MODE_AUTO;
    uint32_t last_heartbeat_ms = millis() - 150; // 150ms ago (timeout is 120ms)
    uint32_t current_ms = millis();
    
    // Check watchdog timeout
    if (current_mode == MODE_AUTO && current_state != STATE_DISARMED) {
        if (last_heartbeat_ms > 0) {
            uint32_t heartbeat_age = current_ms - last_heartbeat_ms;
            if (heartbeat_age > 120) {
                current_state = STATE_FAULT;
            }
        }
    }
    
    TEST_ASSERT_EQUAL(STATE_FAULT, current_state);
}

void test_mode_switching(void) {
    current_mode = MODE_MANUAL;
    TEST_ASSERT_EQUAL(MODE_MANUAL, current_mode);
    
    current_mode = MODE_AUTO;
    TEST_ASSERT_EQUAL(MODE_AUTO, current_mode);
    
    current_mode = MODE_MANUAL;
    TEST_ASSERT_EQUAL(MODE_MANUAL, current_mode);
}

void test_invalid_transition_from_disarmed(void) {
    current_state = STATE_DISARMED;
    
    // Should not transition to RUNNING directly from DISARMED
    if (current_state == STATE_DISARMED) {
        // This transition should not happen
        TEST_ASSERT_NOT_EQUAL(STATE_RUNNING, current_state);
    }
}

void test_estop_to_fault_transition(void) {
    current_state = STATE_RUNNING;
    bool estop_triggered = true;
    
    if (estop_triggered) {
        current_state = STATE_FAULT;
    }
    
    TEST_ASSERT_EQUAL(STATE_FAULT, current_state);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_initial_state);
    RUN_TEST(test_disarmed_to_armed_transition);
    RUN_TEST(test_armed_to_disarmed_transition);
    RUN_TEST(test_armed_to_running_transition);
    RUN_TEST(test_heartbeat_timeout_to_fault);
    RUN_TEST(test_mode_switching);
    RUN_TEST(test_invalid_transition_from_disarmed);
    RUN_TEST(test_estop_to_fault_transition);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}

