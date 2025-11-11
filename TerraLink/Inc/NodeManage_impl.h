#include "NodeManage.h"
#include "Flash.h"
#include "comm.h"

#define NODE_COUNT_FLASH_ADDRESS 0x08080000
#define NODE_FLASH_ADDRESS_START 0x08081000

#define DEFAULT_COMM_WAKEUP_TIMER_INTERVAL 15
#define DEFAULT_COMM_WAKEUP_TIMER_TIME_AWAKE 5
#define DEFAULT_MEASUREMENT_WAKEUP_TIMER_INTERVAL 60
#define DEFAULT_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE 5

static inline SNodeConfig get_default_node_config(void)
{
    SNodeConfig config = {
        .comm_wakeup_timer_interval = DEFAULT_COMM_WAKEUP_TIMER_INTERVAL,
        .measurement_wakeup_timer_interval = DEFAULT_MEASUREMENT_WAKEUP_TIMER_INTERVAL,
        .comm_wakeup_timer_time_awake = DEFAULT_COMM_WAKEUP_TIMER_TIME_AWAKE,
        .measurement_wakeup_timer_time_awake = DEFAULT_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE
    };

    return config;
}

static inline HAL_StatusTypeDef load_node_count_from_flash(size_t *node_count)
{
    return Flash_read_check_flag(NODE_COUNT_FLASH_ADDRESS, (uint8_t *)node_count, sizeof(size_t));
}

static inline HAL_StatusTypeDef load_nodes_from_flash(size_t node_count)
{
    return Flash_read(NODE_FLASH_ADDRESS_START, (uint8_t *)nodes, sizeof(SNode) * node_count);
}

static inline HAL_StatusTypeDef save_node_count_to_flash(size_t node_count)
{
    uint8_t buffer[sizeof(size_t) + sizeof(uint8_t)];
    memcpy(buffer, &node_count, sizeof(size_t));
    buffer[sizeof(size_t)] = 0xA5;
    return Flash_write(NODE_COUNT_FLASH_ADDRESS, buffer, sizeof(buffer));
}

// TODO: Add error handling i debugowanie info
static inline HAL_StatusTypeDef save_nodes_to_flash(const SNode *nodes, size_t node_count)
{
    return Flash_write(NODE_FLASH_ADDRESS_START, (const uint8_t *)nodes, sizeof(SNode) * node_count);
}

static inline bool validate_node_connection(const SNode *node)
{
    return !!(comm_test_conn(node->device.id));
}

static inline bool sync_node_settings(const SNode *node)
{
    return Comm_SyncConfig(node->device.id);
}

static inline uint32_t get_timestamp(void)
{
    // TODO: Poprawić na RTC
    return HAL_GetTick();
}