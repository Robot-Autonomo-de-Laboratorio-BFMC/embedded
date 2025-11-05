#include <Arduino.h>
#include <unity.h>
#include <string.h>
#include <stdlib.h>

// Simulate the parse function from link_rx_task
bool parse_uart_message(const char *msg, char *channel, char *cmd, int32_t *value) {
    char *msg_copy = strdup(msg);
    if (msg_copy == NULL) {
        return false;
    }
    
    // Remove newline/carriage return
    char *p = strchr(msg_copy, '\n');
    if (p) *p = '\0';
    p = strchr(msg_copy, '\r');
    if (p) *p = '\0';
    
    // Parse channel (single character)
    if (strlen(msg_copy) < 3) {
        free(msg_copy);
        return false;
    }
    *channel = msg_copy[0];
    
    // Find command start
    char *cmd_start = strchr(msg_copy, ':');
    if (cmd_start == NULL) {
        free(msg_copy);
        return false;
    }
    cmd_start++;
    
    // Find value separator
    char *value_start = strchr(cmd_start, ':');
    if (value_start != NULL) {
        *value_start = '\0';
        value_start++;
        *value = atoi(value_start);
    } else {
        *value = 0;
    }
    
    strncpy(cmd, cmd_start, 32);
    cmd[31] = '\0';
    
    free(msg_copy);
    return true;
}

void test_parse_emergency_message(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("E:BRAKE_NOW", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('E', channel);
    TEST_ASSERT_EQUAL_STRING("BRAKE_NOW", cmd);
    TEST_ASSERT_EQUAL(0, value);
}

void test_parse_control_message_with_value(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("C:SET_SPEED:128", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('C', channel);
    TEST_ASSERT_EQUAL_STRING("SET_SPEED", cmd);
    TEST_ASSERT_EQUAL(128, value);
}

void test_parse_steer_message(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("C:SET_STEER:90", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('C', channel);
    TEST_ASSERT_EQUAL_STRING("SET_STEER", cmd);
    TEST_ASSERT_EQUAL(90, value);
}

void test_parse_management_message(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("M:SYS_ARM", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('M', channel);
    TEST_ASSERT_EQUAL_STRING("SYS_ARM", cmd);
    TEST_ASSERT_EQUAL(0, value);
}

void test_parse_message_with_newline(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("C:SET_SPEED:255\n", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('C', channel);
    TEST_ASSERT_EQUAL_STRING("SET_SPEED", cmd);
    TEST_ASSERT_EQUAL(255, value);
}

void test_parse_message_with_carriage_return(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("M:SYS_MODE:AUTO\r", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('M', channel);
    TEST_ASSERT_EQUAL_STRING("SYS_MODE", cmd);
    // Note: AUTO won't be parsed as integer, will be 0
}

void test_parse_invalid_message_too_short(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("E:", &channel, cmd, &value);
    TEST_ASSERT_FALSE(result);
}

void test_parse_invalid_message_no_colon(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("BRAKE_NOW", &channel, cmd, &value);
    TEST_ASSERT_FALSE(result);
}

void test_parse_negative_value(void) {
    char channel;
    char cmd[32];
    int32_t value = 0;
    
    bool result = parse_uart_message("C:SET_STEER:-45", &channel, cmd, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL('C', channel);
    TEST_ASSERT_EQUAL_STRING("SET_STEER", cmd);
    TEST_ASSERT_EQUAL(-45, value);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_emergency_message);
    RUN_TEST(test_parse_control_message_with_value);
    RUN_TEST(test_parse_steer_message);
    RUN_TEST(test_parse_management_message);
    RUN_TEST(test_parse_message_with_newline);
    RUN_TEST(test_parse_message_with_carriage_return);
    RUN_TEST(test_parse_invalid_message_too_short);
    RUN_TEST(test_parse_invalid_message_no_colon);
    RUN_TEST(test_parse_negative_value);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}

