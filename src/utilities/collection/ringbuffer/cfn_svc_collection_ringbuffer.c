#include "utilities/cfn_svc_collection.h"
#include <string.h>

typedef struct {
    size_t head;
    size_t tail;
    size_t count;
} ringbuffer_meta_t;

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base) {
    cfn_svc_collection_t *driver = (cfn_svc_collection_t *) base;
    if (!driver || !driver->phy || !driver->phy->user_arg) return CFN_HAL_ERROR_BAD_PARAM;
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    meta->head = 0;
    meta->tail = 0;
    meta->count = 0;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base) {
    cfn_svc_collection_t *driver = (cfn_svc_collection_t *) base;
    if (!driver || !driver->phy || !driver->phy->user_arg) return CFN_HAL_ERROR_BAD_PARAM;
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    meta->head = 0;
    meta->tail = 0;
    meta->count = 0;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(state); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config) {
    cfn_svc_collection_t *driver = (cfn_svc_collection_t *) base;
    driver->config = (const cfn_svc_collection_config_t *) config;
    return CFN_HAL_ERROR_OK;
}
static cfn_hal_error_code_t port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg) {
    cfn_svc_collection_t *driver = (cfn_svc_collection_t *) base;
    driver->cb = (cfn_svc_collection_callback_t) callback;
    driver->cb_user_arg = user_arg;
    return CFN_HAL_ERROR_OK;
}
static cfn_hal_error_code_t port_base_event_enable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_disable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask) { CFN_HAL_UNUSED(base); if(event_mask)*event_mask=0; return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_enable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_disable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask) { CFN_HAL_UNUSED(base); if(error_mask)*error_mask=0; return CFN_HAL_ERROR_OK; }

static cfn_hal_error_code_t port_push_back(cfn_svc_collection_t *driver, const void *item) {
    if (!driver || !driver->phy || !driver->phy->buffer || !driver->phy->user_arg || !driver->config || !item) return CFN_HAL_ERROR_BAD_PARAM;
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    
    if (meta->count >= driver->phy->size) {
        return CFN_HAL_ERROR_MEMORY_FULL;
    }

    uint8_t *buf = (uint8_t *) driver->phy->buffer;
    size_t offset = meta->head * driver->config->item_size;
    
    memcpy(&buf[offset], item, driver->config->item_size);
    
    meta->head = (meta->head + 1) % driver->phy->size;
    meta->count++;
    
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_push_front(cfn_svc_collection_t *driver, const void *item) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(item);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_insert_at(cfn_svc_collection_t *driver, size_t index, const void *item) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(index);
    CFN_HAL_UNUSED(item);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_pop_back(cfn_svc_collection_t *driver, void *item_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(item_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_pop_front(cfn_svc_collection_t *driver, void *item_out) {
    if (!driver || !driver->phy || !driver->phy->buffer || !driver->phy->user_arg || !driver->config || !item_out) return CFN_HAL_ERROR_BAD_PARAM;
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    
    if (meta->count == 0) {
        return CFN_HAL_ERROR_NOT_FOUND;
    }

    const uint8_t *buf = (const uint8_t *) driver->phy->buffer;
    size_t offset = meta->tail * driver->config->item_size;
    
    memcpy(item_out, &buf[offset], driver->config->item_size);
    
    meta->tail = (meta->tail + 1) % driver->phy->size;
    meta->count--;
    
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_remove_at(cfn_svc_collection_t *driver, size_t index, void *item_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(index);
    CFN_HAL_UNUSED(item_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_peek_back(cfn_svc_collection_t *driver, void *item_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(item_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_peek_front(cfn_svc_collection_t *driver, void *item_out) {
    if (!driver || !driver->phy || !driver->phy->buffer || !driver->phy->user_arg || !driver->config || !item_out) return CFN_HAL_ERROR_BAD_PARAM;
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    
    if (meta->count == 0) {
        return CFN_HAL_ERROR_NOT_FOUND;
    }

    const uint8_t *buf = (const uint8_t *) driver->phy->buffer;
    size_t offset = meta->tail * driver->config->item_size;
    
    memcpy(item_out, &buf[offset], driver->config->item_size);
    
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_peek_at(cfn_svc_collection_t *driver, size_t index, void *item_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(index);
    CFN_HAL_UNUSED(item_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_get_size(cfn_svc_collection_t *driver, size_t *size_out) {
    if (!driver || !driver->phy || !driver->phy->user_arg || !size_out) return CFN_HAL_ERROR_BAD_PARAM;
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    *size_out = meta->count;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_get_capacity(cfn_svc_collection_t *driver, size_t *capacity_out) {
    if (!driver || !driver->phy || !capacity_out) return CFN_HAL_ERROR_BAD_PARAM;
    *capacity_out = driver->phy->size;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_is_empty(cfn_svc_collection_t *driver, bool *is_empty_out) {
    if (!driver || !driver->phy || !driver->phy->user_arg || !is_empty_out) return CFN_HAL_ERROR_BAD_PARAM;
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    *is_empty_out = (meta->count == 0);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_is_full(cfn_svc_collection_t *driver, bool *is_full_out) {
    if (!driver || !driver->phy || !driver->phy->user_arg || !is_full_out) return CFN_HAL_ERROR_BAD_PARAM;
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    *is_full_out = (meta->count >= driver->phy->size);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_clear(cfn_svc_collection_t *driver) {
    if (!driver || !driver->phy || !driver->phy->user_arg) return CFN_HAL_ERROR_BAD_PARAM;
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    meta->head = 0;
    meta->tail = 0;
    meta->count = 0;
    return CFN_HAL_ERROR_OK;
}

static const cfn_svc_collection_api_t API = {
    .base = {
        .init = port_base_init,
        .deinit = port_base_deinit,
        .power_state_set = port_base_power_state_set,
        .config_set = port_base_config_set,
        .callback_register = port_base_callback_register,
        .event_enable = port_base_event_enable,
        .event_disable = port_base_event_disable,
        .event_get = port_base_event_get,
        .error_enable = port_base_error_enable,
        .error_disable = port_base_error_disable,
        .error_get = port_base_error_get,
    },
    .push_back = port_push_back,
    .push_front = port_push_front,
    .insert_at = port_insert_at,
    .pop_back = port_pop_back,
    .pop_front = port_pop_front,
    .remove_at = port_remove_at,
    .peek_back = port_peek_back,
    .peek_front = port_peek_front,
    .peek_at = port_peek_at,
    .get_size = port_get_size,
    .get_capacity = port_get_capacity,
    .is_empty = port_is_empty,
    .is_full = port_is_full,
    .clear = port_clear,
};

cfn_hal_error_code_t cfn_svc_collection_ringbuffer_construct(cfn_svc_collection_t *driver, const cfn_svc_collection_config_t *config, const cfn_svc_collection_phy_t *phy)
{
    if ((driver == NULL) || (phy == NULL)) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = &API;
    driver->base.type = CFN_SVC_TYPE_COLLECTION;
    driver->base.status = CFN_HAL_DRIVER_STATUS_CONSTRUCTED;
    driver->config = config;
    driver->phy = phy;
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_svc_collection_ringbuffer_destruct(cfn_svc_collection_t *driver)
{
    if (driver == NULL) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = NULL;
    driver->base.type = CFN_SVC_TYPE_COLLECTION;
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    driver->config = NULL;
    driver->phy = NULL;
    return CFN_HAL_ERROR_OK;
}
