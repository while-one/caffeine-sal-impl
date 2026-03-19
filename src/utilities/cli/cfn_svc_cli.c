#include "utilities/cfn_svc_cli.h"
#include "cfn_hal_uart.h"

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base) {
    cfn_svc_cli_t *driver = (cfn_svc_cli_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_uart_t *uart = (cfn_hal_uart_t *) driver->phy->instance;
    return cfn_hal_uart_init(uart);
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base) {
    cfn_svc_cli_t *driver = (cfn_svc_cli_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_uart_t *uart = (cfn_hal_uart_t *) driver->phy->instance;
    return cfn_hal_uart_deinit(uart);
}

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state) {
    cfn_svc_cli_t *driver = (cfn_svc_cli_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_uart_t *uart = (cfn_hal_uart_t *) driver->phy->instance;
    return cfn_hal_uart_power_state_set(uart, state);
}

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config) {
    cfn_svc_cli_t *driver = (cfn_svc_cli_t *) base;
    driver->config = (const cfn_svc_cli_config_t *) config;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg) {
    cfn_svc_cli_t *driver = (cfn_svc_cli_t *) base;
    driver->cb = (cfn_svc_cli_callback_t) callback;
    driver->cb_user_arg = user_arg;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_event_enable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_disable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask) { CFN_HAL_UNUSED(base); if(event_mask)*event_mask=0; return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_enable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_disable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask) { CFN_HAL_UNUSED(base); if(error_mask)*error_mask=0; return CFN_HAL_ERROR_OK; }

static cfn_hal_error_code_t port_feed_char(cfn_svc_cli_t *driver, char c) {
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_uart_t *uart = (cfn_hal_uart_t *) driver->phy->instance;

    if (driver->config && driver->config->echo_enabled) {
        cfn_hal_uart_tx_polling(uart, (const uint8_t *)&c, 1, 100);
    }
    
    /* Internal buffer management would go here */
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_print_string(cfn_svc_cli_t *driver, const char *str) {
    if (!driver || !driver->phy || !driver->phy->instance || !str) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_uart_t *uart = (cfn_hal_uart_t *) driver->phy->instance;
    
    size_t len = 0;
    while(str[len] != '\0') len++;
    
    return cfn_hal_uart_tx_polling(uart, (const uint8_t *)str, len, 1000);
}

static cfn_hal_error_code_t port_print_line(cfn_svc_cli_t *driver, const char *str) {
    cfn_hal_error_code_t err = port_print_string(driver, str);
    if (err == CFN_HAL_ERROR_OK) {
        err = port_print_string(driver, "\r\n");
    }
    return err;
}

static cfn_hal_error_code_t port_register_command(cfn_svc_cli_t *driver, const cfn_svc_cli_cmd_t *cmd) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(cmd);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_unregister_command(cfn_svc_cli_t *driver, const char *name) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(name);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_set_prompt(cfn_svc_cli_t *driver, const char *prompt) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(prompt);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_clear_screen(cfn_svc_cli_t *driver) {
    return port_print_string(driver, "\033[2J\033[H");
}

static const cfn_svc_cli_api_t API = {
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
    .feed_char = port_feed_char,
    .print_string = port_print_string,
    .print_line = port_print_line,
    .register_command = port_register_command,
    .unregister_command = port_unregister_command,
    .set_prompt = port_set_prompt,
    .clear_screen = port_clear_screen,
};

cfn_hal_error_code_t cfn_svc_cli_construct(cfn_svc_cli_t *driver, const cfn_svc_cli_config_t *config, const cfn_svc_cli_phy_t *phy)
{
    if ((driver == NULL) || (phy == NULL)) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = &API;
    driver->base.type = CFN_SVC_TYPE_CLI;
    driver->base.status = CFN_HAL_DRIVER_STATUS_CONSTRUCTED;
    driver->config = config;
    driver->phy = phy;
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_svc_cli_destruct(cfn_svc_cli_t *driver)
{
    if (driver == NULL) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = NULL;
    driver->base.type = CFN_SVC_TYPE_CLI;
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    driver->config = NULL;
    driver->phy = NULL;
    return CFN_HAL_ERROR_OK;
}
