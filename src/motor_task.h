#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include "mailbox.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

void motor_task(void *pvParameters);
void motor_task_trigger_emergency(void);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_TASK_H



