#include "utilities/cfn_sal_collection.h"
#include "cfn_sal_collection_ringbuffer_port.h"
#include <string.h>

typedef struct
{
    size_t head;
    size_t tail;
    size_t count;
} ringbuffer_meta_t;

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base)
{
    cfn_sal_collection_t *driver = (cfn_sal_collection_t *) base;
    if (!driver || !driver->phy || !driver->phy->user_arg)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    meta->head              = 0;
    meta->tail              = 0;
    meta->count             = 0;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_collection_t *driver = (cfn_sal_collection_t *) base;
    if (!driver || !driver->phy || !driver->phy->user_arg)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    meta->head              = 0;
    meta->tail              = 0;
    meta->count             = 0;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config)
{
    cfn_sal_collection_t *driver = (cfn_sal_collection_t *) base;
    driver->config               = (const cfn_sal_collection_config_t *) config;
    return CFN_HAL_ERROR_OK;
}
static cfn_hal_error_code_t
port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg)
{
    cfn_sal_collection_t *driver = (cfn_sal_collection_t *) base;
    driver->cb                   = (cfn_sal_collection_callback_t) callback;
    driver->cb_user_arg          = user_arg;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask)
{
    CFN_HAL_UNUSED(base);
    if (event_mask)
    {
        *event_mask = 0;
    }
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask)
{
    CFN_HAL_UNUSED(base);
    if (error_mask)
    {
        *error_mask = 0;
    }
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_push_back(cfn_sal_collection_t *driver, const void *item)
{
    if (!driver || !driver->phy || !driver->phy->handle || !driver->phy->user_arg || !driver->config || !item)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;

    if (meta->count >= (size_t) driver->phy->instance)
    {
        return CFN_HAL_ERROR_MEMORY_FULL;
    }

    uint8_t *buf    = (uint8_t *) driver->phy->handle;
    size_t   offset = meta->head * driver->config->item_size;

    memcpy(&buf[offset], item, driver->config->item_size);

    meta->head = (meta->head + 1) % (size_t) driver->phy->instance;
    meta->count++;

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_pop_front(cfn_sal_collection_t *driver, void *item_out)
{
    if (!driver || !driver->phy || !driver->phy->handle || !driver->phy->user_arg || !driver->config || !item_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;

    if (meta->count == 0)
    {
        return CFN_HAL_ERROR_NOT_FOUND;
    }

    const uint8_t *buf    = (const uint8_t *) driver->phy->handle;
    size_t         offset = meta->tail * driver->config->item_size;

    memcpy(item_out, &buf[offset], driver->config->item_size);

    meta->tail = (meta->tail + 1) % (size_t) driver->phy->instance;
    meta->count--;

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_peek_front(cfn_sal_collection_t *driver, void *item_out)
{
    if (!driver || !driver->phy || !driver->phy->handle || !driver->phy->user_arg || !driver->config || !item_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;

    if (meta->count == 0)
    {
        return CFN_HAL_ERROR_NOT_FOUND;
    }

    const uint8_t *buf    = (const uint8_t *) driver->phy->handle;
    size_t         offset = meta->tail * driver->config->item_size;

    memcpy(item_out, &buf[offset], driver->config->item_size);

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_get_size(cfn_sal_collection_t *driver, size_t *size_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !size_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    *size_out                     = meta->count;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_get_capacity(cfn_sal_collection_t *driver, size_t *capacity_out)
{
    if (!driver || !driver->phy || !capacity_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    *capacity_out = (size_t) driver->phy->instance;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_is_empty(cfn_sal_collection_t *driver, bool *is_empty_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !is_empty_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    *is_empty_out                 = (meta->count == 0);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_is_full(cfn_sal_collection_t *driver, bool *is_full_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !is_full_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    const ringbuffer_meta_t *meta = (const ringbuffer_meta_t *) driver->phy->user_arg;
    *is_full_out                  = (meta->count >= (size_t) driver->phy->instance);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_clear(cfn_sal_collection_t *driver)
{
    if (!driver || !driver->phy || !driver->phy->user_arg)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    ringbuffer_meta_t *meta = (ringbuffer_meta_t *) driver->phy->user_arg;
    meta->head              = 0;
    meta->tail              = 0;
    meta->count             = 0;
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_collection_api_t API = {
    .base = {
        .init = port_base_init,
        .deinit = port_base_deinit,
        .power_state_set = NULL,
        .config_set = port_base_config_set,
        .callback_register = port_base_callback_register,
        .event_enable = NULL,
        .event_disable = NULL,
        .event_get = port_base_event_get,
        .error_enable = NULL,
        .error_disable = NULL,
        .error_get = port_base_error_get,
    },
    .push_back = port_push_back,
    .push_front = NULL,
    .insert_at = NULL,
    .pop_back = NULL,
    .pop_front = port_pop_front,
    .remove_at = NULL,
    .peek_back = NULL,
    .peek_front = port_peek_front,
    .peek_at = NULL,
    .get_size = port_get_size,
    .get_capacity = port_get_capacity,
    .is_empty = port_is_empty,
    .is_full = port_is_full,
    .clear = port_clear,
};

cfn_hal_error_code_t cfn_sal_collection_ringbuffer_construct(cfn_sal_collection_t              *driver,
                                                             const cfn_sal_collection_config_t *config,
                                                             const cfn_sal_phy_t               *phy,
                                                             cfn_sal_collection_callback_t      callback,
                                                             void                              *user_arg)
{
    if ((driver == NULL) || (phy == NULL))
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_collection_populate(driver, 0, &API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_collection_ringbuffer_destruct(cfn_sal_collection_t *driver)
{
    if (driver == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_collection_populate(driver, 0, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
