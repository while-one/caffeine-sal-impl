#include "devices/cfn_sal_accel.h"
#include "cfn_sal_accel_lis2dh12.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_util.h"
#include <string.h>

#define LIS2DH12_I2C_ADDR   0x18UL
#define LIS2DH12_REG_CTRL1  0x20UL
#define LIS2DH12_REG_DATA_X 0x28UL

#define LIS2DH12_I2C_TIMEOUT_MS    100
#define LIS2DH12_AUTO_INC_MASK     0x80UL
#define LIS2DH12_CTRL1_DEFAULT_CFG 0x57

static cfn_hal_error_code_t port_base_init(cfn_hal_driver_t *base)
{
    cfn_sal_accel_t *driver = (cfn_sal_accel_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c       = (cfn_hal_i2c_t *) driver->phy->instance;

    cfn_hal_error_code_t err = cfn_hal_i2c_init(i2c);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* Enable sensor: 100Hz, Normal mode, all axes enabled */
    uint8_t                       ctrl1 = LIS2DH12_CTRL1_DEFAULT_CFG;
    cfn_hal_i2c_mem_transaction_t xfr   = {
          .dev_addr = LIS2DH12_I2C_ADDR, .mem_addr = LIS2DH12_REG_CTRL1, .mem_addr_size = 1, .data = &ctrl1, .size = 1
    };
    return cfn_hal_i2c_mem_write(i2c, &xfr, LIS2DH12_I2C_TIMEOUT_MS);
}

static cfn_hal_error_code_t port_base_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_accel_t *driver = (cfn_sal_accel_t *) base;
    if (!driver || !driver->phy || !driver->phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_deinit(i2c);
}

static cfn_hal_error_code_t port_base_power_state_set(cfn_hal_driver_t *base, cfn_hal_power_state_t state)
{
    cfn_sal_accel_t *driver = (cfn_sal_accel_t *) base;
    cfn_hal_i2c_t   *i2c    = (cfn_hal_i2c_t *) driver->phy->instance;
    return cfn_hal_i2c_power_state_set(i2c, state);
}

static cfn_hal_error_code_t port_base_config_set(cfn_hal_driver_t *base, const void *config)
{
    cfn_sal_accel_t *driver = (cfn_sal_accel_t *) base;
    driver->config          = (const cfn_sal_accel_config_t *) config;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t
port_base_callback_register(cfn_hal_driver_t *base, cfn_hal_callback_t callback, void *user_arg)
{
    cfn_sal_accel_t *driver = (cfn_sal_accel_t *) base;
    driver->cb              = (cfn_sal_accel_callback_t) callback;
    driver->cb_user_arg     = user_arg;
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

static cfn_hal_error_code_t port_read_xyz_raw(cfn_sal_accel_t *driver, cfn_sal_accel_data_t *data_out)
{
    if (!driver || !driver->phy || !driver->phy->instance || !data_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) driver->phy->instance;

    uint8_t data[6];
    /* LIS2DH12 multi-byte read requires setting bit 7 of the address */
    cfn_hal_i2c_mem_transaction_t xfr = { .dev_addr      = LIS2DH12_I2C_ADDR,
                                          .mem_addr      = LIS2DH12_REG_DATA_X | LIS2DH12_AUTO_INC_MASK,
                                          .mem_addr_size = 1,
                                          .data          = data,
                                          .size          = 6 };

    cfn_hal_error_code_t err          = cfn_hal_i2c_mem_read(i2c, &xfr, LIS2DH12_I2C_TIMEOUT_MS);
    if (err == CFN_HAL_ERROR_OK)
    {
        data_out->x = cfn_util_bytes_to_int16_le(data[1], data[0]) / 16;
        data_out->y = cfn_util_bytes_to_int16_le(data[3], data[2]) / 16;
        data_out->z = cfn_util_bytes_to_int16_le(data[5], data[4]) / 16;
    }
    return err;
}

static cfn_hal_error_code_t port_read_xyz_mg(cfn_sal_accel_t *driver, cfn_sal_accel_data_t *data_out)
{
    cfn_sal_accel_data_t raw;
    cfn_hal_error_code_t err = port_read_xyz_raw(driver, &raw);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* Normal mode conversion @ 2g: 1mg/LSB (shifted) */
    data_out->x = raw.x;
    data_out->y = raw.y;
    data_out->z = raw.z;

    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_accel_api_t API = {
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
    .read_xyz_mg = port_read_xyz_mg,
    .read_xyz_raw = port_read_xyz_raw,
    .set_range = NULL,
    .get_range = NULL,
    .set_datarate = NULL,
    .get_datarate = NULL,
    .enable_interrupts = NULL,
};

cfn_hal_error_code_t cfn_sal_accel_lis2dh12_construct(cfn_sal_accel_t              *driver,
                                                      const cfn_sal_accel_config_t *config,
                                                      const cfn_sal_phy_t          *phy,
                                                      void                         *dependency,
                                                      cfn_sal_accel_callback_t      callback,
                                                      void                         *user_arg)
{
    if ((driver == NULL) || (phy == NULL))
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_accel_populate(driver, 0, dependency, &API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_accel_lis2dh12_destruct(cfn_sal_accel_t *driver)
{
    if (driver == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_accel_populate(driver, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
