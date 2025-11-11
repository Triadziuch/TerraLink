#include "comm.h"
#include "STMDevice.h"
#include "NodeManage.h"

static inline uint8_t get_own_id() {
    return STMDevice_GetDeviceId();
}

static inline bool id_exists(uint8_t id) {
    return (Node_GetNodeById(id) != NULL);
}

static inline void update_wakeup_info(uint8_t node_id, const wakeup_t *wakeup_info) {
    Node_UpdateWakeupInfo(node_id, wakeup_info);
}