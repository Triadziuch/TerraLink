#include "comm.h"
#include "STMDevice.h"

static inline uint8_t get_own_id() {
    return STMDevice_GetDeviceId();
}