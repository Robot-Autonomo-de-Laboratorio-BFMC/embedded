#include "lights_task.h"
#include "hardware.h"
#include "mailbox.h"
#include "messages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

#define LIGHTS_TASK_PERIOD_MS 1000  // 1 Hz for auto-mode checks
#define LDR_CHECK_PERIOD_MS 1000

typedef enum {
    LIGHTS_MODE_OFF,
    LIGHTS_MODE_ON,
    LIGHTS_MODE_AUTO
} lights_mode_t;

static mailbox_t *lights_mailbox = NULL;
static lights_mode_t current_mode = LIGHTS_MODE_OFF;
static uint32_t last_ldr_check = 0;

void lights_task(void *pvParameters) {
    lights_mailbox = (mailbox_t *)pvParameters;
    
    Serial.println("[LightsTask] Lights task started");
    
    while (1) {
        // Read mailbox for lights commands
        topic_t topic;
        command_type_t cmd;
        int32_t value;
        uint32_t ts_ms;
        bool expired;
        
        if (mailbox_read(lights_mailbox, &topic, &cmd, &value, &ts_ms, &expired)) {
            if (!expired) {
                switch (cmd) {
                    case CMD_LIGHTS_ON:
                        current_mode = LIGHTS_MODE_ON;
                        lights_set_headlights(true);
                        Serial.println("[LightsTask] Headlights ON");
                        break;
                        
                    case CMD_LIGHTS_OFF:
                        current_mode = LIGHTS_MODE_OFF;
                        lights_set_headlights(false);
                        Serial.println("[LightsTask] Headlights OFF");
                        break;
                        
                    case CMD_LIGHTS_AUTO:
                        current_mode = LIGHTS_MODE_AUTO;
                        Serial.println("[LightsTask] Headlights AUTO mode");
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        // Handle auto mode
        if (current_mode == LIGHTS_MODE_AUTO) {
            uint32_t current_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (current_ms - last_ldr_check >= LDR_CHECK_PERIOD_MS) {
                uint16_t ldr_value = ldr_read();
                
                if (ldr_value < LDR_THRESHOLD) {
                    // High light intensity
                    lights_set_headlights(false);
                } else {
                    // Low light intensity
                    lights_set_headlights(true);
                }
                
                last_ldr_check = current_ms;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(LIGHTS_TASK_PERIOD_MS));
    }
}
