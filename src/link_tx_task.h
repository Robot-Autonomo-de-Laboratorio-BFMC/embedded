#ifndef LINK_TX_TASK_H
#define LINK_TX_TASK_H

#include "messages.h"

#ifdef __cplusplus
extern "C" {
#endif

void link_tx_task(void *pvParameters);
void link_tx_send_status(system_mode_t mode, system_state_t state, uint32_t heartbeat_age_ms);

#ifdef __cplusplus
}
#endif

#endif // LINK_TX_TASK_H



