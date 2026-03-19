#include "devices/cfn_svc_temp_sensor.h"
#include "cfn_hal_i2c.h"
#include <math.h>

#define TMP117_I2C_ADDR         0x48
#define TMP117_REG_TEMP_RESULT  0x00
#define TMP117_RESOLUTION       0.0078125f

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base) {
    cfn_svc_temp_sensor_t *driver = (cfn_svc_temp_sensor_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_init(i2c);
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base) {
    cfn_svc_temp_sensor_t *driver = (cfn_svc_temp_sensor_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_deinit(i2c);
}

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state) {
    cfn_svc_temp_sensor_t *driver = (cfn_svc_temp_sensor_t *) base;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_power_state_set(i2c, state);
}

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config) {
    cfn_svc_temp_sensor_t *driver = (cfn_svc_temp_sensor_t *) base;
    driver->config = (const cfn_svc_temp_config_t *) config;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg) {
    cfn_svc_temp_sensor_t *driver = (cfn_svc_temp_sensor_t *) base;
    driver->cb = (cfn_svc_temp_callback_t) callback;
    driver->cb_user_arg = user_arg;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_event_enable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_disable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask) { CFN_HAL_UNUSED(base); if(event_mask)*event_mask=0; return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_enable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_disable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask) { CFN_HAL_UNUSED(base); if(error_mask)*error_mask=0; return CFN_HAL_ERROR_OK; }

static cfn_hal_error_code_t port_read_celsius(cfn_svc_temp_sensor_t *driver, float *temp_out) {
    if (!driver || !driver->phy || !driver->phy->instance || !temp_out) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;

    uint8_t data[2];
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = TMP117_I2C_ADDR,
        .mem_addr = TMP117_REG_TEMP_RESULT,
        .mem_addr_size = 1,
        .data = data,
        .size = 2
    };

    cfn_hal_error_code_t err = cfn_hal_i2c_mem_read(i2c, &xfr, 100);
    if (err != CFN_HAL_ERROR_OK) return err;

    int16_t raw_temp = (int16_t)((data[0] << 8) | data[1]);
    *temp_out = (float)raw_temp * TMP117_RESOLUTION;

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_read_fahrenheit(cfn_svc_temp_sensor_t *driver, float *temp_out) {
    float celsius;
    cfn_hal_error_code_t err = port_read_celsius(driver, &celsius);
    if (err == CFN_HAL_ERROR_OK) {
        *temp_out = (celsius * 9.0f / 5.0f) + 32.0f;
    }
    return err;
}

static cfn_hal_error_code_t port_read_raw(cfn_svc_temp_sensor_t *driver, int32_t *raw_out) {
    if (!driver || !driver->phy || !driver->phy->instance || !raw_out) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;

    uint8_t data[2];
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = TMP117_I2C_ADDR,
        .mem_addr = TMP117_REG_TEMP_RESULT,
        .mem_addr_size = 1,
        .data = data,
        .size = 2
    };

    cfn_hal_error_code_t err = cfn_hal_i2c_mem_read(i2c, &xfr, 100);
    if (err == CFN_HAL_ERROR_OK) {
        *raw_out = (int32_t)((data[0] << 8) | data[1]);
    }
    return err;
}

static cfn_hal_error_code_t port_set_resolution(cfn_svc_temp_sensor_t *driver, uint8_t bits) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(bits);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_get_resolution(cfn_svc_temp_sensor_t *driver, uint8_t *bits_out) {
    CFN_HAL_UNUSED(driver);
    if (bits_out) *bits_out = 16;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_set_high_threshold(cfn_svc_temp_sensor_t *driver, float celsius) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(celsius);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_set_low_threshold(cfn_svc_temp_sensor_t *driver, float celsius) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(celsius);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static const cfn_svc_temp_sensor_api_t API = {
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
    .read_celsius = port_read_celsius,
    .read_fahrenheit = port_read_fahrenheit,
    .read_raw = port_read_raw,
    .set_resolution = port_set_resolution,
    .get_resolution = port_get_resolution,
    .set_high_threshold = port_set_high_threshold,
    .set_low_threshold = port_set_low_threshold,
};

cfn_hal_error_code_t cfn_svc_temp_rmp117_construct(cfn_svc_temp_sensor_t *driver, const cfn_svc_temp_config_t *config, const cfn_svc_temp_phy_t *phy)
{
    if ((driver == NULL) || (phy == NULL)) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = &API;
    driver->base.type = CFN_SVC_TYPE_TEMP_SENSOR;
    driver->base.status = CFN_HAL_DRIVER_STATUS_CONSTRUCTED;
    driver->config = config;
    driver->phy = phy;
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_svc_temp_rmp117_destruct(cfn_svc_temp_sensor_t *driver)
{
    if (driver == NULL) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = NULL;
    driver->base.type = CFN_SVC_TYPE_TEMP_SENSOR;
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    driver->config = NULL;
    driver->phy = NULL;
    return CFN_HAL_ERROR_OK;
}
