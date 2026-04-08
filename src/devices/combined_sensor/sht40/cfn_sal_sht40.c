/**
 * @file cfn_sal_sht40.c
 * @brief SHT40 combined sensor (Temperature and Humidity) implementation.
 */

#include "cfn_sal_sht40.h"
#include "cfn_hal_i2c.h"

/* -------------------------------------------------------------------------- */
/* Private Constants                                                          */
/* -------------------------------------------------------------------------- */

#define SHT40_CMD_MEAS_HIGH_PREC 0xFD
#define SHT40_CMD_SOFT_RESET     0x94

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
            if (crc & 0x80)
            {
                crc = (uint8_t) ((crc << 1) ^ 0x31);
            }
            else
            {
                crc <<= 1;
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

    if (sht->combined_state.init_ref_count == 0)
    {
        cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) sht->combined_state.phy->instance;
        if (!dev->i2c)
        {
            return CFN_HAL_ERROR_BAD_PARAM;
        }

        cfn_hal_error_code_t err = cfn_hal_i2c_init(dev->i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        sht->combined_state.hw_initialized = true;
        sht->last_read_timestamp_ms        = 0;
    }

    sht->combined_state.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t sht40_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_sht40_t *sht = get_sht40_from_base(base);
    if (!sht)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (sht->combined_state.init_ref_count > 0)
    {
        sht->combined_state.init_ref_count--;

        if (sht->combined_state.init_ref_count == 0)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) sht->combined_state.phy->instance;
            cfn_hal_i2c_deinit(dev->i2c);
            sht->combined_state.hw_initialized = false;
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

    if (use_caching && sht->combined_state.hw_initialized && (now - sht->last_read_timestamp_ms < 10))
    {
        return CFN_HAL_ERROR_OK;
    }

    cfn_hal_i2c_device_t *dev     = (cfn_hal_i2c_device_t *) sht->combined_state.phy->instance;

    uint8_t                   cmd = SHT40_CMD_MEAS_HIGH_PREC;
    uint8_t                   buffer[6];
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = &cmd,
                                      .nbr_of_tx_bytes = 1,
                                      .rx_payload      = buffer,
                                      .nbr_of_rx_bytes = 6 };

    cfn_hal_error_code_t err      = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
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
    uint16_t t_raw           = (uint16_t) ((buffer[0] << 8) | buffer[1]);
    uint16_t rh_raw          = (uint16_t) ((buffer[3] << 8) | buffer[4]);

    sht->cached_temp_celsius = -45.0f + (175.0f * (float) t_raw) / 65535.0f;
    sht->cached_hum_rh       = -6.0f + (125.0f * (float) rh_raw) / 65535.0f;

    /* Clamp humidity to [0, 100] per datasheet */
    if (sht->cached_hum_rh < 0.0f)
    {
        sht->cached_hum_rh = 0.0f;
    }
    if (sht->cached_hum_rh > 100.0f)
    {
        sht->cached_hum_rh = 100.0f;
    }

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
        *temp_out = (celsius * 9.0f / 5.0f) + 32.0f;
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
        *raw_out = (int32_t) (sht->cached_temp_celsius * 100.0f);
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
        *raw_out = (int32_t) (sht->cached_hum_rh * 100.0f);
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

    sensor->combined_state.phy            = phy;
    sensor->combined_state.init_ref_count = 0;
    sensor->combined_state.hw_initialized = false;
    sensor->last_read_timestamp_ms        = 0;

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, &TEMP_API, phy, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, &HUM_API, phy, NULL, NULL, NULL);

    /* Inject time source as a dependency for caching */
    sensor->temp.base.dependency = time_source;
    sensor->hum.base.dependency  = time_source;

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_sht40_destruct(const cfn_sal_sht40_t *sensor)
{
    CFN_HAL_UNUSED(sensor);
    return CFN_HAL_ERROR_OK;
}
