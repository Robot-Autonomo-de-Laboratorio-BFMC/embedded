#include "motor_task.h"
#include "hardware.h"
#include "mailbox.h"
#include "messages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

#define MOTOR_TASK_PERIOD_MS 10  // 100 Hz
#define EMERGENCY_NOTIFICATION_BIT (1 << 0)

static mailbox_t *motor_mailbox = NULL;
static TaskHandle_t motor_task_handle = NULL;

void motor_task(void *pvParameters) {
    motor_mailbox = (mailbox_t *)pvParameters;
    motor_task_handle = xTaskGetCurrentTaskHandle();
    
    uint8_t current_speed = 0;
    bool motor_direction = true; // forward
    
    Serial.println("[MotorTask] Motor task started");
    
    while (1) {
        // Check for emergency notifications first (<1ms response)
        uint32_t notification_value = ulTaskNotifyTake(pdTRUE, 0);
        if (notification_value & EMERGENCY_NOTIFICATION_BIT) {
            Serial.println("[MotorTask] Emergency brake triggered!");
            motor_stop();
            lights_set_reverse(false);
            vTaskDelay(pdMS_TO_TICKS(MOTOR_TASK_PERIOD_MS));
            continue;
        }
        
        // Read mailbox for motor commands
        topic_t topic;
        command_type_t cmd;
        int32_t value;
        uint32_t ts_ms;
        bool expired;
        
        if (mailbox_read(motor_mailbox, &topic, &cmd, &value, &ts_ms, &expired)) {
            if (!expired) {
                switch (cmd) {
                    case CMD_SET_SPEED:
                        current_speed = (uint8_t)value;
                        if (current_speed > MOTOR_SPEED_MAX) {
                            current_speed = MOTOR_SPEED_MAX;
                        }
                        motor_set_speed(current_speed);
                        break;
                        
                    case CMD_BRAKE_NOW:
                    case CMD_STOP:
                        motor_stop();
                        lights_set_reverse(false);
                        current_speed = 0;
                        Serial.println("[MotorTask] Motor stopped (brake/stop command)");
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        // Handle direction changes (typically from separate forward/back commands)
        // Direction is handled by direct GPIO writes, but we track it for reverse lights
        bool new_direction = digitalRead(GPIO_MOTOR_IN3);
        if (new_direction != motor_direction) {
            motor_direction = new_direction;
            lights_set_reverse(!motor_direction); // Reverse lights when going backwards
        }
        
        vTaskDelay(pdMS_TO_TICKS(MOTOR_TASK_PERIOD_MS));
    }
}

void motor_task_trigger_emergency(void) {
    if (motor_task_handle != NULL) {
        xTaskNotify(motor_task_handle, EMERGENCY_NOTIFICATION_BIT, eSetBits);
    }
}
