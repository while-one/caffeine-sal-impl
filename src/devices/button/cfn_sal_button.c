#include "devices/cfn_sal_button.h"
#include "cfn_hal_gpio.h"

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config)
{
    cfn_sal_button_t *driver = (cfn_sal_button_t *) base;
    driver->config           = (const cfn_sal_button_config_t *) config;
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

static cfn_hal_error_code_t port_get_state(cfn_sal_button_t *driver, cfn_sal_button_state_t *state_out)
{
    if (!driver || !driver->phy || !driver->phy->instance || !state_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    cfn_hal_gpio_state_t       gpio_state;
    cfn_hal_error_code_t       err = cfn_hal_gpio_pin_read(pin->port, pin->pin, &gpio_state);

    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    bool logical_pressed = false;
    if (driver->config && driver->config->active_low)
    {
        logical_pressed = (gpio_state == CFN_HAL_GPIO_STATE_LOW);
    }
    else
    {
        logical_pressed = (gpio_state == CFN_HAL_GPIO_STATE_HIGH);
    }

    *state_out = logical_pressed ? CFN_SAL_BUTTON_STATE_PRESSED : CFN_SAL_BUTTON_STATE_RELEASED;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_is_pressed(cfn_sal_button_t *driver, bool *pressed_out)
{
    if (!pressed_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_button_state_t state;
    cfn_hal_error_code_t   err = port_get_state(driver, &state);
    if (err == CFN_HAL_ERROR_OK)
    {
        *pressed_out = (state == CFN_SAL_BUTTON_STATE_PRESSED);
    }
    return err;
}

static cfn_hal_error_code_t port_is_released(cfn_sal_button_t *driver, bool *released_out)
{
    if (!released_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_button_state_t state;
    cfn_hal_error_code_t   err = port_get_state(driver, &state);
    if (err == CFN_HAL_ERROR_OK)
    {
        *released_out = (state == CFN_SAL_BUTTON_STATE_RELEASED);
    }
    return err;
}

static const cfn_sal_button_api_t API = {
    .base = {
        .init = NULL,
        .deinit = NULL,
        .power_state_set = NULL,
        .config_set = port_base_config_set,
        .callback_register = NULL,
        .event_enable = NULL,
        .event_disable = NULL,
        .event_get = port_base_event_get,
        .error_enable = NULL,
        .error_disable = NULL,
        .error_get = port_base_error_get,
    },
    .get_state = port_get_state,
    .set_debounce_time = NULL,
    .is_pressed = port_is_pressed,
    .is_released = port_is_released,
};

cfn_hal_error_code_t cfn_sal_button_construct(cfn_sal_button_t              *driver,
                                              const cfn_sal_button_config_t *config,
                                              const cfn_sal_phy_t           *phy,
                                              void                          *dependency,
                                              cfn_sal_button_callback_t      callback,
                                              void                          *user_arg)
{
    if ((driver == NULL) || (phy == NULL))
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_button_populate(driver, 0, dependency, &API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_button_destruct(cfn_sal_button_t *driver)
{
    if (driver == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_button_populate(driver, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
