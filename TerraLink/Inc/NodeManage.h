/*
 * NodeManage.h
 *
 *  Created on: Nov 1, 2025
 *      Author: Triadziuch
 */

#ifndef INC_NODE_MANAGEMENT_H_
#define INC_NODE_MANAGEMENT_H_

#include "STMDevice.h"
#include "packet.h"

#define MAX_NODES 5

typedef struct{
	data_record_t soil_moisture;
	data_record_t light;
	data_record_t temperature;
} SNodeDataRecords;

typedef struct{
	uint32_t comm_wakeup_timer_interval;
	uint32_t measurement_wakeup_timer_interval;
	uint8_t comm_wakeup_timer_time_awake;
	uint8_t measurement_wakeup_timer_time_awake;
} SNodeConfig;

typedef struct{
    SDevice device;
	SNodeConfig config;

    SNodeDataRecords data_records;
	bool handshake;
	bool started;
} SNode;

extern SNode nodes[MAX_NODES];

void Node_Init(void);

size_t Node_GetNodeCount(void);
const SNode* Node_GetNodeById(uint8_t node_id);
const SNode* Node_GetNodeByIndex(uint8_t index);

void Node_AddDataRecord(uint8_t node_id, const data_record_t* record);
const data_record_t* Node_GetDataRecord(uint8_t node_id, uint8_t data_type);

const SDevice *Node_PrepareNewDevice(const SDeviceUID *argUID);
const SNode *Node_AddNode(const SDevice *argDevice);
void Node_RemoveNode(uint8_t node_id);

void Node_UpdateNodeConfig(uint8_t node_id, const SNodeConfig* config);
void Node_SetStarted(uint8_t node_id, bool started);


#endif /* INC_NODE_MANAGEMENT_H_ */