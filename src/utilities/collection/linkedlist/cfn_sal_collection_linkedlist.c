#include "utilities/cfn_sal_collection.h"
#include <string.h>

typedef struct list_node_s
{
    struct list_node_s *next;
    struct list_node_s *prev;
    uint8_t             data[];
} list_node_t;

typedef struct
{
    list_node_t *head;
    list_node_t *tail;
    size_t       count;
    size_t       capacity;
} list_meta_t;

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base)
{
    cfn_sal_collection_t *driver = (cfn_sal_collection_t *) base;
    if (!driver || !driver->phy || !driver->phy->user_arg)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;
    meta->head        = NULL;
    meta->tail        = NULL;
    meta->count       = 0;
    meta->capacity    = (size_t) driver->phy->instance;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_collection_t *driver = (cfn_sal_collection_t *) base;
    if (!driver || !driver->phy || !driver->phy->user_arg)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;
    meta->head        = NULL;
    meta->tail        = NULL;
    meta->count       = 0;
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

/* In this implementation, we assume the physical 'buffer' is an array of node structures
   pre-allocated by the user. */

static list_node_t *get_free_node(cfn_sal_collection_t *driver)
{
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;
    if (meta->count >= meta->capacity)
    {
        return NULL;
    }

    size_t   node_size = sizeof(list_node_t) + driver->config->item_size;
    uint8_t *pool      = (uint8_t *) driver->phy->handle;

    /* Simple linear scan for unlinked node - normally you'd use a free list */
    for (size_t i = 0; i < meta->capacity; i++)
    {
        list_node_t *n      = (list_node_t *) (pool + (i * node_size));
        /* check if node is in the list */
        bool         linked = false;
        list_node_t *curr   = meta->head;
        while (curr)
        {
            if (curr == n)
            {
                linked = true;
                break;
            }
            curr = curr->next;
        }
        if (!linked)
        {
            return n;
        }
    }
    return NULL;
}

static cfn_hal_error_code_t port_push_back(cfn_sal_collection_t *driver, const void *item)
{
    if (!driver || !item)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;

    list_node_t *node = get_free_node(driver);
    if (!node)
    {
        return CFN_HAL_ERROR_MEMORY_FULL;
    }

    memcpy(node->data, item, driver->config->item_size);
    node->next = NULL;
    node->prev = meta->tail;

    if (meta->tail)
    {
        meta->tail->next = node;
    }
    meta->tail = node;
    if (!meta->head)
    {
        meta->head = node;
    }

    meta->count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_push_front(cfn_sal_collection_t *driver, const void *item)
{
    if (!driver || !item)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;

    list_node_t *node = get_free_node(driver);
    if (!node)
    {
        return CFN_HAL_ERROR_MEMORY_FULL;
    }

    memcpy(node->data, item, driver->config->item_size);
    node->next = meta->head;
    node->prev = NULL;

    if (meta->head)
    {
        meta->head->prev = node;
    }
    meta->head = node;
    if (!meta->tail)
    {
        meta->tail = node;
    }

    meta->count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_pop_front(cfn_sal_collection_t *driver, void *item_out)
{
    if (!driver || !item_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;
    if (!meta->head)
    {
        return CFN_HAL_ERROR_NOT_FOUND;
    }

    list_node_t *node = meta->head;
    memcpy(item_out, node->data, driver->config->item_size);

    meta->head = node->next;
    if (meta->head)
    {
        meta->head->prev = NULL;
    }
    else
    {
        meta->tail = NULL;
    }

    meta->count--;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_peek_front(cfn_sal_collection_t *driver, void *item_out)
{
    if (!driver || !item_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    const list_meta_t *meta = (const list_meta_t *) driver->phy->user_arg;
    if (!meta->head)
    {
        return CFN_HAL_ERROR_NOT_FOUND;
    }
    memcpy(item_out, meta->head->data, driver->config->item_size);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_get_size(cfn_sal_collection_t *driver, size_t *size_out)
{
    if (!driver || !size_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    const list_meta_t *meta = (const list_meta_t *) driver->phy->user_arg;
    *size_out               = meta->count;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_clear(cfn_sal_collection_t *driver)
{
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    list_meta_t *meta = (list_meta_t *) driver->phy->user_arg;
    meta->head        = NULL;
    meta->tail        = NULL;
    meta->count       = 0;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_get_capacity(cfn_sal_collection_t *driver, size_t *capacity_out)
{
    if (capacity_out)
    {
        *capacity_out = (size_t) driver->phy->instance;
    }
    return CFN_HAL_ERROR_OK;
}
static cfn_hal_error_code_t port_is_empty(cfn_sal_collection_t *driver, bool *is_empty_out)
{
    if (!is_empty_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    size_t               size;
    cfn_hal_error_code_t err = port_get_size(driver, &size);
    if (err == CFN_HAL_ERROR_OK)
    {
        *is_empty_out = (size == 0);
    }
    return err;
}
static cfn_hal_error_code_t port_is_full(cfn_sal_collection_t *driver, bool *is_full_out)
{
    CFN_HAL_UNUSED(driver);
    if (is_full_out)
    {
        *is_full_out = false;
    }
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
    .push_front = port_push_front,
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

cfn_hal_error_code_t cfn_sal_collection_construct(cfn_sal_collection_t              *driver,
                                                  const cfn_sal_collection_config_t *config,
                                                  const cfn_sal_phy_t               *phy,
                                                  void                              *dependency,
                                                  cfn_sal_collection_callback_t      callback,
                                                  void                              *user_arg)
{
    if ((driver == NULL) || (phy == NULL))
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_collection_populate(driver, 0, dependency, &API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_collection_destruct(cfn_sal_collection_t *driver)
{
    if (driver == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_collection_populate(driver, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
