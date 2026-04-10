/**
 * @file cfn_sal_aht20.c
 * @brief AHT20 composite sensor (Temperature and Humidity) implementation.
 */

#include "cfn_sal_aht20.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_util.h"

/* -------------------------------------------------------------------------- */
/* Private Constants                                                          */
/* -------------------------------------------------------------------------- */

#define AHT20_CMD_TRIGGER 0xAC
#define AHT20_CMD_RESET   0xBA

#define AHT20_I2C_TIMEOUT_MS  100
#define AHT20_ADC_MAX_VAL_F32 1048576.0F
#define AHT20_TEMP_SCALE_F32  200.0F
#define AHT20_TEMP_OFFSET_F32 50.0F
#define AHT20_HUM_SCALE_F32   100.0F

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static uint8_t crc8_aht20(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80UL)
            {
                crc = (uint8_t) ((uint32_t) (crc << 1UL) ^ 0x31UL);
            }
            else
            {
                crc <<= 1UL;
            }
        }
    }
    return crc;
}

static cfn_sal_aht20_t *get_aht20_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_aht20_t, temp);
    }
    if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_aht20_t, hum);
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Shared Lifecycle                                                           */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t aht20_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_aht20_t *aht = get_aht20_from_base(base);
    if (!aht)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (aht->shared.init_ref_count == 0)
    {
        cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) aht->shared.phy->instance;
        if (!dev->i2c)
        {
            return CFN_HAL_ERROR_BAD_PARAM;
        }

        cfn_hal_error_code_t err = cfn_hal_i2c_init(dev->i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        aht->shared.hw_initialized  = true;
        aht->last_read_timestamp_ms = 0;
    }

    aht->shared.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t aht20_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_aht20_t *aht = get_aht20_from_base(base);
    if (!aht)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (aht->shared.init_ref_count > 0)
    {
        aht->shared.init_ref_count--;

        if (aht->shared.init_ref_count == 0)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) aht->shared.phy->instance;
            cfn_hal_i2c_deinit(dev->i2c);
            aht->shared.hw_initialized = false;
        }
    }

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t aht20_perform_read(cfn_sal_aht20_t *aht)
{
    /* Check cache validity (10ms window) */
    uint64_t now         = 0;
    bool     use_caching = false;

    if (aht->temp.base.dependency != NULL)
    {
        cfn_sal_timekeeping_get_ms((cfn_sal_timekeeping_t *) aht->temp.base.dependency, &now);
        use_caching = true;
    }

    if (use_caching && aht->shared.hw_initialized && (now - aht->last_read_timestamp_ms < 10))
    {
        return CFN_HAL_ERROR_OK;
    }

    cfn_hal_i2c_device_t *dev        = (cfn_hal_i2c_device_t *) aht->shared.phy->instance;

    uint8_t                   cmd[3] = { AHT20_CMD_TRIGGER, 0x33, 0x00 };
    uint8_t                   buffer[7];
    cfn_hal_i2c_transaction_t xfr = {
        .slave_address = dev->address, .tx_payload = cmd, .nbr_of_tx_bytes = 3, .rx_payload = NULL, .nbr_of_rx_bytes = 0
    };

    /* Trigger */
    cfn_hal_error_code_t err = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, AHT20_I2C_TIMEOUT_MS);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* Read result */
    xfr.tx_payload      = NULL;
    xfr.nbr_of_tx_bytes = 0;
    xfr.rx_payload      = buffer;
    xfr.nbr_of_rx_bytes = 7;

    err                 = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, AHT20_I2C_TIMEOUT_MS);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* CRC check */
    if (crc8_aht20(&buffer[0], 6) != buffer[6])
    {
        return CFN_HAL_ERROR_DATA_CRC;
    }

    /* Unpack 20-bit data using utility to avoid multiple explicit uint32_t casts */
    uint32_t raw_hum   = ((uint32_t) cfn_util_bytes_to_int32_be(0, buffer[1], buffer[2], buffer[3])) >> 4UL;
    uint32_t raw_temp  = (uint32_t) cfn_util_bytes_to_int32_be(0, buffer[3] & 0x0FUL, buffer[4], buffer[5]);

    aht->cached_hum_rh = ((float) raw_hum * AHT20_HUM_SCALE_F32) / AHT20_ADC_MAX_VAL_F32;
    aht->cached_temp_celsius =
        (((float) raw_temp * AHT20_TEMP_SCALE_F32) / AHT20_ADC_MAX_VAL_F32) - AHT20_TEMP_OFFSET_F32;
    aht->last_read_timestamp_ms = now;

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Temperature Implementation                                                 */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t aht20_temp_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_aht20_t     *aht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_aht20_t, temp);
    cfn_hal_error_code_t err = aht20_perform_read(aht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *temp_out = aht->cached_temp_celsius;
    }
    return err;
}

static cfn_hal_error_code_t aht20_temp_read_fahrenheit(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    float                celsius;
    cfn_hal_error_code_t err = aht20_temp_read_celsius(driver, &celsius);
    if (err == CFN_HAL_ERROR_OK && temp_out)
    {
        *temp_out = (celsius * 9.0F / 5.0F) + 32.0F;
    }
    return err;
}

static cfn_hal_error_code_t aht20_temp_read_raw(cfn_sal_temp_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_aht20_t     *aht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_aht20_t, temp);
    cfn_hal_error_code_t err = aht20_perform_read(aht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) (aht->cached_temp_celsius * 100.0F);
    }
    return err;
}

/* -------------------------------------------------------------------------- */
/* Humidity Implementation                                                    */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t aht20_hum_read_rh(cfn_sal_hum_sensor_t *driver, float *hum_out)
{
    if (!driver || !hum_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_aht20_t     *aht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_aht20_t, hum);
    cfn_hal_error_code_t err = aht20_perform_read(aht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *hum_out = aht->cached_hum_rh;
    }
    return err;
}

static cfn_hal_error_code_t aht20_hum_read_raw(cfn_sal_hum_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_aht20_t     *aht = CFN_HAL_CONTAINER_OF(driver, cfn_sal_aht20_t, hum);
    cfn_hal_error_code_t err = aht20_perform_read(aht);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (int32_t) (aht->cached_hum_rh * 100.0F);
    }
    return err;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base            = { .init = aht20_shared_init, .deinit = aht20_shared_deinit },
    .read_celsius    = aht20_temp_read_celsius,
    .read_fahrenheit = aht20_temp_read_fahrenheit,
    .read_raw        = aht20_temp_read_raw,
};

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base                   = { .init = aht20_shared_init, .deinit = aht20_shared_deinit },
    .read_relative_humidity = aht20_hum_read_rh,
    .read_raw               = aht20_hum_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */
cfn_hal_error_code_t cfn_sal_aht20_construct(cfn_sal_aht20_t       *sensor,
                                             const cfn_sal_phy_t   *phy,
                                             void                  *dependency,
                                             cfn_sal_timekeeping_t *time_source)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_composite_init(&sensor->shared, phy);
    sensor->last_read_timestamp_ms = 0;

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, dependency, &TEMP_API, phy, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, dependency, &HUM_API, phy, NULL, NULL, NULL);

    /* Inject time source as a dependency for caching */
    sensor->temp.base.dependency = time_source;
    sensor->hum.base.dependency  = time_source;

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_aht20_destruct(cfn_sal_aht20_t *sensor)
{
    if (!sensor)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_temp_sensor_populate(&sensor->temp, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    return CFN_HAL_ERROR_OK;
}
