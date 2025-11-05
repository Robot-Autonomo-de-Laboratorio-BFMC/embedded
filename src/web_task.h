#ifndef WEB_TASK_H
#define WEB_TASK_H

#include "mailbox.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mailbox_t *motor_mailbox;
    mailbox_t *steer_mailbox;
    mailbox_t *lights_mailbox;
    mailbox_t *supervisor_mailbox;
} web_task_params_t;

void web_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // WEB_TASK_H



