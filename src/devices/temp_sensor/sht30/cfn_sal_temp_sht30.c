#include "devices/cfn_sal_temp_sensor.h"
#include "cfn_sal_temp_sht30_port.h"
#include "cfn_hal_i2c.h"
#include <math.h>

#define SHT30_I2C_ADDR          0x44
#define SHT30_CMD_MEAS_HIGH_REP 0x2400

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base)
{
    cfn_sal_temp_sensor_t *driver = (cfn_sal_temp_sensor_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_init(i2c);
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_temp_sensor_t *driver = (cfn_sal_temp_sensor_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_deinit(i2c);
}

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state)
{
    cfn_sal_temp_sensor_t *driver = (cfn_sal_temp_sensor_t *) base;
    cfn_hal_i2c_t         *i2c    = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_power_state_set(i2c, state);
}

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config)
{
    cfn_sal_temp_sensor_t *driver = (cfn_sal_temp_sensor_t *) base;
    driver->config                = (const cfn_sal_temp_config_t *) config;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t
port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg)
{
    cfn_sal_temp_sensor_t *driver = (cfn_sal_temp_sensor_t *) base;
    driver->cb                    = (cfn_sal_temp_callback_t) callback;
    driver->cb_user_arg           = user_arg;
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

static cfn_hal_error_code_t port_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !driver->phy || !driver->phy->instance || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c                = (cfn_hal_i2c_t *) driver->phy->instance;

    uint8_t                   cmd[2]  = { (SHT30_CMD_MEAS_HIGH_REP >> 8) & 0xFF, SHT30_CMD_MEAS_HIGH_REP & 0xFF };
    uint8_t                   data[6] = { 0 };
    cfn_hal_i2c_transaction_t xfr     = { .slave_address   = SHT30_I2C_ADDR,
                                          .tx_payload      = cmd,
                                          .nbr_of_tx_bytes = 2,
                                          .rx_payload      = data,
                                          .nbr_of_rx_bytes = 6 };

    cfn_hal_error_code_t err          = cfn_hal_i2c_xfr_polling(i2c, &xfr, 100);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    uint16_t raw_temp = (data[0] << 8) | data[1];
    *temp_out         = -45.0f + 175.0f * ((float) raw_temp / 65535.0f);

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t port_read_fahrenheit(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    float                celsius;
    cfn_hal_error_code_t err = port_read_celsius(driver, &celsius);
    if (err == CFN_HAL_ERROR_OK)
    {
        *temp_out = (celsius * 9.0f / 5.0f) + 32.0f;
    }
    return err;
}

static cfn_hal_error_code_t port_read_raw(cfn_sal_temp_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !driver->phy || !driver->phy->instance || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c               = (cfn_hal_i2c_t *) driver->phy->instance;

    uint8_t                   cmd[2] = { (SHT30_CMD_MEAS_HIGH_REP >> 8) & 0xFF, SHT30_CMD_MEAS_HIGH_REP & 0xFF };
    uint8_t                   data[2];
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = SHT30_I2C_ADDR,
                                      .tx_payload      = cmd,
                                      .nbr_of_tx_bytes = 2,
                                      .rx_payload      = data,
                                      .nbr_of_rx_bytes = 2 };

    cfn_hal_error_code_t err      = cfn_hal_i2c_xfr_polling(i2c, &xfr, 100);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) ((data[0] << 8) | data[1]);
    }
    return err;
}

static cfn_hal_error_code_t port_get_resolution(cfn_sal_temp_sensor_t *driver, uint8_t *bits_out)
{
    CFN_HAL_UNUSED(driver);
    if (bits_out)
    {
        *bits_out = 16;
    }
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_temp_sensor_api_t API = {
    .base = {
        .init = port_base_init,
        .deinit = port_base_deinit,
        .power_state_set = port_base_power_state_set,
        .config_set = port_base_config_set,
        .callback_register = port_base_callback_register,
        .event_enable = NULL,
        .event_disable = NULL,
        .event_get = port_base_event_get,
        .error_enable = NULL,
        .error_disable = NULL,
        .error_get = port_base_error_get,
    },
    .read_celsius = port_read_celsius,
    .read_fahrenheit = port_read_fahrenheit,
    .read_raw = port_read_raw,
    .set_resolution = NULL,
    .get_resolution = port_get_resolution,
    .set_high_threshold = NULL,
    .set_low_threshold = NULL,
};

cfn_hal_error_code_t cfn_sal_temp_sht30_construct(cfn_sal_temp_sensor_t       *driver,
                                                  const cfn_sal_temp_config_t *config,
                                                  const cfn_sal_phy_t         *phy,
                                                  cfn_sal_temp_callback_t      callback,
                                                  void                        *user_arg)
{
    if ((driver == NULL) || (phy == NULL))
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_temp_sensor_populate(driver, 0, &API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_temp_sht30_destruct(cfn_sal_temp_sensor_t *driver)
{
    if (driver == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_temp_sensor_populate(driver, 0, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
