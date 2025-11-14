#include "steer_task.h"
#include "hardware.h"
#include "mailbox.h"
#include "messages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

#define STEER_TASK_PERIOD_MS 10  // 100 Hz

static mailbox_t *steer_mailbox = NULL;

void steer_task(void *pvParameters) {
    steer_mailbox = (mailbox_t *)pvParameters;
    
    uint16_t current_angle = SERVO_CENTER;
    
    Serial.println("[SteerTask] Steer task started");
    
    while (1) {
        // Read mailbox for steering commands
        topic_t topic;
        command_type_t cmd;
        int32_t value;
        uint32_t ts_ms;
        bool expired;
        
        if (mailbox_read(steer_mailbox, &topic, &cmd, &value, &ts_ms, &expired)) {
            if (!expired) {
                switch (cmd) {
                    case CMD_SET_STEER: {
                        uint16_t new_angle = (uint16_t)value;
                        // Clamp angle to valid range
                        if (new_angle < SERVO_LEFT) {
                            new_angle = SERVO_LEFT;
                        } else if (new_angle > SERVO_RIGHT) {
                            new_angle = SERVO_RIGHT;
                        }
                        // Only execute and print if angle actually changed
                        if (new_angle != current_angle) {
                            current_angle = new_angle;
                            steer_set_angle(current_angle);
                            Serial.print("EVENT:CMD_EXECUTED:SET_STEER:");
                            Serial.println(current_angle);
                            Serial.flush();
                        }
                        break;
                    }
                        
                    case CMD_STOP:
                        // Only execute and print if not already centered
                        if (current_angle != SERVO_CENTER) {
                            current_angle = SERVO_CENTER;
                            steer_set_angle(current_angle);
                            Serial.println("EVENT:CMD_EXECUTED:SET_STEER_CENTER");
                            Serial.flush();
                            Serial.println("[SteerTask] Steering centered (stop command)");
                        }
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(STEER_TASK_PERIOD_MS));
    }
}
