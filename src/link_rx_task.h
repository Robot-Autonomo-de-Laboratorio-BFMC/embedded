#ifndef LINK_RX_TASK_H
#define LINK_RX_TASK_H

#include "mailbox.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mailbox_t *motor_mailbox;
    mailbox_t *steer_mailbox;
    mailbox_t *lights_mailbox;
    mailbox_t *supervisor_mailbox;
} link_rx_params_t;

void link_rx_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // LINK_RX_TASK_H



