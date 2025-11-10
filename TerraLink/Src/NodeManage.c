#include "NodeManage.h"
#include "NodeManage_impl.h"

SNode nodes[MAX_NODES];
static size_t node_count = 0;

static uint8_t get_unused_id(void)
{
    uint8_t used_ids[MAX_NODES] = {0};
    for (size_t i = 0; i < node_count; ++i)
        used_ids[nodes[i].device.id % MAX_NODES] = 1;

    for (uint8_t id = 1; id < MAX_NODES; ++id)
        if (used_ids[id] == 0)
            return id;
    return 0;
}

static SNode *get_node_by_id(uint8_t node_id)
{
    for (size_t i = 0; i < node_count; ++i)
        if (nodes[i].device.id == node_id)
            return &nodes[i];

    return NULL;
}

static SNode *get_node_by_uid(const SDeviceUID *uid)
{
    for (size_t i = 0; i < node_count; ++i)
    {
        if (nodes[i].device.uid.UID_0 == uid->UID_0 &&
            nodes[i].device.uid.UID_1 == uid->UID_1 &&
            nodes[i].device.uid.UID_2 == uid->UID_2)
        {
            return &nodes[i];
        }
    }
    return NULL;
}

void Node_Init(void)
{
    HAL_StatusTypeDef status = load_node_count_from_flash(&node_count);
    if (status != HAL_OK)
    {
        node_count = 0;
        return;
    }

    status = load_nodes_from_flash(node_count);
    if (status != HAL_OK)
    {
        node_count = 0;
        return;
    }

    for (size_t i = 0; i < node_count; ++i)
    {
        nodes[i].handshake = false;
        nodes[i].started = false;

        if (validate_node_connection(&nodes[i]))
        {
            nodes[i].handshake = sync_node_settings(&nodes[i]);
        }
    }
}

size_t Node_GetNodeCount(void)
{
    return node_count;
}

const SNode *Node_GetNodeById(uint8_t node_id)
{
    return get_node_by_id(node_id);
}

const SNode *Node_GetNodeByIndex(uint8_t index)
{
    if (index >= Node_GetNodeCount())
        return NULL;

    return &nodes[index];
}

void Node_AddDataRecord(uint8_t node_id, const data_record_t *record)
{
    SNode *node = get_node_by_id(node_id);
    if (node == NULL)
        return;

    switch (record->type)
    {
    case DATA_SOIL_MOISTURE:
        node->data_records.soil_moisture = *record;
        break;
    case DATA_LIGHT:
        node->data_records.light = *record;
        break;
    case DATA_TEMP:
        node->data_records.temperature = *record;
        break;
    default:
        break;
    }
}

const data_record_t *Node_GetDataRecord(uint8_t node_id, uint8_t data_type)
{
    const SNode *node = Node_GetNodeById(node_id);
    if (node == NULL)
    {
        return NULL;
    }

    switch (data_type)
    {
    case DATA_SOIL_MOISTURE:
        return &node->data_records.soil_moisture;
    case DATA_LIGHT:
        return &node->data_records.light;
    case DATA_TEMP:
        return &node->data_records.temperature;
    default:
        return NULL;
    }
}

const SDevice *Node_PrepareNewDevice(const SDeviceUID *argUID)
{
    if (node_count >= MAX_NODES || argUID == NULL)
    {
        return NULL;
    }

    SDevice *new_device = &nodes[node_count].device;
    new_device->uid = *argUID;
    new_device->id = get_unused_id();
    new_device->type = DEVICE_TYPE_NODE;

    return new_device;
}

const SNode *Node_AddNode(const SDevice *argDevice)
{
    if (node_count >= MAX_NODES || argDevice == NULL)
        return false;

    SNode *new_node = &nodes[node_count];
    new_node->device = *argDevice;
    node_count++;

    save_node_count_to_flash(node_count);
    save_nodes_to_flash(nodes, node_count);

    if (sync_node_settings(new_node))
        new_node->handshake = true;

    return new_node;
}

void Node_SetStarted(uint8_t node_id, bool started)
{
    SNode *node = get_node_by_id(node_id);
    if (node == NULL)
        return;

    node->started = started;
}
