#include <Arduino.h>
#include <unity.h>
#include "messages.h"

void test_command_types(void) {
    TEST_ASSERT_EQUAL(0, CMD_SET_SPEED);
    TEST_ASSERT_EQUAL(1, CMD_SET_STEER);
    TEST_ASSERT_EQUAL(2, CMD_BRAKE_NOW);
    TEST_ASSERT_EQUAL(3, CMD_STOP);
    TEST_ASSERT_EQUAL(4, CMD_SYS_ARM);
    TEST_ASSERT_EQUAL(5, CMD_SYS_DISARM);
    TEST_ASSERT_EQUAL(6, CMD_SYS_MODE);
}

void test_topics(void) {
    TEST_ASSERT_EQUAL(0, TOPIC_MOTOR);
    TEST_ASSERT_EQUAL(1, TOPIC_STEER);
    TEST_ASSERT_EQUAL(2, TOPIC_LIGHTS);
    TEST_ASSERT_EQUAL(3, TOPIC_SYSTEM);
}

void test_modes(void) {
    TEST_ASSERT_EQUAL(0, MODE_MANUAL);
    TEST_ASSERT_EQUAL(1, MODE_AUTO);
    
    system_mode_t mode = MODE_MANUAL;
    TEST_ASSERT_EQUAL(MODE_MANUAL, mode);
    
    mode = MODE_AUTO;
    TEST_ASSERT_EQUAL(MODE_AUTO, mode);
}

void test_states(void) {
    TEST_ASSERT_EQUAL(0, STATE_DISARMED);
    TEST_ASSERT_EQUAL(1, STATE_ARMED);
    TEST_ASSERT_EQUAL(2, STATE_RUNNING);
    TEST_ASSERT_EQUAL(3, STATE_FAULT);
    
    system_state_t state = STATE_DISARMED;
    TEST_ASSERT_EQUAL(STATE_DISARMED, state);
    
    state = STATE_ARMED;
    TEST_ASSERT_EQUAL(STATE_ARMED, state);
    
    state = STATE_RUNNING;
    TEST_ASSERT_EQUAL(STATE_RUNNING, state);
    
    state = STATE_FAULT;
    TEST_ASSERT_EQUAL(STATE_FAULT, state);
}

void test_channel_constants(void) {
    TEST_ASSERT_EQUAL('E', CHANNEL_EMERGENCY);
    TEST_ASSERT_EQUAL('C', CHANNEL_CONTROL);
    TEST_ASSERT_EQUAL('M', CHANNEL_MANAGEMENT);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_command_types);
    RUN_TEST(test_topics);
    RUN_TEST(test_modes);
    RUN_TEST(test_states);
    RUN_TEST(test_channel_constants);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}

