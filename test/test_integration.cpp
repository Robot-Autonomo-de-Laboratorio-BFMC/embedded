#include <Arduino.h>
#include <unity.h>
#include "mailbox.h"
#include "messages.h"

// Integration test: Test mailbox + message flow
mailbox_t motor_mb;
mailbox_t steer_mb;
mailbox_t supervisor_mb;

void test_motor_command_flow(void) {
    mailbox_init(&motor_mb);
    
    // Simulate SET_SPEED command
    bool write_result = mailbox_write(&motor_mb, TOPIC_MOTOR, CMD_SET_SPEED, 128, 200);
    TEST_ASSERT_TRUE(write_result);
    
    // Simulate task reading command
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&motor_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_EQUAL(TOPIC_MOTOR, topic);
    TEST_ASSERT_EQUAL(CMD_SET_SPEED, cmd);
    TEST_ASSERT_EQUAL(128, value);
    TEST_ASSERT_FALSE(expired);
}

void test_steer_command_flow(void) {
    mailbox_init(&steer_mb);
    
    // Simulate SET_STEER command
    mailbox_write(&steer_mb, TOPIC_STEER, CMD_SET_STEER, 90, 200);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&steer_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_EQUAL(TOPIC_STEER, topic);
    TEST_ASSERT_EQUAL(CMD_SET_STEER, cmd);
    TEST_ASSERT_EQUAL(90, value);
}

void test_emergency_brake_flow(void) {
    mailbox_init(&motor_mb);
    
    // Simulate BRAKE_NOW command
    mailbox_write(&motor_mb, TOPIC_EMERGENCY, CMD_BRAKE_NOW, 0, 100);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&motor_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_EQUAL(TOPIC_EMERGENCY, topic);
    TEST_ASSERT_EQUAL(CMD_BRAKE_NOW, cmd);
}

void test_system_mode_switch(void) {
    mailbox_init(&supervisor_mb);
    
    // Switch to AUTO mode
    mailbox_write(&supervisor_mb, TOPIC_SYSTEM, CMD_SYS_MODE, MODE_AUTO, 5000);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&supervisor_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_EQUAL(TOPIC_SYSTEM, topic);
    TEST_ASSERT_EQUAL(CMD_SYS_MODE, cmd);
    TEST_ASSERT_EQUAL(MODE_AUTO, value);
    
    // Switch back to MANUAL
    mailbox_write(&supervisor_mb, TOPIC_SYSTEM, CMD_SYS_MODE, MODE_MANUAL, 5000);
    read_result = mailbox_read(&supervisor_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_EQUAL(MODE_MANUAL, value);
}

void test_multiple_mailboxes_independent(void) {
    mailbox_init(&motor_mb);
    mailbox_init(&steer_mb);
    
    // Write to motor mailbox
    mailbox_write(&motor_mb, TOPIC_MOTOR, CMD_SET_SPEED, 200, 1000);
    
    // Write to steer mailbox
    mailbox_write(&steer_mb, TOPIC_STEER, CMD_SET_STEER, 105, 1000);
    
    // Read from motor mailbox
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    mailbox_read(&motor_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_EQUAL(CMD_SET_SPEED, cmd);
    TEST_ASSERT_EQUAL(200, value);
    
    // Read from steer mailbox
    mailbox_read(&steer_mb, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_EQUAL(CMD_SET_STEER, cmd);
    TEST_ASSERT_EQUAL(105, value);
}

void test_command_priority_emergency(void) {
    mailbox_init(&motor_mb);
    
    // Write normal command
    mailbox_write(&motor_mb, TOPIC_MOTOR, CMD_SET_SPEED, 128, 200);
    delay(10);
    
    // Write emergency command (should overwrite)
    mailbox_write(&motor_mb, TOPIC_EMERGENCY, CMD_BRAKE_NOW, 0, 100);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    mailbox_read(&motor_mb, &topic, &cmd, &value, &ts_ms, &expired);
    // Last writer wins - should be BRAKE_NOW
    TEST_ASSERT_EQUAL(CMD_BRAKE_NOW, cmd);
    TEST_ASSERT_EQUAL(TOPIC_EMERGENCY, topic);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_motor_command_flow);
    RUN_TEST(test_steer_command_flow);
    RUN_TEST(test_emergency_brake_flow);
    RUN_TEST(test_system_mode_switch);
    RUN_TEST(test_multiple_mailboxes_independent);
    RUN_TEST(test_command_priority_emergency);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}

