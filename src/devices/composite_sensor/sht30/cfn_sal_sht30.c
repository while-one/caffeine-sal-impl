/**
 * @file cfn_sal_sht30.c
 * @brief SHT30 composite sensor implementation.
 */

#include "cfn_sal_sht30.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_util.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Constants & Definitions                                                    */
/* -------------------------------------------------------------------------- */

#define SHT30_CMD_MEAS_HIGH_REP  0x2400UL
#define SHT30_CMD_SOFT_RESET     0x30A2UL
#define SHT30_CMD_HEATER_ENABLE  0x306DUL
#define SHT30_CMD_HEATER_DISABLE 0x3066UL

#define SHT30_MEAS_DELAY_MS  15
#define SHT30_I2C_TIMEOUT_MS 100

#define SHT30_ADC_MAX_VAL_F32 65535.0F
#define SHT30_TEMP_SCALE_F32  175.0F
#define SHT30_TEMP_OFFSET_F32 45.0F
#define SHT30_HUM_SCALE_F32   100.0F

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static uint8_t crc8_sht30(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++)
        {
            if (crc & 0x80UL)
            {
                crc = (uint8_t) (((uint32_t) crc << 1U) ^ 0x31U);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }
    return crc;
}

static cfn_sal_sht30_t *get_sht30_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_sht30_t, temp);
    }

    if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_sht30_t, hum);
    }
    return NULL;
}

static cfn_hal_error_code_t sht30_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_sht30_t *sht = get_sht30_from_base(base);
    if (!sht || !sht->shared.phy || !sht->shared.phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (sht->shared.init_ref_count == 0)
    {
        cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;
        if (!dev->i2c)
        {
            return CFN_HAL_ERROR_BAD_PARAM;
        }

        cfn_hal_error_code_t err = cfn_hal_i2c_init(dev->i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        sht->shared.hw_initialized = true;
    }

    sht->shared.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t sht30_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_sht30_t *sht = get_sht30_from_base(base);
    if (!sht)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (sht->shared.init_ref_count > 0)
    {
        sht->shared.init_ref_count--;

        if (sht->shared.init_ref_count == 0)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;
            cfn_hal_i2c_deinit(dev->i2c);
            sht->shared.hw_initialized = false;
        }
    }

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t sht30_perform_read(cfn_sal_sht30_t *sht)
{
    cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;

    uint8_t cmd[2] = { cfn_util_get_msb16(SHT30_CMD_MEAS_HIGH_REP), cfn_util_get_lsb16(SHT30_CMD_MEAS_HIGH_REP) };
    uint8_t buffer[6];
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = cmd,
                                      .nbr_of_tx_bytes = 2,
                                      .rx_payload      = buffer,
                                      .nbr_of_rx_bytes = 6 };

    cfn_hal_error_code_t err      = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, SHT30_I2C_TIMEOUT_MS);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    if (crc8_sht30(&buffer[0], 2) != buffer[2] || crc8_sht30(&buffer[3], 2) != buffer[5])
    {
        return CFN_HAL_ERROR_DATA_CRC;
    }

    uint16_t t_raw           = (uint16_t) cfn_util_bytes_to_int16_le(buffer[0], buffer[1]);
    uint16_t rh_raw          = (uint16_t) cfn_util_bytes_to_int16_le(buffer[3], buffer[4]);

    sht->cached_temp_celsius = -SHT30_TEMP_OFFSET_F32 + (SHT30_TEMP_SCALE_F32 * (float) t_raw) / SHT30_ADC_MAX_VAL_F32;
    sht->cached_hum_rh       = (SHT30_HUM_SCALE_F32 * (float) rh_raw) / SHT30_ADC_MAX_VAL_F32;

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Temperature Implementation                                                 */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t sht30_temp_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht30_t     *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht30_t, temp);
    cfn_hal_error_code_t err = sht30_perform_read(sht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *temp_out = sht->cached_temp_celsius;
    }
    return err;
}

static cfn_hal_error_code_t sht30_temp_read_fahrenheit(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    float                celsius;
    cfn_hal_error_code_t err = sht30_temp_read_celsius(driver, &celsius);
    if (err == CFN_HAL_ERROR_OK && temp_out)
    {
        *temp_out = (celsius * 9.0F / 5.0F) + 32.0F;
    }
    return err;
}

static cfn_hal_error_code_t sht30_temp_read_raw(cfn_sal_temp_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht30_t      *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht30_t, temp);
    cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;

    /* Standard Big-Endian command split for SHT30 sensor */
    uint8_t cmd[2]            = { cfn_util_get_msb16((uint16_t) SHT30_CMD_MEAS_HIGH_REP),
                                  cfn_util_get_lsb16((uint16_t) SHT30_CMD_MEAS_HIGH_REP) };

    uint8_t                   buffer[2];
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = cmd,
                                      .nbr_of_tx_bytes = 2,
                                      .rx_payload      = buffer,
                                      .nbr_of_rx_bytes = 2 };

    cfn_hal_error_code_t err      = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, SHT30_I2C_TIMEOUT_MS);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) cfn_util_bytes_to_int16_le(buffer[0], buffer[1]);
    }
    return err;
}

static cfn_hal_error_code_t sht30_temp_soft_reset(cfn_sal_temp_sensor_t *driver)
{
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht30_t      *sht        = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht30_t, temp);
    cfn_hal_i2c_device_t *dev        = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;

    uint8_t                   cmd[2] = { cfn_util_get_msb16((uint16_t) SHT30_CMD_SOFT_RESET),
                                         cfn_util_get_lsb16((uint16_t) SHT30_CMD_SOFT_RESET) };
    cfn_hal_i2c_transaction_t xfr    = { .slave_address = dev->address, .tx_payload = cmd, .nbr_of_tx_bytes = 2 };
    return cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, SHT30_I2C_TIMEOUT_MS);
}

static cfn_hal_error_code_t
sht30_temp_enable_heater(cfn_sal_temp_sensor_t *driver, uint32_t power_mw, uint32_t duration_ms)
{
    CFN_HAL_UNUSED(duration_ms);
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht30_t      *sht     = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht30_t, temp);
    cfn_hal_i2c_device_t *dev     = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;

    uint16_t cmd_raw              = (power_mw > 0) ? SHT30_CMD_HEATER_ENABLE : SHT30_CMD_HEATER_DISABLE;
    uint8_t  cmd[2]               = { cfn_util_get_msb16((uint16_t) cmd_raw), cfn_util_get_lsb16((uint16_t) cmd_raw) };

    cfn_hal_i2c_transaction_t xfr = { .slave_address = dev->address, .tx_payload = cmd, .nbr_of_tx_bytes = 2 };
    return cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, SHT30_I2C_TIMEOUT_MS);
}

/* -------------------------------------------------------------------------- */
/* Humidity Implementation                                                    */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t sht30_hum_read_rh(cfn_sal_hum_sensor_t *driver, float *hum_out)
{
    if (!driver || !hum_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht30_t     *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht30_t, hum);
    cfn_hal_error_code_t err = sht30_perform_read(sht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *hum_out = sht->cached_hum_rh;
    }
    return err;
}

static cfn_hal_error_code_t sht30_hum_read_raw(cfn_sal_hum_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht30_t      *sht        = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht30_t, hum);
    cfn_hal_i2c_device_t *dev        = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;

    uint8_t                   cmd[2] = { cfn_util_get_msb16((uint16_t) SHT30_CMD_MEAS_HIGH_REP),
                                         cfn_util_get_lsb16((uint16_t) SHT30_CMD_MEAS_HIGH_REP) };
    uint8_t                   buffer[6];
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = cmd,
                                      .nbr_of_tx_bytes = 2,
                                      .rx_payload      = buffer,
                                      .nbr_of_rx_bytes = 6 };

    cfn_hal_error_code_t err      = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, SHT30_I2C_TIMEOUT_MS);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) cfn_util_bytes_to_int16_le(buffer[3], buffer[4]);
    }
    return err;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base = {
        .init = sht30_shared_init,
        .deinit = sht30_shared_deinit,
    },
    .read_celsius = sht30_temp_read_celsius,
    .read_fahrenheit = sht30_temp_read_fahrenheit,
    .read_raw = sht30_temp_read_raw,
    .soft_reset = sht30_temp_soft_reset,
    .enable_heater = sht30_temp_enable_heater,
};

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base = {
        .init = sht30_shared_init,
        .deinit = sht30_shared_deinit,
    },
    .read_relative_humidity = sht30_hum_read_rh,
    .read_raw = sht30_hum_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t cfn_sal_sht30_construct(cfn_sal_sht30_t *sensor, const cfn_sal_phy_t *phy, void *dependency)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_composite_init(&sensor->shared, phy);

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, dependency, &TEMP_API, phy, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, dependency, &HUM_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_sht30_destruct(cfn_sal_sht30_t *sensor)
{
    if (sensor == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}
