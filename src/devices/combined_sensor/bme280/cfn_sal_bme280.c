/**
 * @file cfn_sal_bme280.c
 * @brief BME280 combination sensor implementation.
 */

#include "cfn_sal_bme280.h"
#include "cfn_hal_i2c.h"

/* -------------------------------------------------------------------------- */
/* Shared Context Management                                                  */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_shared_init(cfn_hal_driver_t *base)
{
    /* To access the shared context, we cast the driver to any of the supported types,
       since they all have `cfn_hal_driver_t base` as their first member, and a `phy` pointer.
       However, to be safe, we extract the phy pointer depending on the type. */
    const cfn_sal_phy_t *phy = NULL;

    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        phy = ((cfn_sal_temp_sensor_t *) base)->phy;
    }
    else if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        phy = ((cfn_sal_hum_sensor_t *) base)->phy;
    }
    else if (base->type == CFN_SAL_TYPE_PRESSURE_SENSOR)
    {
        phy = ((cfn_sal_pressure_sensor_t *) base)->phy;
    }

    if (!phy || !phy->user_arg || !phy->handle)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_ctx_t *ctx = (cfn_sal_bme280_ctx_t *) phy->user_arg;

    /* Only initialize hardware if this is the first interface to call init */
    if (cfn_sal_shared_ctx_should_init(&ctx->base))
    {
        cfn_hal_i2c_t *i2c        = (cfn_hal_i2c_t *) phy->handle;
        ctx->base.last_init_error = cfn_hal_i2c_init(i2c);

        if (ctx->base.last_init_error == CFN_HAL_ERROR_OK)
        {
            ctx->hw_initialized = true;
            /* Mock: perform BME280 specific initialization / calibration reading here */
            ctx->t_fine         = 0;
        }
    }

    return ctx->base.last_init_error;
}

static cfn_hal_error_code_t bme280_shared_deinit(cfn_hal_driver_t *base)
{
    const cfn_sal_phy_t *phy = NULL;

    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        phy = ((cfn_sal_temp_sensor_t *) base)->phy;
    }
    else if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        phy = ((cfn_sal_hum_sensor_t *) base)->phy;
    }
    else if (base->type == CFN_SAL_TYPE_PRESSURE_SENSOR)
    {
        phy = ((cfn_sal_pressure_sensor_t *) base)->phy;
    }

    if (!phy || !phy->user_arg || !phy->handle)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_bme280_ctx_t *ctx = (cfn_sal_bme280_ctx_t *) phy->user_arg;

    /* Only deinitialize hardware if this is the last interface to call deinit */
    if (cfn_sal_shared_ctx_should_deinit(&ctx->base))
    {
        cfn_hal_i2c_t *i2c        = (cfn_hal_i2c_t *) phy->handle;
        ctx->base.last_init_error = cfn_hal_i2c_deinit(i2c);
        ctx->hw_initialized       = false;
    }

    return ctx->base.last_init_error;
}

/* -------------------------------------------------------------------------- */
/* Temperature Sensor Implementation                                          */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_temp_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    const cfn_sal_bme280_ctx_t *ctx = (const cfn_sal_bme280_ctx_t *) driver->phy->user_arg;
    if (!ctx->hw_initialized)
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
    if (!driver || !driver->phy || !driver->phy->user_arg || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    const cfn_sal_bme280_ctx_t *ctx = (const cfn_sal_bme280_ctx_t *) driver->phy->user_arg;
    if (!ctx->hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }
    *raw_out = 0x1234; /* Mock raw data */
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base = {
        .init = bme280_shared_init,
        .deinit = bme280_shared_deinit,
    },
    .read_celsius = bme280_temp_read_celsius,
    .read_fahrenheit = bme280_temp_read_fahrenheit,
    .read_raw = bme280_temp_read_raw,
};

cfn_hal_error_code_t cfn_sal_bme280_temp_construct(cfn_sal_temp_sensor_t       *driver,
                                                   const cfn_sal_temp_config_t *config,
                                                   const cfn_sal_phy_t         *phy,
                                                   cfn_sal_temp_callback_t      callback,
                                                   void                        *user_arg)
{
    if (!driver || !phy)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_temp_sensor_populate(driver, 0, &TEMP_API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Humidity Sensor Implementation                                             */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_hum_read_percent(cfn_sal_hum_sensor_t *driver, float *hum_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !hum_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    const cfn_sal_bme280_ctx_t *ctx = (const cfn_sal_bme280_ctx_t *) driver->phy->user_arg;
    if (!ctx->hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    /* Mock reading */
    *hum_out = 50.0f;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_hum_read_raw(cfn_sal_hum_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    const cfn_sal_bme280_ctx_t *ctx = (const cfn_sal_bme280_ctx_t *) driver->phy->user_arg;
    if (!ctx->hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }
    *raw_out = 0x5678; /* Mock raw data */
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base = {
        .init = bme280_shared_init,
        .deinit = bme280_shared_deinit,
    },
    .read_relative_humidity = bme280_hum_read_percent,
    .read_raw = bme280_hum_read_raw,
};

cfn_hal_error_code_t cfn_sal_bme280_hum_construct(cfn_sal_hum_sensor_t       *driver,
                                                  const cfn_sal_hum_config_t *config,
                                                  const cfn_sal_phy_t        *phy,
                                                  cfn_sal_hum_callback_t      callback,
                                                  void                       *user_arg)
{
    if (!driver || !phy)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_hum_sensor_populate(driver, 0, &HUM_API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Pressure Sensor Implementation                                             */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_press_read_hpa(cfn_sal_pressure_sensor_t *driver, float *hpa_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !hpa_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    const cfn_sal_bme280_ctx_t *ctx = (const cfn_sal_bme280_ctx_t *) driver->phy->user_arg;
    if (!ctx->hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }

    /* Mock reading */
    *hpa_out = 1013.25f;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_press_read_raw(cfn_sal_pressure_sensor_t *driver, int32_t *raw_out)
{
    if (!driver || !driver->phy || !driver->phy->user_arg || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    const cfn_sal_bme280_ctx_t *ctx = (const cfn_sal_bme280_ctx_t *) driver->phy->user_arg;
    if (!ctx->hw_initialized)
    {
        return CFN_HAL_ERROR_DRIVER_NOT_INIT;
    }
    *raw_out = 0x9ABC; /* Mock raw data */
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_pressure_sensor_api_t PRESS_API = {
    .base = {
        .init = bme280_shared_init,
        .deinit = bme280_shared_deinit,
    },
    .read_hpa = bme280_press_read_hpa,
    .read_raw = bme280_press_read_raw,
};

cfn_hal_error_code_t cfn_sal_bme280_pressure_construct(cfn_sal_pressure_sensor_t       *driver,
                                                       const cfn_sal_pressure_config_t *config,
                                                       const cfn_sal_phy_t             *phy,
                                                       cfn_sal_pressure_callback_t      callback,
                                                       void                            *user_arg)
{
    if (!driver || !phy)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_pressure_sensor_populate(driver, 0, &PRESS_API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}
