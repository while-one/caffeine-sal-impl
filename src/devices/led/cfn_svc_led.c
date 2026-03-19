#include "devices/cfn_svc_led.h"
#include "cfn_hal_gpio.h"

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base)
{
    cfn_svc_led_t *driver = (cfn_svc_led_t *) base;
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;

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
    cfn_svc_led_t *driver = (cfn_svc_led_t *) base;
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;

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

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(state); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(config); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(callback); CFN_HAL_UNUSED(user_arg); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_enable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_disable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask) { CFN_HAL_UNUSED(base); if(event_mask)*event_mask=0; return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_enable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_disable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask) { CFN_HAL_UNUSED(base); if(error_mask)*error_mask=0; return CFN_HAL_ERROR_OK; }

static cfn_hal_error_code_t port_set_state(cfn_svc_led_t *driver, cfn_svc_led_state_t state) {
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    cfn_hal_gpio_state_t gpio_state;

    if (state == CFN_SVC_LED_STATE_ON)
    {
        gpio_state = (driver->config && driver->config->active_low) ? CFN_HAL_GPIO_STATE_LOW : CFN_HAL_GPIO_STATE_HIGH;
    }
    else
    {
        gpio_state = (driver->config && driver->config->active_low) ? CFN_HAL_GPIO_STATE_HIGH : CFN_HAL_GPIO_STATE_LOW;
    }

    return cfn_hal_gpio_pin_write(pin->port, pin->pin, gpio_state);
}

static cfn_hal_error_code_t port_get_state(cfn_svc_led_t *driver, cfn_svc_led_state_t *state_out) {
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    cfn_hal_gpio_state_t gpio_state;
    cfn_hal_error_code_t err = cfn_hal_gpio_pin_read(pin->port, pin->pin, &gpio_state);

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

    *state_out = logical_on ? CFN_SVC_LED_STATE_ON : CFN_SVC_LED_STATE_OFF;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_toggle(cfn_svc_led_t *driver) {
    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    return cfn_hal_gpio_pin_toggle(pin->port, pin->pin);
}

static cfn_hal_error_code_t port_set_brightness(cfn_svc_led_t *driver, uint8_t percent) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(percent);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_get_brightness(cfn_svc_led_t *driver, uint8_t *percent_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(percent_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_set_color(cfn_svc_led_t *driver, cfn_svc_led_color_t color) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(color);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_blink_start(cfn_svc_led_t *driver, uint32_t period_ms) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(period_ms);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_blink_stop(cfn_svc_led_t *driver) {
    CFN_HAL_UNUSED(driver);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static const cfn_svc_led_api_t API = {
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
    .set_state = port_set_state,
    .get_state = port_get_state,
    .toggle = port_toggle,
    .set_brightness = port_set_brightness,
    .get_brightness = port_get_brightness,
    .set_color = port_set_color,
    .blink_start = port_blink_start,
    .blink_stop = port_blink_stop,
};

cfn_hal_error_code_t cfn_svc_led_construct(cfn_svc_led_t *driver, const cfn_svc_led_config_t *config, const cfn_svc_led_phy_t *phy)
{
    if ((driver == NULL) || (phy == NULL)) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = &API;
    driver->base.type = CFN_SVC_TYPE_LED;
    driver->base.status = CFN_HAL_DRIVER_STATUS_CONSTRUCTED;
    driver->config = config;
    driver->phy = phy;
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_svc_led_destruct(cfn_svc_led_t *driver)
{
    if (driver == NULL) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = NULL;
    driver->base.type = CFN_SVC_TYPE_LED;
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    driver->config = NULL;
    driver->phy = NULL;
    return CFN_HAL_ERROR_OK;
}
