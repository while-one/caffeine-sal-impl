#include "devices/cfn_svc_accel.h"
#include "cfn_hal_i2c.h"
#include <string.h>

#define BMA530_I2C_ADDR         0x18
#define BMA530_REG_DATA_X       0x02
#define BMA530_REG_DATA_Y       0x04
#define BMA530_REG_DATA_Z       0x06

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base) {
    cfn_svc_accel_t *driver = (cfn_svc_accel_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_init(i2c);
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base) {
    cfn_svc_accel_t *driver = (cfn_svc_accel_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_deinit(i2c);
}

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state) {
    cfn_svc_accel_t *driver = (cfn_svc_accel_t *) base;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_power_state_set(i2c, state);
}

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config) {
    cfn_svc_accel_t *driver = (cfn_svc_accel_t *) base;
    driver->config = (const cfn_svc_accel_config_t *) config;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg) {
    cfn_svc_accel_t *driver = (cfn_svc_accel_t *) base;
    driver->cb = (cfn_svc_accel_callback_t) callback;
    driver->cb_user_arg = user_arg;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_base_event_enable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_disable(cfn_hal_driver_t *base, uint32_t event_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(event_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_event_get(cfn_hal_driver_t *base, uint32_t *event_mask) { CFN_HAL_UNUSED(base); if(event_mask)*event_mask=0; return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_enable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_disable(cfn_hal_driver_t *base, uint32_t error_mask) { CFN_HAL_UNUSED(base); CFN_HAL_UNUSED(error_mask); return CFN_HAL_ERROR_OK; }
static cfn_hal_error_code_t port_base_error_get(cfn_hal_driver_t *base, uint32_t *error_mask) { CFN_HAL_UNUSED(base); if(error_mask)*error_mask=0; return CFN_HAL_ERROR_OK; }

static cfn_hal_error_code_t port_read_xyz_raw(cfn_svc_accel_t *driver, cfn_svc_accel_data_t *data_out) {
    if (!driver || !driver->phy || !driver->phy->instance || !data_out) return CFN_HAL_ERROR_BAD_PARAM;
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;

    uint8_t data[6];
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = BMA530_I2C_ADDR,
        .mem_addr = BMA530_REG_DATA_X,
        .mem_addr_size = 1,
        .data = data,
        .size = 6
    };

    cfn_hal_error_code_t err = cfn_hal_i2c_mem_read(i2c, &xfr, 100);
    if (err == CFN_HAL_ERROR_OK) {
        data_out->x = (int16_t)((data[1] << 8) | data[0]);
        data_out->y = (int16_t)((data[3] << 8) | data[2]);
        data_out->z = (int16_t)((data[5] << 8) | data[4]);
    }
    return err;
}

static cfn_hal_error_code_t port_read_xyz_mg(cfn_svc_accel_t *driver, cfn_svc_accel_data_t *data_out) {
    cfn_svc_accel_data_t raw;
    cfn_hal_error_code_t err = port_read_xyz_raw(driver, &raw);
    if (err != CFN_HAL_ERROR_OK) return err;

    /* Conversion logic based on range */
    float sensitivity = 1.0f; // placeholder
    data_out->x = (int32_t)((float)raw.x * sensitivity);
    data_out->y = (int32_t)((float)raw.y * sensitivity);
    data_out->z = (int32_t)((float)raw.z * sensitivity);

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_set_range(cfn_svc_accel_t *driver, cfn_svc_accel_range_t range) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(range);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_get_range(cfn_svc_accel_t *driver, cfn_svc_accel_range_t *range_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(range_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_set_datarate(cfn_svc_accel_t *driver, uint32_t hz) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(hz);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_get_datarate(cfn_svc_accel_t *driver, uint32_t *hz_out) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(hz_out);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t port_enable_interrupts(cfn_svc_accel_t *driver, uint32_t event_mask) {
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(event_mask);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static const cfn_svc_accel_api_t API = {
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
    .read_xyz_mg = port_read_xyz_mg,
    .read_xyz_raw = port_read_xyz_raw,
    .set_range = port_set_range,
    .get_range = port_get_range,
    .set_datarate = port_set_datarate,
    .get_datarate = port_get_datarate,
    .enable_interrupts = port_enable_interrupts,
};

cfn_hal_error_code_t cfn_svc_accel_bma530_construct(cfn_svc_accel_t *driver, const cfn_svc_accel_config_t *config, const cfn_svc_accel_phy_t *phy)
{
    if ((driver == NULL) || (phy == NULL)) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = &API;
    driver->base.type = CFN_SVC_TYPE_ACCEL;
    driver->base.status = CFN_HAL_DRIVER_STATUS_CONSTRUCTED;
    driver->config = config;
    driver->phy = phy;
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_svc_accel_bma530_destruct(cfn_svc_accel_t *driver)
{
    if (driver == NULL) return CFN_HAL_ERROR_BAD_PARAM;
    driver->api = NULL;
    driver->base.type = CFN_SVC_TYPE_ACCEL;
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    driver->config = NULL;
    driver->phy = NULL;
    return CFN_HAL_ERROR_OK;
}
