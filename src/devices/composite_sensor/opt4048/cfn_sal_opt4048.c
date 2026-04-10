/**
 * @file cfn_sal_opt4048.c
 * @brief OPT4048 composite sensor implementation.
 */

#include "cfn_sal_opt4048.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_util.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Constants & Definitions                                                    */
/* -------------------------------------------------------------------------- */

#define OPT4048_REG_CH0_MSB   0x00
#define OPT4048_REG_CH0_LSB   0x01
#define OPT4048_REG_CH1_MSB   0x02
#define OPT4048_REG_CH1_LSB   0x03
#define OPT4048_REG_CH2_MSB   0x04
#define OPT4048_REG_CH2_LSB   0x05
#define OPT4048_REG_CH3_MSB   0x06
#define OPT4048_REG_CH3_LSB   0x07
#define OPT4048_REG_CONFIG    0x0A
#define OPT4048_REG_DEVICE_ID 0x11

#define OPT4048_DEVICE_ID_VAL 0x0821 /* Typically 0x0821 or 0x0822 depending on revision/silicon */

#define OPT4048_CONFIG_RANGE_AUTO 0xC000UL
#define OPT4048_CONFIG_OP_CONT    0x0020UL
#define OPT4048_CONFIG_LATCH      0x0008UL

#define OPT4048_I2C_TIMEOUT_MS 100

/* Mathematical scaling factor (2.15 mlux per LSB) */
#define OPT4048_LSB_STEP (0.00215F)

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static cfn_sal_opt4048_t *get_opt4048_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_LIGHT_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_opt4048_t, als);
    }

    if (base->type == CFN_SAL_TYPE_COLOR_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_opt4048_t, color);
    }
    return NULL;
}

static cfn_hal_error_code_t opt4048_read_reg16(cfn_hal_i2c_t *i2c, uint16_t addr, uint8_t reg, uint16_t *val)
{
    uint8_t                       buffer[2];
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = addr, .mem_addr = reg, .mem_addr_size = 1, .data = buffer, .size = 2
    };

    cfn_hal_error_code_t err = cfn_hal_i2c_mem_read(i2c, &xfr, OPT4048_I2C_TIMEOUT_MS);
    if (err == CFN_HAL_ERROR_OK)
    {
        /* I2C standard is MSB first, check device. OPT usually outputs MSB then LSB */
        *val = (uint16_t) cfn_util_bytes_to_int16_le(buffer[0], buffer[1]);
    }
    return err;
}

static cfn_hal_error_code_t opt4048_write_reg16(cfn_hal_i2c_t *i2c, uint16_t addr, uint8_t reg, uint16_t val)
{
    uint8_t buffer[2];
    buffer[0]                         = cfn_util_get_msb16((uint16_t) val);
    buffer[1]                         = cfn_util_get_lsb16((uint16_t) val);

    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = addr, .mem_addr = reg, .mem_addr_size = 1, .data = buffer, .size = 2
    };
    return cfn_hal_i2c_mem_write(i2c, &xfr, OPT4048_I2C_TIMEOUT_MS);
}

static uint32_t opt4048_decode_channel(uint16_t msb, uint16_t lsb)
{
    /* * msb: [ E E E E M M M M | M M M M M M M M ] (16-bit)
     * lsb: [ M M M M M M M M | . . . . . . . . ] (16-bit)
     */

    /* Exponent: Top 4 bits of the 16-bit MSB register */
    uint8_t exponent = (uint8_t) cfn_util_extract_field((uint32_t) msb, 0xF000U, 12U);
    /* * Mantissa:
     * 1. Take bottom 12 bits of MSB, shift left by 8
     * 2. Take top 8 bits of LSB
     */
    uint32_t mantissa =
        ((uint32_t) (msb & 0x0FFFU) << 8U) | (uint32_t) cfn_util_extract_field((uint16_t) lsb, 0xFF00U, 8U);

    /* Value = Mantissa * 2^Exponent */
    return mantissa << exponent;
}

/**
 * @brief Performs the shared hardware initialization for the OPT4048 sensor.
 * @details Implements reference counting to ensure the I2C bus and sensor
 * configuration are only initialized once for shared driver instances.
 * * @param[in] base Pointer to the abstract HAL driver structure.
 * @return cfn_hal_error_code_t Result of the initialization sequence.
 */
static cfn_hal_error_code_t opt4048_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_opt4048_t *opt = get_opt4048_from_base(base);

    /* 1. Parameter Validation */
    if (!opt || !opt->shared.phy || !opt->shared.phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    /* 2. Atomic-style Initialization check */
    if (opt->shared.init_ref_count == 0U)
    {
        cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) opt->shared.phy->instance;
        if (!dev->i2c)
        {
            return CFN_HAL_ERROR_BAD_PARAM;
        }

        /* Initialize underlying I2C peripheral */
        cfn_hal_error_code_t err = cfn_hal_i2c_init(dev->i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        /* 3. Hardware Verification (Device ID) */
        uint16_t id_val           = 0U;
        err                       = opt4048_read_reg16(dev->i2c, dev->address, OPT4048_REG_DEVICE_ID, &id_val);

        /* Check only the MSB (Device ID) and ignore the LSB (Revision) */
        const uint8_t ACTUAL_ID   = cfn_util_get_msb16(id_val);
        const uint8_t EXPECTED_ID = cfn_util_get_msb16((uint16_t) OPT4048_DEVICE_ID_VAL);

        if ((err != CFN_HAL_ERROR_OK) || (ACTUAL_ID != EXPECTED_ID))
        {
            /* Explicit Cleanup: Don't leave the I2C bus initialized if the sensor isn't there */
            (void) cfn_hal_i2c_deinit(dev->i2c);
            return CFN_HAL_ERROR_PERIPHERAL_FAIL;
        }

        /* 4. Sensor Configuration */
        const uint16_t CONFIG =
            (uint32_t) OPT4048_CONFIG_RANGE_AUTO | (uint32_t) OPT4048_CONFIG_OP_CONT | (uint32_t) OPT4048_CONFIG_LATCH;

        err = opt4048_write_reg16(dev->i2c, dev->address, OPT4048_REG_CONFIG, CONFIG);
        if (err != CFN_HAL_ERROR_OK)
        {
            /* Configuration failure is a functional error */
            (void) cfn_hal_i2c_deinit(dev->i2c);
            return err;
        }

        opt->shared.hw_initialized = true;
    }

    /* Increment reference count only on successful (or already completed) init */
    opt->shared.init_ref_count++;

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t opt4048_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_opt4048_t *opt = get_opt4048_from_base(base);
    if (!opt)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (opt->shared.init_ref_count > 0)
    {
        opt->shared.init_ref_count--;

        if (opt->shared.init_ref_count == 0)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) opt->shared.phy->instance;

            /* Put to sleep: Operating Mode = 00 (Standby) */
            uint16_t cfg              = 0;
            (void) opt4048_read_reg16(dev->i2c, dev->address, OPT4048_REG_CONFIG, &cfg);
            cfg &= ~OPT4048_CONFIG_OP_CONT;
            (void) opt4048_write_reg16(dev->i2c, dev->address, OPT4048_REG_CONFIG, cfg);

            cfn_hal_i2c_deinit(dev->i2c);
            opt->shared.hw_initialized = false;
        }
    }

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t opt4048_perform_read(cfn_sal_opt4048_t *opt)
{
    /* Check cache validity (10ms window) */
    uint64_t now         = 0;
    bool     use_caching = false;

    if (opt->als.base.dependency != NULL)
    {
        cfn_sal_timekeeping_get_ms((cfn_sal_timekeeping_t *) opt->als.base.dependency, &now);
        use_caching = true;
    }

    if (use_caching && opt->shared.hw_initialized && (now - opt->last_read_timestamp_ms < 10))
    {
        return CFN_HAL_ERROR_OK;
    }

    cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) opt->shared.phy->instance;
    uint16_t              regs[8];

    /* Sequential read of all 4 channels (8 registers total) starting from 0x00 */
    uint8_t                       buffer[16];
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = dev->address, .mem_addr = OPT4048_REG_CH0_MSB, .mem_addr_size = 1, .data = buffer, .size = 16
    };

    cfn_hal_error_code_t err = cfn_hal_i2c_mem_read(dev->i2c, &xfr, OPT4048_I2C_TIMEOUT_MS);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    for (size_t i = 0; i < 8; i++)
    {
        regs[i] = (uint16_t) cfn_util_bytes_to_int16_le(buffer[i * 2], buffer[(i * 2) + 1]);
    }

    opt->cached_raw_ch0 = opt4048_decode_channel(regs[0], regs[1]);
    opt->cached_raw_ch1 = opt4048_decode_channel(regs[2], regs[3]);
    opt->cached_raw_ch2 = opt4048_decode_channel(regs[4], regs[5]);
    opt->cached_raw_ch3 = opt4048_decode_channel(regs[6], regs[7]);

    /* Math based on standard OPT4048 scaling */
    opt->cached_x       = (float) opt->cached_raw_ch0 * OPT4048_LSB_STEP;
    opt->cached_y       = (float) opt->cached_raw_ch1 * OPT4048_LSB_STEP;
    opt->cached_z       = (float) opt->cached_raw_ch2 * OPT4048_LSB_STEP;

    /* Y channel represents Lux directly */
    opt->cached_lux     = opt->cached_y;

    /* Calculate CCT using McCamy's formula */
    float sum_xyz       = opt->cached_x + opt->cached_y + opt->cached_z;
    if (sum_xyz > 0.0001F)
    {
        float x_chrom   = opt->cached_x / sum_xyz;
        float y_chrom   = opt->cached_y / sum_xyz;
        float n         = (x_chrom - 0.3320F) / (0.1858F - y_chrom);
        float n2        = n * n;
        float n3        = n2 * n;
        opt->cached_cct = (449.0F * n3) + (3525.0F * n2) + (6823.3F * n) + 5520.33F;
    }
    else
    {
        opt->cached_cct = 0.0F;
    }

    opt->last_read_timestamp_ms = now;

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Light Implementation                                                       */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t opt4048_light_read_raw(cfn_sal_light_sensor_t *driver, uint32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_opt4048_t   *opt = CFN_HAL_CONTAINER_OF(driver, cfn_sal_opt4048_t, als);
    cfn_hal_error_code_t err = opt4048_perform_read(opt);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = opt->cached_raw_ch1; /* Lux is CH1 */
    }
    return err;
}

static cfn_hal_error_code_t opt4048_light_read_lux(cfn_sal_light_sensor_t *driver, float *lux_out)
{
    if (!driver || !lux_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_opt4048_t   *opt = CFN_HAL_CONTAINER_OF(driver, cfn_sal_opt4048_t, als);
    cfn_hal_error_code_t err = opt4048_perform_read(opt);
    if (err == CFN_HAL_ERROR_OK)
    {
        *lux_out = opt->cached_lux;
    }
    return err;
}

static cfn_hal_error_code_t opt4048_light_set_thresholds(cfn_sal_light_sensor_t *driver, uint32_t low, uint32_t high)
{
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(low);
    CFN_HAL_UNUSED(high);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t opt4048_light_set_integration_time(cfn_sal_light_sensor_t *driver, uint32_t ms)
{
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(ms);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t opt4048_light_set_persistence(cfn_sal_light_sensor_t *driver, uint8_t consec_hits)
{
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(consec_hits);
    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Color Implementation                                                       */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t opt4048_color_read_xyz(cfn_sal_color_sensor_t *driver, float *x, float *y, float *z)
{
    if (!driver || !x || !y || !z)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_opt4048_t   *opt = CFN_HAL_CONTAINER_OF(driver, cfn_sal_opt4048_t, color);
    cfn_hal_error_code_t err = opt4048_perform_read(opt);
    if (err == CFN_HAL_ERROR_OK)
    {
        *x = opt->cached_x;
        *y = opt->cached_y;
        *z = opt->cached_z;
    }
    return err;
}

static cfn_hal_error_code_t opt4048_color_read_cct(cfn_sal_color_sensor_t *driver, float *cct_k)
{
    if (!driver || !cct_k)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_opt4048_t   *opt = CFN_HAL_CONTAINER_OF(driver, cfn_sal_opt4048_t, color);
    cfn_hal_error_code_t err = opt4048_perform_read(opt);
    if (err == CFN_HAL_ERROR_OK)
    {
        *cct_k = opt->cached_cct;
    }
    return err;
}

static cfn_hal_error_code_t opt4048_color_read_raw_channels(
    cfn_sal_color_sensor_t *driver, uint32_t *ch0, uint32_t *ch1, uint32_t *ch2, uint32_t *ch3)
{
    if (!driver || !ch0 || !ch1 || !ch2 || !ch3)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_opt4048_t   *opt = CFN_HAL_CONTAINER_OF(driver, cfn_sal_opt4048_t, color);
    cfn_hal_error_code_t err = opt4048_perform_read(opt);
    if (err == CFN_HAL_ERROR_OK)
    {
        *ch0 = opt->cached_raw_ch0;
        *ch1 = opt->cached_raw_ch1;
        *ch2 = opt->cached_raw_ch2;
        *ch3 = opt->cached_raw_ch3;
    }
    return err;
}

static cfn_hal_error_code_t opt4048_color_set_auto_range(cfn_sal_color_sensor_t *driver, bool enable)
{
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(enable);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t opt4048_color_set_integration_time(cfn_sal_color_sensor_t *driver, uint32_t ms)
{
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(ms);
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_light_sensor_api_t LIGHT_API = {
    .base = {
        .init = opt4048_shared_init,
        .deinit = opt4048_shared_deinit,
    },
    .read_lux = opt4048_light_read_lux,
    .read_raw = opt4048_light_read_raw,
    .read_channels = NULL,
    .set_gain = NULL,
    .set_integration_time = opt4048_light_set_integration_time,
    .set_thresholds = opt4048_light_set_thresholds,
    .set_persistence = opt4048_light_set_persistence,
};

static const cfn_sal_color_sensor_api_t COLOR_API = {
    .base = {
        .init = opt4048_shared_init,
        .deinit = opt4048_shared_deinit,
    },
    .read_xyz = opt4048_color_read_xyz,
    .read_cct = opt4048_color_read_cct,
    .read_raw_channels = opt4048_color_read_raw_channels,
    .set_integration_time = opt4048_color_set_integration_time,
    .set_auto_range = opt4048_color_set_auto_range,
    .set_thresholds = NULL,
    .get_status = NULL,
};

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t
cfn_sal_opt4048_construct(cfn_sal_opt4048_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_composite_init(&sensor->shared, phy);
    sensor->last_read_timestamp_ms = 0;

    cfn_sal_light_sensor_populate(&sensor->als, 0, (void *) time_source, &LIGHT_API, phy, NULL, NULL, NULL);
    cfn_sal_color_sensor_populate(&sensor->color, 0, (void *) time_source, &COLOR_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_opt4048_destruct(cfn_sal_opt4048_t *sensor)
{
    if (sensor == NULL)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_light_sensor_populate(&sensor->als, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    cfn_sal_color_sensor_populate(&sensor->color, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}
