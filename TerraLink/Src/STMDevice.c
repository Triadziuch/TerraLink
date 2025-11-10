#include "STMDevice.h"
#include "STMDevice_impl.h"

SDevice STMDevice_GetDevice(void)
{
    SDevice device = {
        .type = get_device_type(),
        .id = get_device_id(),
        .uid = get_device_uid()
    };
    return device;
}

uint8_t STMDevice_GetDeviceId(void)
{
    return get_device_id();
}
