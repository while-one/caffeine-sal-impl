/**
 * @file cfn_sal_lsm6ds3.c
 * @brief LSM6DS3 composite sensor (Accelerometer and Gyroscope) implementation.
 */

#include "cfn_sal_lsm6ds3.h"
#include "cfn_hal_util.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Private Constants                                                          */
/* -------------------------------------------------------------------------- */

#define LSM6DS3_REG_WHO_AM_I   0x0F
#define LSM6DS3_REG_CTRL1_XL   0x10
#define LSM6DS3_REG_CTRL2_G    0x11
#define LSM6DS3_REG_CTRL3_C    0x12
#define LSM6DS3_REG_OUTX_L_G   0x22
#define LSM6DS3_REG_OUTX_L_XL  0x28
#define LSM6DS3_REG_STEP_CNT_L 0x4B

#define LSM6DS3_WHO_AM_I_VAL   0x69
#define LSM6DS3_I2C_TIMEOUT_MS 100

#define LSM6DS3_ACCEL_SENSITIVITY_2G_MG  0.061F
#define LSM6DS3_ACCEL_SENSITIVITY_4G_MG  0.122F
#define LSM6DS3_ACCEL_SENSITIVITY_8G_MG  0.244F
#define LSM6DS3_ACCEL_SENSITIVITY_16G_MG 0.488F

#define LSM6DS3_GYRO_SENSITIVITY_250DPS_MDPS  8.75F
#define LSM6DS3_GYRO_SENSITIVITY_500DPS_MDPS  17.50F
#define LSM6DS3_GYRO_SENSITIVITY_1000DPS_MDPS 35.0F
#define LSM6DS3_GYRO_SENSITIVITY_2000DPS_MDPS 70.0F

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t lsm6ds3_write_reg(const cfn_sal_phy_t *phy, uint8_t reg, uint8_t val)
{
    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
    {
        cfn_hal_i2c_device_t         *dev = (cfn_hal_i2c_device_t *) phy->instance;
        cfn_hal_i2c_mem_transaction_t xfr = {
            .dev_addr = dev->address, .mem_addr = reg, .mem_addr_size = 1, .data = &val, .size = 1
        };
        return cfn_hal_i2c_mem_write(dev->i2c, &xfr, LSM6DS3_I2C_TIMEOUT_MS);
    }

    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
    {
        cfn_hal_spi_device_t     *dev   = (cfn_hal_spi_device_t *) phy->instance;
        uint8_t                   tx[2] = { reg & 0x7FUL, val };
        cfn_hal_spi_transaction_t xfr = { .tx_payload = tx, .rx_payload = NULL, .nbr_of_bytes = 2, .cs = dev->cs_pin };
        return cfn_hal_spi_xfr_polling(dev->spi, &xfr, LSM6DS3_I2C_TIMEOUT_MS);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t lsm6ds3_read_regs(const cfn_sal_phy_t *phy, uint8_t reg, uint8_t *data, size_t len)
{
    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
    {
        cfn_hal_i2c_device_t         *dev = (cfn_hal_i2c_device_t *) phy->instance;
        cfn_hal_i2c_mem_transaction_t xfr = {
            .dev_addr = dev->address, .mem_addr = reg, .mem_addr_size = 1, .data = data, .size = len
        };
        return cfn_hal_i2c_mem_read(dev->i2c, &xfr, LSM6DS3_I2C_TIMEOUT_MS);
    }

    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
    {
        cfn_hal_spi_device_t     *dev = (cfn_hal_spi_device_t *) phy->instance;
        uint8_t                   cmd = reg | 0x80UL;
        cfn_hal_spi_transaction_t xfr = {
            .tx_payload = &cmd, .rx_payload = data, .nbr_of_bytes = len, .cs = dev->cs_pin
        };
        return cfn_hal_spi_xfr_polling(dev->spi, &xfr, LSM6DS3_I2C_TIMEOUT_MS);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_sal_lsm6ds3_t *get_lsm6ds3_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_ACCEL)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_lsm6ds3_t, accel);
    }
    if (base->type == CFN_SAL_TYPE_GYRO_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_lsm6ds3_t, gyro);
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Shared Lifecycle                                                           */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t lsm6ds3_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_lsm6ds3_t *lsm = get_lsm6ds3_from_base(base);
    if (!lsm)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (lsm->shared.init_ref_count == 0)
    {
        if (lsm->shared.phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) lsm->shared.phy->instance;
            cfn_hal_i2c_init(dev->i2c);
        }
        else if (lsm->shared.phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
        {
            cfn_hal_spi_device_t *dev = (cfn_hal_spi_device_t *) lsm->shared.phy->instance;
            cfn_hal_spi_init(dev->spi);
        }

        /* WHO_AM_I */
        uint8_t id = 0;
        if (lsm6ds3_read_regs(lsm->shared.phy, LSM6DS3_REG_WHO_AM_I, &id, 1) != CFN_HAL_ERROR_OK ||
            id != LSM6DS3_WHO_AM_I_VAL)
        {
            return CFN_HAL_ERROR_FAIL;
        }

        /* Basic Config: Enable block data update, auto-increment */
        (void) lsm6ds3_write_reg(lsm->shared.phy, LSM6DS3_REG_CTRL3_C, 0x44);

        lsm->shared.hw_initialized = true;
    }
    lsm->shared.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t lsm6ds3_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_lsm6ds3_t *lsm = get_lsm6ds3_from_base(base);
    if (!lsm)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    if (lsm->shared.init_ref_count > 0)
    {
        lsm->shared.init_ref_count--;
        if (lsm->shared.init_ref_count == 0)
        {
            /* Power down both */
            (void) lsm6ds3_write_reg(lsm->shared.phy, LSM6DS3_REG_CTRL1_XL, 0x00);
            (void) lsm6ds3_write_reg(lsm->shared.phy, LSM6DS3_REG_CTRL2_G, 0x00);
            lsm->shared.hw_initialized = false;
        }
    }
    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Accelerometer Implementation                                               */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t lsm6ds3_accel_read_raw(cfn_sal_accel_t *driver, cfn_sal_accel_data_t *data_out)
{
    if (!driver || !data_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_lsm6ds3_t *lsm = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);

    /* Check cache validity (5ms window for high-rate IMU) */
    uint64_t now           = 0;
    bool     use_caching   = false;

    if (lsm->accel.base.dependency != NULL)
    {
        cfn_sal_timekeeping_get_ms((cfn_sal_timekeeping_t *) lsm->accel.base.dependency, &now);
        use_caching = true;
    }

    if (use_caching && lsm->shared.hw_initialized && (now - lsm->last_read_timestamp_accel_ms < 5))
    {
        *data_out = lsm->cached_accel_raw;
        return CFN_HAL_ERROR_OK;
    }

    uint8_t              buffer[6];
    cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->shared.phy, LSM6DS3_REG_OUTX_L_XL, buffer, 6);
    if (err == CFN_HAL_ERROR_OK)
    {
        lsm->cached_accel_raw.x           = cfn_util_bytes_to_int16_le(buffer[1], buffer[0]);
        lsm->cached_accel_raw.y           = cfn_util_bytes_to_int16_le(buffer[3], buffer[2]);
        lsm->cached_accel_raw.z           = cfn_util_bytes_to_int16_le(buffer[5], buffer[4]);
        *data_out                         = lsm->cached_accel_raw;
        lsm->last_read_timestamp_accel_ms = now;
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_accel_read_mg(cfn_sal_accel_t *driver, cfn_sal_accel_data_t *data_out)
{
    cfn_sal_accel_data_t raw;
    cfn_hal_error_code_t err = lsm6ds3_accel_read_raw(driver, &raw);
    if (err == CFN_HAL_ERROR_OK)
    {
        const cfn_sal_lsm6ds3_t *lsm         = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);
        float                    sensitivity = LSM6DS3_ACCEL_SENSITIVITY_2G_MG;
        switch (lsm->current_accel_range)
        {
            case CFN_SAL_ACCEL_RANGE_4G:
                sensitivity = LSM6DS3_ACCEL_SENSITIVITY_4G_MG;
                break;
            case CFN_SAL_ACCEL_RANGE_8G:
                sensitivity = LSM6DS3_ACCEL_SENSITIVITY_8G_MG;
                break;
            case CFN_SAL_ACCEL_RANGE_16G:
                sensitivity = LSM6DS3_ACCEL_SENSITIVITY_16G_MG;
                break;
            default:
                break;
        }
        data_out->x = (int16_t) ((float) raw.x * sensitivity);
        data_out->y = (int16_t) ((float) raw.y * sensitivity);
        data_out->z = (int16_t) ((float) raw.z * sensitivity);
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_accel_set_range(cfn_sal_accel_t *driver, cfn_sal_accel_range_t range)
{
    cfn_sal_lsm6ds3_t   *lsm = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);
    uint8_t              ctrl1;
    cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->shared.phy, LSM6DS3_REG_CTRL1_XL, &ctrl1, 1);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    ctrl1 &= ~0x0CUL; /* Clear FS bits */
    switch (range)
    {
        case CFN_SAL_ACCEL_RANGE_4G:
            ctrl1 |= 0x08UL;
            break;
        case CFN_SAL_ACCEL_RANGE_8G:
            ctrl1 |= 0x0CUL;
            break;
        case CFN_SAL_ACCEL_RANGE_16G:
            ctrl1 |= 0x04UL;
            break;
        default:
            break;
    }

    err = lsm6ds3_write_reg(lsm->shared.phy, LSM6DS3_REG_CTRL1_XL, ctrl1);
    if (err == CFN_HAL_ERROR_OK)
    {
        lsm->current_accel_range = range;
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_accel_read_steps(cfn_sal_accel_t *driver, uint32_t *steps)
{
    const cfn_sal_lsm6ds3_t *lsm = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);
    uint8_t                  buffer[2];
    cfn_hal_error_code_t     err = lsm6ds3_read_regs(lsm->shared.phy, LSM6DS3_REG_STEP_CNT_L, buffer, 2);
    if (err == CFN_HAL_ERROR_OK)
    {
        *steps = (uint32_t) cfn_util_bytes_to_int16_le(buffer[1], buffer[0]);
    }
    return err;
}

/* -------------------------------------------------------------------------- */
/* Gyroscope Implementation                                                   */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t lsm6ds3_gyro_read_raw(cfn_sal_gyro_sensor_t *driver, cfn_sal_gyro_data_t *data_out)
{
    if (!driver || !data_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_lsm6ds3_t *lsm = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, gyro);

    /* Check cache validity (5ms window) */
    uint64_t now           = 0;
    bool     use_caching   = false;

    if (lsm->gyro.base.dependency != NULL)
    {
        cfn_sal_timekeeping_get_ms((cfn_sal_timekeeping_t *) lsm->gyro.base.dependency, &now);
        use_caching = true;
    }

    if (use_caching && lsm->shared.hw_initialized && (now - lsm->last_read_timestamp_gyro_ms < 5))
    {
        *data_out = lsm->cached_gyro_raw;
        return CFN_HAL_ERROR_OK;
    }

    uint8_t              buffer[6];
    cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->shared.phy, LSM6DS3_REG_OUTX_L_G, buffer, 6);
    if (err == CFN_HAL_ERROR_OK)
    {
        lsm->cached_gyro_raw.x           = cfn_util_bytes_to_int16_le(buffer[1], buffer[0]);
        lsm->cached_gyro_raw.y           = cfn_util_bytes_to_int16_le(buffer[3], buffer[2]);
        lsm->cached_gyro_raw.z           = cfn_util_bytes_to_int16_le(buffer[5], buffer[4]);
        *data_out                        = lsm->cached_gyro_raw;
        lsm->last_read_timestamp_gyro_ms = now;
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_gyro_read_mdps(cfn_sal_gyro_sensor_t *driver, cfn_sal_gyro_data_t *data_out)
{
    cfn_sal_gyro_data_t  raw;
    cfn_hal_error_code_t err = lsm6ds3_gyro_read_raw(driver, &raw);
    if (err == CFN_HAL_ERROR_OK)
    {
        const cfn_sal_lsm6ds3_t *lsm         = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, gyro);
        float                    sensitivity = LSM6DS3_GYRO_SENSITIVITY_250DPS_MDPS;
        switch (lsm->current_gyro_range)
        {
            case CFN_SAL_GYRO_RANGE_500DPS:
                sensitivity = LSM6DS3_GYRO_SENSITIVITY_500DPS_MDPS;
                break;
            case CFN_SAL_GYRO_RANGE_1000DPS:
                sensitivity = LSM6DS3_GYRO_SENSITIVITY_1000DPS_MDPS;
                break;
            case CFN_SAL_GYRO_RANGE_2000DPS:
                sensitivity = LSM6DS3_GYRO_SENSITIVITY_2000DPS_MDPS;
                break;
            default:
                break;
        }
        data_out->x = (int16_t) ((float) raw.x * sensitivity);
        data_out->y = (int16_t) ((float) raw.y * sensitivity);
        data_out->z = (int16_t) ((float) raw.z * sensitivity);
    }
    return err;
}

static const cfn_sal_accel_api_t ACCEL_API = {
    .base              = { .init = lsm6ds3_shared_init, .deinit = lsm6ds3_shared_deinit },
    .read_xyz_raw      = lsm6ds3_accel_read_raw,
    .read_xyz_mg       = lsm6ds3_accel_read_mg,
    .set_range         = lsm6ds3_accel_set_range,
    .read_step_counter = lsm6ds3_accel_read_steps,
};

static const cfn_sal_gyro_sensor_api_t GYRO_API = {
    .base          = { .init = lsm6ds3_shared_init, .deinit = lsm6ds3_shared_deinit },
    .read_xyz_raw  = lsm6ds3_gyro_read_raw,
    .read_xyz_mdps = lsm6ds3_gyro_read_mdps,
};

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */
cfn_hal_error_code_t
cfn_sal_lsm6ds3_construct(cfn_sal_lsm6ds3_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_composite_init(&sensor->shared, phy);
    sensor->last_read_timestamp_accel_ms = 0;
    sensor->last_read_timestamp_gyro_ms  = 0;

    sensor->current_accel_range          = CFN_SAL_ACCEL_RANGE_2G;
    sensor->current_gyro_range           = CFN_SAL_GYRO_RANGE_250DPS;

    cfn_sal_accel_populate(&sensor->accel, 0, (void *) time_source, &ACCEL_API, phy, NULL, NULL, NULL);
    cfn_sal_gyro_sensor_populate(&sensor->gyro, 0, (void *) time_source, &GYRO_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_lsm6ds3_destruct(cfn_sal_lsm6ds3_t *sensor)
{
    if (sensor == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_accel_populate(&sensor->accel, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    cfn_sal_gyro_sensor_populate(&sensor->gyro, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}
