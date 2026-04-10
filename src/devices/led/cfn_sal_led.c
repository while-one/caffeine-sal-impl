#include "devices/cfn_sal_led.h"
#include "cfn_hal_gpio.h"

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base)
{
    cfn_sal_led_t             *driver = (cfn_sal_led_t *) base;
    cfn_hal_gpio_pin_handle_t *pin    = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;

    if (pin == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_hal_gpio_state_t state = CFN_HAL_GPIO_STATE_LOW;
    if (driver->config && driver->config->active_low)
    {
        state = CFN_HAL_GPIO_STATE_HIGH;
    }

    return cfn_hal_gpio_pin_write(pin->port, pin->pin, state);
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_led_t             *driver = (cfn_sal_led_t *) base;
    cfn_hal_gpio_pin_handle_t *pin    = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;

    if (pin == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_hal_gpio_state_t state = CFN_HAL_GPIO_STATE_LOW;
    if (driver->config && driver->config->active_low)
    {
        state = CFN_HAL_GPIO_STATE_HIGH;
    }

    return cfn_hal_gpio_pin_write(pin->port, pin->pin, state);
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

static cfn_hal_error_code_t port_set_state(cfn_sal_led_t *driver, cfn_sal_led_state_t state)
{
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    cfn_hal_gpio_state_t       gpio_state;

    if (state == CFN_SAL_LED_STATE_ON)
    {
        gpio_state = (driver->config && driver->config->active_low) ? CFN_HAL_GPIO_STATE_LOW : CFN_HAL_GPIO_STATE_HIGH;
    }
    else
    {
        gpio_state = (driver->config && driver->config->active_low) ? CFN_HAL_GPIO_STATE_HIGH : CFN_HAL_GPIO_STATE_LOW;
    }

    return cfn_hal_gpio_pin_write(pin->port, pin->pin, gpio_state);
}

static cfn_hal_error_code_t port_get_state(cfn_sal_led_t *driver, cfn_sal_led_state_t *state_out)
{
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    cfn_hal_gpio_state_t       gpio_state;
    cfn_hal_error_code_t       err = cfn_hal_gpio_pin_read(pin->port, pin->pin, &gpio_state);

    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    bool logical_on;
    if (driver->config && driver->config->active_low)
    {
        logical_on = (gpio_state == CFN_HAL_GPIO_STATE_LOW);
    }
    else
    {
        logical_on = (gpio_state == CFN_HAL_GPIO_STATE_HIGH);
    }

    *state_out = logical_on ? CFN_SAL_LED_STATE_ON : CFN_SAL_LED_STATE_OFF;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_toggle(cfn_sal_led_t *driver)
{
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    return cfn_hal_gpio_pin_toggle(pin->port, pin->pin);
}

static const cfn_sal_led_api_t API = {
    .base = {
        .init = port_base_init,
        .deinit = port_base_deinit,
        .power_state_set = NULL,
        .config_set = NULL,
        .callback_register = NULL,
        .event_enable = NULL,
        .event_disable = NULL,
        .event_get = port_base_event_get,
        .error_enable = NULL,
        .error_disable = NULL,
        .error_get = port_base_error_get,
    },
    .set_state = port_set_state,
    .get_state = port_get_state,
    .toggle = port_toggle,
    .set_brightness = NULL,
    .get_brightness = NULL,
    .set_color = NULL,
    .blink_start = NULL,
    .blink_stop = NULL,
};

cfn_hal_error_code_t cfn_sal_led_construct(cfn_sal_led_t              *driver,
                                           const cfn_sal_led_config_t *config,
                                           const cfn_sal_phy_t        *phy,
                                           void                       *dependency,
                                           cfn_sal_led_callback_t      callback,
                                           void                       *user_arg)
{
    if ((driver == NULL) || (phy == NULL))
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_led_populate(driver, 0, dependency, &API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_led_destruct(cfn_sal_led_t *driver)
{
    if (driver == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_led_populate(driver, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
