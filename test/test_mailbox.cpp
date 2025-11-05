#include <Arduino.h>
#include <unity.h>
#include "mailbox.h"
#include "messages.h"

// Mailbox test variables
mailbox_t test_mailbox;

void setUp(void) {
    // Initialize mailbox before each test
    mailbox_init(&test_mailbox);
}

void tearDown(void) {
    // Clean up after each test
    // Nothing needed for mailbox cleanup
}

void test_mailbox_init(void) {
    mailbox_t mb;
    mailbox_init(&mb);
    
    TEST_ASSERT_FALSE(mb.valid);
    TEST_ASSERT_EQUAL(0, mb.ts_ms);
    TEST_ASSERT_EQUAL(0, mb.value);
    TEST_ASSERT_NOT_NULL(mb.mutex);
}

void test_mailbox_write_read(void) {
    bool result = mailbox_write(&test_mailbox, TOPIC_MOTOR, CMD_SET_SPEED, 128, 1000);
    TEST_ASSERT_TRUE(result);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&test_mailbox, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_EQUAL(TOPIC_MOTOR, topic);
    TEST_ASSERT_EQUAL(CMD_SET_SPEED, cmd);
    TEST_ASSERT_EQUAL(128, value);
    TEST_ASSERT_FALSE(expired);
}

void test_mailbox_expiration(void) {
    // Write with short TTL
    mailbox_write(&test_mailbox, TOPIC_MOTOR, CMD_SET_SPEED, 128, 50);
    
    // Wait for expiration (simulate with small delay)
    delay(60);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&test_mailbox, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_TRUE(expired); // Should be expired
}

void test_mailbox_no_expiration(void) {
    // Write with no expiration (TTL = 0)
    mailbox_write(&test_mailbox, TOPIC_STEER, CMD_SET_STEER, 90, 0);
    
    delay(100); // Wait longer than normal
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(&test_mailbox, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_TRUE(read_result);
    TEST_ASSERT_FALSE(expired); // Should NOT be expired
}

void test_mailbox_last_writer_wins(void) {
    // Write first value
    mailbox_write(&test_mailbox, TOPIC_MOTOR, CMD_SET_SPEED, 100, 1000);
    delay(10);
    
    // Write second value (should overwrite)
    mailbox_write(&test_mailbox, TOPIC_MOTOR, CMD_SET_SPEED, 200, 1000);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    mailbox_read(&test_mailbox, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_EQUAL(200, value); // Should have latest value
}

void test_mailbox_null_pointer(void) {
    bool result = mailbox_write(NULL, TOPIC_MOTOR, CMD_SET_SPEED, 128, 1000);
    TEST_ASSERT_FALSE(result);
    
    topic_t topic;
    command_type_t cmd;
    int32_t value;
    uint32_t ts_ms;
    bool expired;
    
    bool read_result = mailbox_read(NULL, &topic, &cmd, &value, &ts_ms, &expired);
    TEST_ASSERT_FALSE(read_result);
}

void setup() {
    // Wait for serial monitor to connect
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_mailbox_init);
    RUN_TEST(test_mailbox_write_read);
    RUN_TEST(test_mailbox_expiration);
    RUN_TEST(test_mailbox_no_expiration);
    RUN_TEST(test_mailbox_last_writer_wins);
    RUN_TEST(test_mailbox_null_pointer);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}

