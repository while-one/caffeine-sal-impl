#include "devices/cfn_svc_button.h"
#include "cfn_hal_gpio.h"

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base) { CFN_HAL_UNUSED(base); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base) { CFN_HAL_UNUSED(base); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(state); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config) {
    cfn_svc_button_t *driver = (cfn_svc_button_t *) base;
    driver->config = (const cfn_svc_button_config_t *) config;
    return CFN_HAL_ERROR_OK;
}
static cfn_hal_error_code_t port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(callback); CFN_HAL_UNUSED(user_arg); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_enable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_disable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask) { CFN_HAL_UNUSED(base); if(event_mask)*event_mask=0; return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_enable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_disable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask) { CFN_HAL_UNUSED(base); if(error_mask)*error_mask=0; return CFN_HAL_ERROR_OK; }

static cfn_hal_error_code_t port_get_state(cfn_svc_button_t *driver, cfn_svc_button_state_t *state_out) {
    if (!driver || !driver->phy || !driver->phy->instance || !state_out) return CFN_HAL_ERROR_BAD_PARAM;

    cfn_hal_gpio_pin_handle_t *pin = (cfn_hal_gpio_pin_handle_t *) driver->phy->instance;
    cfn_hal_gpio_state_t gpio_state;
    cfn_hal_error_code_t err = cfn_hal_gpio_pin_read(pin->port, pin->pin, &gpio_state);

    if (err != CFN_HAL_ERROR_OK) return err;

    bool logical_pressed = false;
    if (driver->config && driver->config->active_low)
    {
        logical_pressed = (gpio_state == CFN_HAL_GPIO_STATE_LOW);
    }
    else
    {
        logical_pressed = (gpio_state == CFN_HAL_GPIO_STATE_HIGH);
    }

    *state_out = logical_pressed ? CFN_SVC_BUTTON_STATE_PRESSED : CFN_SVC_BUTTON_STATE_RELEASED;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_set_debounce_time(cfn_svc_button_t *driver, uint32_t ms) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(ms);
    /* Debounce configuration requires context state management, typically done via config struct mutation */
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_is_pressed(cfn_svc_button_t *driver, bool *pressed_out) {
    if (!pressed_out) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_svc_button_state_t state;
    cfn_hal_error_code_t err = port_get_state(driver, &state);
    if (err == CFN_HAL_ERROR_OK) {
        *pressed_out = (state == CFN_SVC_BUTTON_STATE_PRESSED);
    }
    return err;
}

static cfn_hal_error_code_t port_is_released(cfn_svc_button_t *driver, bool *released_out) {
    if (!released_out) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_svc_button_state_t state;
    cfn_hal_error_code_t err = port_get_state(driver, &state);
    if (err == CFN_HAL_ERROR_OK) {
        *released_out = (state == CFN_SVC_BUTTON_STATE_RELEASED);
    }
    return err;
}

static const cfn_svc_button_api_t API = {
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
    .get_state = port_get_state,
    .set_debounce_time = port_set_debounce_time,
    .is_pressed = port_is_pressed,
    .is_released = port_is_released,
};

cfn_hal_error_code_t cfn_svc_button_construct(cfn_svc_button_t *driver, const cfn_svc_button_config_t *config, const cfn_svc_button_phy_t *phy)
{
    if ((driver == NULL) || (phy == NULL)) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = &API;
    driver->base.type = CFN_SVC_TYPE_BUTTON;
    driver->base.status = CFN_HAL_DRIVER_STATUS_CONSTRUCTED;
    driver->config = config;
    driver->phy = phy;
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_svc_button_destruct(cfn_svc_button_t *driver)
{
    if (driver == NULL) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = NULL;
    driver->base.type = CFN_SVC_TYPE_BUTTON;
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    driver->config = NULL;
    driver->phy = NULL;
    return CFN_HAL_ERROR_OK;
}
