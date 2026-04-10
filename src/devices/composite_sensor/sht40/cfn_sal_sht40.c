/**
 * @file cfn_sal_sht40.c
 * @brief SHT40 composite sensor (Temperature and Humidity) implementation.
 */

#include "cfn_sal_sht40.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_util.h"

/* -------------------------------------------------------------------------- */
/* Private Constants                                                          */
/* -------------------------------------------------------------------------- */

#define SHT40_CMD_MEAS_HIGH_PREC 0xFD
#define SHT40_CMD_SOFT_RESET     0x94

#define SHT40_I2C_TIMEOUT_MS 100

#define SHT40_ADC_MAX_VAL_F32 65535.0F
#define SHT40_TEMP_SCALE_F32  175.0F
#define SHT40_TEMP_OFFSET_F32 45.0F
#define SHT40_HUM_SCALE_F32   125.0F
#define SHT40_HUM_OFFSET_F32  6.0F

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80U)
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

static cfn_sal_sht40_t *get_sht40_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_sht40_t, temp);
    }
    if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_sht40_t, hum);
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Shared Lifecycle                                                           */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t sht40_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_sht40_t *sht = get_sht40_from_base(base);
    if (!sht)
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

        sht->shared.hw_initialized  = true;
        sht->last_read_timestamp_ms = 0;
    }

    sht->shared.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t sht40_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_sht40_t *sht = get_sht40_from_base(base);
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

static cfn_hal_error_code_t sht40_perform_read(cfn_sal_sht40_t *sht)
{
    /* Check cache validity (10ms window) */
    uint64_t now         = 0;
    bool     use_caching = false;

    if (sht->temp.base.dependency != NULL)
    {
        cfn_sal_timekeeping_get_ms((cfn_sal_timekeeping_t *) sht->temp.base.dependency, &now);
        use_caching = true;
    }

    if (use_caching && sht->shared.hw_initialized && (now - sht->last_read_timestamp_ms < 10))
    {
        return CFN_HAL_ERROR_OK;
    }

    cfn_hal_i2c_device_t *dev     = (cfn_hal_i2c_device_t *) sht->shared.phy->instance;

    uint8_t                   cmd = SHT40_CMD_MEAS_HIGH_PREC;
    uint8_t                   buffer[6];
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = &cmd,
                                      .nbr_of_tx_bytes = 1,
                                      .rx_payload      = buffer,
                                      .nbr_of_rx_bytes = 6 };

    cfn_hal_error_code_t err      = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, SHT40_I2C_TIMEOUT_MS);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* CRC8 checks */
    if (crc8(&buffer[0], 2) != buffer[2] || crc8(&buffer[3], 2) != buffer[5])
    {
        return CFN_HAL_ERROR_DATA_CRC;
    }

    /* Math conversions */
    uint16_t t_raw           = (uint16_t) cfn_util_bytes_to_int16_le(buffer[0], buffer[1]);
    uint16_t rh_raw          = (uint16_t) cfn_util_bytes_to_int16_le(buffer[3], buffer[4]);

    sht->cached_temp_celsius = -SHT40_TEMP_OFFSET_F32 + (SHT40_TEMP_SCALE_F32 * (float) t_raw) / SHT40_ADC_MAX_VAL_F32;
    sht->cached_hum_rh       = -SHT40_HUM_OFFSET_F32 + (SHT40_HUM_SCALE_F32 * (float) rh_raw) / SHT40_ADC_MAX_VAL_F32;

    /* Clamp humidity to [0, 100] per datasheet using utilities */
    sht->cached_hum_rh       = cfn_util_clamp_f32(sht->cached_hum_rh, 0.0F, 100.0F);

    sht->last_read_timestamp_ms = now;

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Temperature Implementation                                                 */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t sht40_temp_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht40_t     *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht40_t, temp);
    cfn_hal_error_code_t err = sht40_perform_read(sht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *temp_out = sht->cached_temp_celsius;
    }
    return err;
}

static cfn_hal_error_code_t sht40_temp_read_fahrenheit(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    float                celsius;
    cfn_hal_error_code_t err = sht40_temp_read_celsius(driver, &celsius);
    if (err == CFN_HAL_ERROR_OK && temp_out)
    {
        *temp_out = (celsius * 9.0F / 5.0F) + 32.0F;
    }
    return err;
}

static cfn_hal_error_code_t sht40_temp_read_raw(cfn_sal_temp_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht40_t     *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht40_t, temp);
    cfn_hal_error_code_t err = sht40_perform_read(sht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) (sht->cached_temp_celsius * 100.0F);
    }
    return err;
}

/* -------------------------------------------------------------------------- */
/* Humidity Implementation                                                    */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t sht40_hum_read_rh(cfn_sal_hum_sensor_t *driver, float *hum_out)
{
    if (!driver || !hum_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht40_t     *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht40_t, hum);
    cfn_hal_error_code_t err = sht40_perform_read(sht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *hum_out = sht->cached_hum_rh;
    }
    return err;
}

static cfn_hal_error_code_t sht40_hum_read_raw(cfn_sal_hum_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_sht40_t     *sht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_sht40_t, hum);
    cfn_hal_error_code_t err = sht40_perform_read(sht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) (sht->cached_hum_rh * 100.0F);
    }
    return err;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base              = { .init = sht40_shared_init, .deinit = sht40_shared_deinit },
    .read_celsius      = sht40_temp_read_celsius,
    .read_fahrenheit   = sht40_temp_read_fahrenheit,
    .read_raw          = sht40_temp_read_raw,
    .get_serial_number = NULL,
};

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base                   = { .init = sht40_shared_init, .deinit = sht40_shared_deinit },
    .read_relative_humidity = sht40_hum_read_rh,
    .read_raw               = sht40_hum_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */
cfn_hal_error_code_t
cfn_sal_sht40_construct(cfn_sal_sht40_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_composite_init(&sensor->shared, phy);
    sensor->last_read_timestamp_ms = 0;

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, (void *) time_source, &TEMP_API, phy, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, (void *) time_source, &HUM_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_sht40_destruct(cfn_sal_sht40_t *sensor)
{
    if (sensor == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}
