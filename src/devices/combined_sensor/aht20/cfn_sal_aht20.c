/**
 * @file cfn_sal_aht20.c
 * @brief AHT20 combined sensor implementation.
 */

#include "cfn_sal_aht20.h"
#include "cfn_hal_i2c.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Constants & Definitions                                                    */
/* -------------------------------------------------------------------------- */

#define AHT20_CMD_INIT       0xBE
#define AHT20_CMD_TRIGGER    0xAC
#define AHT20_CMD_SOFT_RESET 0xBA

#define AHT20_STATUS_CALIBRATED 0x08
#define AHT20_STATUS_BUSY       0x80

#define AHT20_MEAS_DELAY_MS 80

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static uint8_t crc8_aht20(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static cfn_sal_aht20_t *get_aht20_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_aht20_t, temp.base);
    }
    else if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_aht20_t, hum.base);
    }
    return NULL;
}

static cfn_hal_error_code_t aht20_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_aht20_t *aht = get_aht20_from_base(base);
    if (!aht || !aht->combined_state.phy || !aht->combined_state.phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (aht->combined_state.init_ref_count == 0)
    {
        cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) aht->combined_state.phy->instance;
        if (!dev->i2c)
        {
            return CFN_HAL_ERROR_BAD_PARAM;
        }

        cfn_hal_error_code_t err = cfn_hal_i2c_init(dev->i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        /* Wait 40ms per datasheet */
        /* cfn_hal_delay(40); // Need time service */

        /* Check calibration */
        uint8_t                   status = 0;
        cfn_hal_i2c_transaction_t xfr = { .slave_address = dev->address, .rx_payload = &status, .nbr_of_rx_bytes = 1 };

        err                           = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        if (!(status & AHT20_STATUS_CALIBRATED))
        {
            uint8_t cmd[3]      = { AHT20_CMD_INIT, 0x08, 0x00 };
            xfr.tx_payload      = cmd;
            xfr.nbr_of_tx_bytes = 3;
            xfr.rx_payload      = NULL;
            xfr.nbr_of_rx_bytes = 0;
            err                 = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
            if (err != CFN_HAL_ERROR_OK)
            {
                return err;
            }
        }

        aht->combined_state.hw_initialized = true;
    }

    aht->combined_state.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t aht20_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_aht20_t *aht = get_aht20_from_base(base);
    if (!aht)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (aht->combined_state.init_ref_count > 0)
    {
        aht->combined_state.init_ref_count--;

        if (aht->combined_state.init_ref_count == 0)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) aht->combined_state.phy->instance;
            cfn_hal_i2c_deinit(dev->i2c);
            aht->combined_state.hw_initialized = false;
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

    if (use_caching && aht->combined_state.hw_initialized && (now - aht->last_read_timestamp_ms < 10))
    {
        return CFN_HAL_ERROR_OK;
    }

    cfn_hal_i2c_device_t *dev        = (cfn_hal_i2c_device_t *) aht->combined_state.phy->instance;

    uint8_t                   cmd[3] = { AHT20_CMD_TRIGGER, 0x33, 0x00 };
    uint8_t                   buffer[7];
    cfn_hal_i2c_transaction_t xfr = {
        .slave_address = dev->address, .tx_payload = cmd, .nbr_of_tx_bytes = 3, .rx_payload = NULL, .nbr_of_rx_bytes = 0
    };

    /* Trigger */
    cfn_hal_error_code_t err = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* Wait 80ms */
    /* cfn_hal_delay(80); */

    /* Read result */
    xfr.tx_payload      = NULL;
    xfr.nbr_of_tx_bytes = 0;
    xfr.rx_payload      = buffer;
    xfr.nbr_of_rx_bytes = 7;

    err                 = cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    /* CRC check */
    if (crc8_aht20(&buffer[0], 6) != buffer[6])
    {
        return CFN_HAL_ERROR_DATA_CRC;
    }

    /* Unpack 20-bit data */
    uint32_t raw_hum            = ((uint32_t) buffer[1] << 12) | ((uint32_t) buffer[2] << 4) | (buffer[3] >> 4);
    uint32_t raw_temp           = ((uint32_t) (buffer[3] & 0x0F) << 16) | ((uint32_t) buffer[4] << 8) | buffer[5];

    aht->cached_hum_rh          = ((float) raw_hum * 100.0f) / 1048576.0f;
    aht->cached_temp_celsius    = (((float) raw_temp * 200.0f) / 1048576.0f) - 50.0f;
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
        *temp_out = (celsius * 9.0f / 5.0f) + 32.0f;
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
        /* AHT20 raw is 20 bits */
        /* Note: aht20_perform_read already unpacked it. We'd need to store it to return it here. */
        /* Re-calculating raw from cached for now as interface expects it. */
        /* raw_temp = (T + 50) * 2^20 / 200 */
        *raw_out = (int32_t) ((aht->cached_temp_celsius + 50.0f) * 1048576.0f / 200.0f);
    }
    return err;
}

static cfn_hal_error_code_t aht20_temp_soft_reset(cfn_sal_temp_sensor_t *driver)
{
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_aht20_t      *aht     = CFN_HAL_CONTAINER_OF(driver, cfn_sal_aht20_t, temp);
    cfn_hal_i2c_device_t *dev     = (cfn_hal_i2c_device_t *) aht->combined_state.phy->instance;

    uint8_t                   cmd = AHT20_CMD_SOFT_RESET;
    cfn_hal_i2c_transaction_t xfr = { .slave_address = dev->address, .tx_payload = &cmd, .nbr_of_tx_bytes = 1 };
    return cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
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
        *raw_out = (int32_t) (aht->cached_hum_rh * 1048576.0f / 100.0f);
    }
    return err;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base = {
        .init = aht20_shared_init,
        .deinit = aht20_shared_deinit,
    },
    .read_celsius = aht20_temp_read_celsius,
    .read_fahrenheit = aht20_temp_read_fahrenheit,
    .read_raw = aht20_temp_read_raw,
    .soft_reset = aht20_temp_soft_reset,
};

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base = {
        .init = aht20_shared_init,
        .deinit = aht20_shared_deinit,
    },
    .read_relative_humidity = aht20_hum_read_rh,
    .read_raw = aht20_hum_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t cfn_sal_aht20_construct(cfn_sal_aht20_t *sensor, const cfn_sal_phy_t *phy)
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

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_aht20_destruct(const cfn_sal_aht20_t *sensor)
{
    CFN_HAL_UNUSED(sensor);
    return CFN_HAL_ERROR_OK;
}
ource as a dependency for caching */
    sensor->temp.base.dependency = time_source;
sensor->hum.base.dependency      = time_source;

return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_aht20_destruct(const cfn_sal_aht20_t *sensor)
{
    CFN_HAL_UNUSED(sensor);
    return CFN_HAL_ERROR_OK;
}
