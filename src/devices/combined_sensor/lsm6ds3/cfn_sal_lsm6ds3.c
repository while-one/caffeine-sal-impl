/**
 * @file cfn_sal_lsm6ds3.c
 * @brief LSM6DS3 combined sensor implementation.
 */

#include "cfn_sal_lsm6ds3.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_spi.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Constants & Definitions                                                    */
/* -------------------------------------------------------------------------- */

#define LSM6DS3_REG_WHO_AM_I       0x0F
#define LSM6DS3_REG_CTRL1_XL       0x10
#define LSM6DS3_REG_CTRL2_G        0x11
#define LSM6DS3_REG_CTRL3_C        0x12
#define LSM6DS3_REG_OUTX_L_G       0x22
#define LSM6DS3_REG_OUTX_L_XL      0x28
#define LSM6DS3_REG_STEP_COUNTER_L 0x4B

#define LSM6DS3_WHO_AM_I_VAL 0x69

#define LSM6DS3_CTRL3_SW_RESET CFN_HAL_BIT(0)
#define LSM6DS3_CTRL3_BDU      CFN_HAL_BIT(6)
#define LSM6DS3_CTRL3_IF_INC   CFN_HAL_BIT(2)

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static cfn_sal_lsm6ds3_t *get_lsm6ds3_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_ACCEL)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_lsm6ds3_t, accel.base);
    }
    else if (base->type == CFN_SAL_TYPE_GYRO_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_lsm6ds3_t, gyro.base);
    }
    return NULL;
}

static cfn_hal_error_code_t lsm6ds3_read_regs(const cfn_sal_phy_t *phy, uint8_t reg, uint8_t *data, size_t size)
{
    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
    {
        cfn_hal_i2c_device_t         *dev = (cfn_hal_i2c_device_t *) phy->instance;
        cfn_hal_i2c_mem_transaction_t xfr = {
            .dev_addr = dev->address, .mem_addr = reg, .mem_addr_size = 1, .data = data, .size = size
        };
        return cfn_hal_i2c_mem_read(dev->i2c, &xfr, 100);
    }
    else if (phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
    {
        cfn_hal_spi_device_t *dev     = (cfn_hal_spi_device_t *) phy->instance;
        /* SPI read: Bit 0 is 1 for Read */
        uint8_t addr                  = reg | 0x80;

        /* Note: This assumes physical implementation handles CS via the device struct */
        /* For now, generic SPI transfer */
        cfn_hal_spi_transaction_t xfr = { .tx_payload      = &addr,
                                          .nbr_of_tx_bytes = 1,
                                          .rx_payload      = data,
                                          .nbr_of_rx_bytes = size,
                                          .cs_pin          = dev->cs_pin };
        /* Assuming cfn_hal_spi_xfr_polling exists and handles the transaction */
        return cfn_hal_spi_xfr_polling(dev->spi, &xfr, 100);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t lsm6ds3_write_reg(const cfn_sal_phy_t *phy, uint8_t reg, uint8_t val)
{
    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
    {
        cfn_hal_i2c_device_t         *dev = (cfn_hal_i2c_device_t *) phy->instance;
        cfn_hal_i2c_mem_transaction_t xfr = {
            .dev_addr = dev->address, .mem_addr = reg, .mem_addr_size = 1, .data = &val, .size = 1
        };
        return cfn_hal_i2c_mem_write(dev->i2c, &xfr, 100);
    }
    else if (phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
    {
        cfn_hal_spi_device_t *dev       = (cfn_hal_spi_device_t *) phy->instance;
        uint8_t               buffer[2] = { reg & 0x7F, val }; /* Bit 0 is 0 for Write */

        cfn_hal_spi_transaction_t xfr   = {
              .tx_payload = buffer, .nbr_of_tx_bytes = 2, .rx_payload = NULL, .nbr_of_rx_bytes = 0, .cs_pin = dev->cs_pin
        };
        return cfn_hal_spi_xfr_polling(dev->spi, &xfr, 100);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t lsm6ds3_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_lsm6ds3_t *lsm = get_lsm6ds3_from_base(base);
    if (!lsm || !lsm->combined_state.phy || !lsm->combined_state.phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (lsm->combined_state.init_ref_count == 0)
    {
        /* Initialize the physical bus */
        if (lsm->combined_state.phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) lsm->combined_state.phy->instance;
            cfn_hal_i2c_init(dev->i2c);
        }
        else if (lsm->combined_state.phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
        {
            cfn_hal_spi_device_t *dev = (cfn_hal_spi_device_t *) lsm->combined_state.phy->instance;
            cfn_hal_spi_init(dev->spi);
        }

        /* Identity Check */
        uint8_t              id  = 0;
        cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->combined_state.phy, LSM6DS3_REG_WHO_AM_I, &id, 1);
        if (err != CFN_HAL_ERROR_OK || id != LSM6DS3_WHO_AM_I_VAL)
        {
            return CFN_HAL_ERROR_FAIL;
        }

        /* Software Reset */
        err = lsm6ds3_write_reg(lsm->combined_state.phy, LSM6DS3_REG_CTRL3_C, LSM6DS3_CTRL3_SW_RESET);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        /* Wait for reset (simple delay for now) */
        /* cfn_hal_delay(10); */

        /* Basic Config: BDU and Auto Increment */
        err = lsm6ds3_write_reg(lsm->combined_state.phy, LSM6DS3_REG_CTRL3_C, LSM6DS3_CTRL3_BDU | LSM6DS3_CTRL3_IF_INC);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        lsm->combined_state.hw_initialized = true;
    }

    lsm->combined_state.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t lsm6ds3_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_lsm6ds3_t *lsm = get_lsm6ds3_from_base(base);
    if (!lsm)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (lsm->combined_state.init_ref_count > 0)
    {
        lsm->combined_state.init_ref_count--;
        if (lsm->combined_state.init_ref_count == 0)
        {
            /* Power down everything */
            (void) lsm6ds3_write_reg(lsm->combined_state.phy, LSM6DS3_REG_CTRL1_XL, 0x00);
            (void) lsm6ds3_write_reg(lsm->combined_state.phy, LSM6DS3_REG_CTRL2_G, 0x00);

            lsm->combined_state.hw_initialized = false;
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

    uint8_t              buffer[6];
    cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->combined_state.phy, LSM6DS3_REG_OUTX_L_XL, buffer, 6);
    if (err == CFN_HAL_ERROR_OK)
    {
        data_out->x = (int16_t) ((buffer[1] << 8) | buffer[0]);
        data_out->y = (int16_t) ((buffer[3] << 8) | buffer[2]);
        data_out->z = (int16_t) ((buffer[5] << 8) | buffer[4]);
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_accel_read_mg(cfn_sal_accel_t *driver, cfn_sal_accel_data_t *data_out)
{
    cfn_sal_accel_data_t raw;
    cfn_hal_error_code_t err = lsm6ds3_accel_read_raw(driver, &raw);
    if (err == CFN_HAL_ERROR_OK)
    {
        cfn_sal_lsm6ds3_t *lsm         = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);
        float              sensitivity = 0.061f; /* Default for 2g */
        switch (lsm->current_accel_range)
        {
            case CFN_SAL_ACCEL_RANGE_4G:
                sensitivity = 0.122f;
                break;
            case CFN_SAL_ACCEL_RANGE_8G:
                sensitivity = 0.244f;
                break;
            case CFN_SAL_ACCEL_RANGE_16G:
                sensitivity = 0.488f;
                break;
            default:
                break;
        }
        data_out->x = (int32_t) ((float) raw.x * sensitivity);
        data_out->y = (int32_t) ((float) raw.y * sensitivity);
        data_out->z = (int32_t) ((float) raw.z * sensitivity);
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_accel_set_range(cfn_sal_accel_t *driver, cfn_sal_accel_range_t range)
{
    cfn_sal_lsm6ds3_t   *lsm   = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);
    uint8_t              ctrl1 = 0;
    cfn_hal_error_code_t err   = lsm6ds3_read_regs(lsm->combined_state.phy, LSM6DS3_REG_CTRL1_XL, &ctrl1, 1);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    ctrl1 &= ~0x0C; /* Clear FS bits */
    switch (range)
    {
        case CFN_SAL_ACCEL_RANGE_2G:
            break;
        case CFN_SAL_ACCEL_RANGE_16G:
            ctrl1 |= 0x04;
            break;
        case CFN_SAL_ACCEL_RANGE_4G:
            ctrl1 |= 0x08;
            break;
        case CFN_SAL_ACCEL_RANGE_8G:
            ctrl1 |= 0x0C;
            break;
    }
    err = lsm6ds3_write_reg(lsm->combined_state.phy, LSM6DS3_REG_CTRL1_XL, ctrl1);
    if (err == CFN_HAL_ERROR_OK)
    {
        lsm->current_accel_range = range;
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_accel_read_steps(cfn_sal_accel_t *driver, uint32_t *steps)
{
    if (!driver || !steps)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_lsm6ds3_t   *lsm = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, accel);
    uint8_t              buffer[2];
    cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->combined_state.phy, LSM6DS3_REG_STEP_COUNTER_L, buffer, 2);
    if (err == CFN_HAL_ERROR_OK)
    {
        *steps = (uint32_t) ((buffer[1] << 8) | buffer[0]);
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

    uint8_t              buffer[6];
    cfn_hal_error_code_t err = lsm6ds3_read_regs(lsm->combined_state.phy, LSM6DS3_REG_OUTX_L_G, buffer, 6);
    if (err == CFN_HAL_ERROR_OK)
    {
        data_out->x = (int16_t) ((buffer[1] << 8) | buffer[0]);
        data_out->y = (int16_t) ((buffer[3] << 8) | buffer[2]);
        data_out->z = (int16_t) ((buffer[5] << 8) | buffer[4]);
    }
    return err;
}

static cfn_hal_error_code_t lsm6ds3_gyro_read_mdps(cfn_sal_gyro_sensor_t *driver, cfn_sal_gyro_data_t *data_out)
{
    cfn_sal_gyro_data_t  raw;
    cfn_hal_error_code_t err = lsm6ds3_gyro_read_raw(driver, &raw);
    if (err == CFN_HAL_ERROR_OK)
    {
        cfn_sal_lsm6ds3_t *lsm         = CFN_HAL_CONTAINER_OF(driver, cfn_sal_lsm6ds3_t, gyro);
        float              sensitivity = 8.75f; /* Default for 250 dps */
        switch (lsm->current_gyro_range)
        {
            case CFN_SAL_GYRO_RANGE_125DPS:
                sensitivity = 4.375f;
                break;
            case CFN_SAL_GYRO_RANGE_500DPS:
                sensitivity = 17.50f;
                break;
            case CFN_SAL_GYRO_RANGE_1000DPS:
                sensitivity = 35.0f;
                break;
            case CFN_SAL_GYRO_RANGE_2000DPS:
                sensitivity = 70.0f;
                break;
            default:
                break;
        }
        data_out->x = (int32_t) ((float) raw.x * sensitivity);
        data_out->y = (int32_t) ((float) raw.y * sensitivity);
        data_out->z = (int32_t) ((float) raw.z * sensitivity);
    }
    return err;
}

static const cfn_sal_accel_api_t ACCEL_API = {
    .base = {
        .init = lsm6ds3_shared_init,
        .deinit = lsm6ds3_shared_deinit,
    },
    .read_xyz_raw = lsm6ds3_accel_read_raw,
    .read_xyz_mg = lsm6ds3_accel_read_mg,
    .set_range = lsm6ds3_accel_set_range,
    .read_step_counter = lsm6ds3_accel_read_steps,
    /* ... other methods mocked or unimplemented ... */
};

static const cfn_sal_gyro_sensor_api_t GYRO_API = {
    .base = {
        .init = lsm6ds3_shared_init,
        .deinit = lsm6ds3_shared_deinit,
    },
    .read_xyz_raw = lsm6ds3_gyro_read_raw,
    .read_xyz_mdps = lsm6ds3_gyro_read_mdps,
    /* ... other methods ... */
};

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t cfn_sal_lsm6ds3_construct(cfn_sal_lsm6ds3_t *sensor, const cfn_sal_phy_t *phy)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    sensor->combined_state.phy            = phy;
    sensor->combined_state.init_ref_count = 0;
    sensor->combined_state.hw_initialized = false;

    sensor->current_accel_range           = CFN_SAL_ACCEL_RANGE_2G;
    sensor->current_gyro_range            = CFN_SAL_GYRO_RANGE_250DPS;

    cfn_sal_accel_populate(&sensor->accel, 0, &ACCEL_API, phy, NULL, NULL, NULL);
    cfn_sal_gyro_sensor_populate(&sensor->gyro, 0, &GYRO_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_lsm6ds3_destruct(const cfn_sal_lsm6ds3_t *sensor)
{
    CFN_HAL_UNUSED(sensor);
    return CFN_HAL_ERROR_OK;
}
