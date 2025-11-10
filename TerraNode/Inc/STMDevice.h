#ifndef INC_STMDEVICE_H_
#define INC_STMDEVICE_H_

#include "stdint.h"

typedef enum {
    DEVICE_TYPE_UNDEFINED = 0,
    DEVICE_TYPE_NODE = 1,
    DEVICE_TYPE_GATEWAY = 2
} EDeviceType;

typedef struct {
	uint32_t UID_0;
	uint32_t UID_1;
	uint32_t UID_2;
} SDeviceUID;

typedef struct{
    SDeviceUID uid;
    uint8_t id;
    EDeviceType type;
} SDevice;

SDevice STMDevice_GetDevice(void);
uint8_t STMDevice_GetDeviceId(void);

#endif /* INC_STMDEVICE_H_ */

