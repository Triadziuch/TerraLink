#include "STMDevice.h"
#include "stm32l0xx_hal.h"

#define DEVICE_TYPE DEVICE_TYPE_NODE
#define DEVICE_ID   0

static inline SDeviceUID get_device_uid()
{
    SDeviceUID uid;
    uid.UID_0 = HAL_GetUIDw0();
    uid.UID_1 = HAL_GetUIDw1();
    uid.UID_2 = HAL_GetUIDw2();
    return uid;
}

static inline EDeviceType get_device_type(void){
    return DEVICE_TYPE_NODE;
}

static inline uint8_t get_device_id(void){
    return DEVICE_ID;
}

