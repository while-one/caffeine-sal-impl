/**
 * @file cfn_sal_bme280.c
 * @brief BME280 combination sensor implementation using the composite struct pattern.
 */

#include "cfn_sal_bme280.h"
#include "cfn_hal_i2c.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Helper: Parent Retrieval                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Logic to navigate from any of the polymorphic interface base drivers
 * to the parent BME280 composite structure.
 */
static cfn_sal_bme280_t *get_bme280_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_bme280_t, temp.base);
    }
    else if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_bme280_t, hum.base);
    }
    else if (base->type == CFN_SAL_TYPE_PRESSURE_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_bme280_t, press.base);
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Shared Lifecycle Management                                                */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_bme280_t *bme = get_bme280_from_base(base);
    if (!bme || !bme->combined_state.phy || !bme->combined_state.phy->handle)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (bme->combined_state.init_ref_count == 0)
    {
        /* First interface being initialized - power up the hardware */
        cfn_hal_i2c_t       *i2c = (cfn_hal_i2c_t *) bme->combined_state.phy->handle;
        cfn_hal_error_code_t err = cfn_hal_i2c_init(i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        bme->combined_state.hw_initialized = true;
        bme->t_fine                        = 0; /* Initial calibration state */
    }

    bme->combined_state.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_bme280_t *bme = get_bme280_from_base(base);
    if (!bme)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (bme->combined_state.init_ref_count > 0)
    {
        bme->combined_state.init_ref_count--;

        if (bme->combined_state.init_ref_count == 0)
        {
            /* Last interface being closed - power down the hardware */
            cfn_hal_i2c_t *i2c = (cfn_hal_i2c_t *) bme->combined_state.phy->handle;
            cfn_hal_i2c_deinit(i2c);
            bme->combined_state.hw_initialized = false;
        }
    }

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Temperature Sensor Implementation                                          */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_temp_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_t *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, temp);
    if (!bme->combined_state.hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    /* Mock reading */
    *temp_out = 25.0f;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_temp_read_fahrenheit(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    float                celsius = 0.0f;
    cfn_hal_error_code_t err     = bme280_temp_read_celsius(driver, &celsius);
    if (err == CFN_HAL_ERROR_OK && temp_out)
    {
        *temp_out = (celsius * 9.0f / 5.0f) + 32.0f;
    }
    return err;
}

static cfn_hal_error_code_t bme280_temp_read_raw(cfn_sal_temp_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_t *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, temp);
    if (!bme->combined_state.hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    *raw_out = 0x1234; /* Mock raw data */
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base = {
        .init   = bme280_shared_init,
        .deinit = bme280_shared_deinit,
    },
    .read_celsius    = bme280_temp_read_celsius,
    .read_fahrenheit = bme280_temp_read_fahrenheit,
    .read_raw        = bme280_temp_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Humidity Sensor Implementation                                             */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_hum_read_percent(cfn_sal_hum_sensor_t *driver, float *hum_out)
{
    if (!driver || !hum_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_t *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, hum);
    if (!bme->combined_state.hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    /* Mock reading */
    *hum_out = 50.0f;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_hum_read_raw(cfn_sal_hum_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_t *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, hum);
    if (!bme->combined_state.hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    *raw_out = 0x5678; /* Mock raw data */
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base = {
        .init   = bme280_shared_init,
        .deinit = bme280_shared_deinit,
    },
    .read_relative_humidity = bme280_hum_read_percent,
    .read_raw               = bme280_hum_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Pressure Sensor Implementation                                             */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_press_read_hpa(cfn_sal_pressure_sensor_t *driver, float *hpa_out)
{
    if (!driver || !hpa_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_t *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, press);
    if (!bme->combined_state.hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    /* Mock reading */
    *hpa_out = 1013.25f;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_press_read_raw(cfn_sal_pressure_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_t *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, press);
    if (!bme->combined_state.hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    *raw_out = 0x9ABC; /* Mock raw data */
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_pressure_sensor_api_t PRESS_API = {
    .base = {
        .init   = bme280_shared_init,
        .deinit = bme280_shared_deinit,
    },
    .read_hpa = bme280_press_read_hpa,
    .read_raw = bme280_press_read_raw,
};

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t cfn_sal_bme280_construct(cfn_sal_bme280_t *sensor, const cfn_sal_phy_t *phy)
{
    if (!sensor || !phy)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    /* Initialize shared state */
    sensor->combined_state.phy            = phy;
    sensor->combined_state.init_ref_count = 0;
    sensor->combined_state.hw_initialized = false;
    sensor->t_fine                        = 0;

    /* Populate polymorphic interfaces */
    /* Note: We use NULL for config and callback initially; the user can cfn_sal_<type>_sensor_init() later */
    cfn_sal_temp_sensor_populate(&sensor->temp, 0, &TEMP_API, phy, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, &HUM_API, phy, NULL, NULL, NULL);
    cfn_sal_pressure_sensor_populate(&sensor->press, 0, &PRESS_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_bme280_destruct(cfn_sal_bme280_t *sensor)
{
    if (!sensor)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    /* Note: individual destructors for interfaces are no-ops here since they are embedded */
    return CFN_HAL_ERROR_OK;
}
