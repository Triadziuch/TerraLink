#include "packet.h"
#include "NodeManage.h"
#include "STMDevice.h"

static inline uint8_t get_own_id() {
    return STMDevice_GetDeviceId();
}

static inline SNodeConfig* get_node_config(uint8_t node_id) {
    const SNode* node = Node_GetNodeById(node_id);
    if (node != NULL) {
        return (SNodeConfig*)&node->config;
    }
    return NULL;
}