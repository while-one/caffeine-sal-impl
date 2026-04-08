/**
 * @file cfn_sal_vcnl4040.c
 * @brief VCNL4040 combined sensor (Ambient Light and Proximity) implementation.
 */

#include "cfn_sal_vcnl4040.h"
#include "cfn_hal_i2c.h"

/* -------------------------------------------------------------------------- */
/* Private Constants                                                          */
/* -------------------------------------------------------------------------- */

#define VCNL4040_REG_ALS_CONF 0x00
#define VCNL4040_REG_ALS_THDH 0x01
#define VCNL4040_REG_ALS_THDL 0x02
#define VCNL4040_REG_PS_CONF1 0x03
#define VCNL4040_REG_PS_MS    0x04
#define VCNL4040_REG_PS_DATA  0x08
#define VCNL4040_REG_ALS_DATA 0x09
#define VCNL4040_REG_ID       0x0C

#define VCNL4040_ID_VAL 0x0186

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t vcnl4040_write_reg16(cfn_hal_i2c_t *i2c, uint16_t addr, uint8_t reg, uint16_t val)
{
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = addr,
        .mem_addr = reg,
        .mem_addr_size = 1,
        .data = (uint8_t*)&val,
        .size = 2
    };
    return cfn_hal_i2c_mem_write(i2c, &xfr, 100);
}

static cfn_hal_error_code_t vcnl4040_read_reg16(cfn_hal_i2c_t *i2c, uint16_t addr, uint8_t reg, uint16_t *val)
{
    cfn_hal_i2c_mem_transaction_t xfr = {
        .dev_addr = addr,
        .mem_addr = reg,
        .mem_addr_size = 1,
        .data = (uint8_t*)val,
        .size = 2
    };
    return cfn_hal_i2c_mem_read(i2c, &xfr, 100);
}

static cfn_sal_vcnl4040_t *get_vcnl4040_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_LIGHT_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_vcnl4040_t, light);
    }
    if (base->type == CFN_SAL_TYPE_PROX_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_vcnl4040_t, prox);
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Shared Lifecycle                                                           */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t vcnl4040_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_vcnl4040_t *vcnl = get_vcnl4040_from_base(base);
    if (!vcnl)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (vcnl->combined_state.init_ref_count == 0)
    {
        cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) vcnl->combined_state.phy->instance;
        if (!dev->i2c)
        {
            return CFN_HAL_ERROR_BAD_PARAM;
        }

        cfn_hal_error_code_t err = cfn_hal_i2c_init(dev->i2c);
        if (err != CFN_HAL_ERROR_OK)
        {
            return err;
        }

        /* WHO_AM_I check */
        uint16_t id = 0;
        err         = vcnl4040_read_reg16(dev->i2c, dev->address, VCNL4040_REG_ID, &id);
        if (err != CFN_HAL_ERROR_OK || (id & 0xFF) != (VCNL4040_ID_VAL & 0xFF))
        {
            return CFN_HAL_ERROR_FAIL;
        }

        /* Power on ALS and PS */
        (void) vcnl4040_write_reg16(dev->i2c, dev->address, VCNL4040_REG_ALS_CONF, 0x0000);
        (void) vcnl4040_write_reg16(dev->i2c, dev->address, VCNL4040_REG_PS_CONF1, 0x0000);

        vcnl->combined_state.hw_initialized = true;
        vcnl->last_read_timestamp_ms        = 0;
    }

    vcnl->combined_state.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t vcnl4040_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_vcnl4040_t *vcnl = get_vcnl4040_from_base(base);
    if (!vcnl)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (vcnl->combined_state.init_ref_count > 0)
    {
        vcnl->combined_state.init_ref_count--;

        if (vcnl->combined_state.init_ref_count == 0)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) vcnl->combined_state.phy->instance;
            /* Shutdown ALS and PS */
            (void) vcnl4040_write_reg16(dev->i2c, dev->address, VCNL4040_REG_ALS_CONF, 0x0001);
            (void) vcnl4040_write_reg16(dev->i2c, dev->address, VCNL4040_REG_PS_CONF1, 0x0001);
            cfn_hal_i2c_deinit(dev->i2c);
            vcnl->combined_state.hw_initialized = false;
        }
    }

    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t vcnl4040_perform_read(cfn_sal_vcnl4040_t *vcnl)
{
    /* Check cache validity (10ms window) */
    uint64_t now         = 0;
    bool     use_caching = false;

    if (vcnl->light.base.dependency != NULL)
    {
        cfn_sal_timekeeping_get_ms((cfn_sal_timekeeping_t *) vcnl->light.base.dependency, &now);
        use_caching = true;
    }

    if (use_caching && vcnl->combined_state.hw_initialized && (now - vcnl->last_read_timestamp_ms < 10))
    {
        return CFN_HAL_ERROR_OK;
    }

    cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) vcnl->combined_state.phy->instance;

    uint16_t             lux  = 0;
    uint16_t             prox = 0;

    cfn_hal_error_code_t err  = vcnl4040_read_reg16(dev->i2c, dev->address, VCNL4040_REG_ALS_DATA, &lux);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    err = vcnl4040_read_reg16(dev->i2c, dev->address, VCNL4040_REG_PS_DATA, &prox);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    vcnl->cached_lux            = lux;
    vcnl->cached_prox           = prox;
    vcnl->last_read_timestamp_ms = now;

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Light Implementation                                                       */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t vcnl4040_light_read_lux(cfn_sal_light_sensor_t *driver, float *lux_out)
{
    if (!driver || !lux_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_vcnl4040_t   *vcnl = CFN_HAL_CONTAINER_OF(driver, cfn_sal_vcnl4040_t, light);
    cfn_hal_error_code_t err  = vcnl4040_perform_read(vcnl);
    if (err == CFN_HAL_ERROR_OK)
    {
        *lux_out = (float) vcnl->cached_lux * 0.1f; /* Simplified lux conversion */
    }
    return err;
}

static cfn_hal_error_code_t vcnl4040_light_read_raw(cfn_sal_light_sensor_t *driver, uint32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_vcnl4040_t   *vcnl = CFN_HAL_CONTAINER_OF(driver, cfn_sal_vcnl4040_t, light);
    cfn_hal_error_code_t err  = vcnl4040_perform_read(vcnl);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (uint32_t) vcnl->cached_lux;
    }
    return err;
}

static cfn_hal_error_code_t vcnl4040_light_set_thresholds(cfn_sal_light_sensor_t *driver, uint32_t low, uint32_t high)
{
    cfn_sal_vcnl4040_t   *vcnl = CFN_HAL_CONTAINER_OF(driver, cfn_sal_vcnl4040_t, light);
    cfn_hal_i2c_device_t *dev  = (cfn_hal_i2c_device_t *) vcnl->combined_state.phy->instance;

    cfn_hal_error_code_t err = vcnl4040_write_reg16(dev->i2c, dev->address, VCNL4040_REG_ALS_THDL, (uint16_t) low);
    if (err == CFN_HAL_ERROR_OK)
    {
        err = vcnl4040_write_reg16(dev->i2c, dev->address, VCNL4040_REG_ALS_THDH, (uint16_t) high);
    }
    return err;
}

static cfn_hal_error_code_t vcnl4040_light_set_integration_time(cfn_sal_light_sensor_t *driver, uint32_t ms)
{
    /* Mocked: Real implementation would calculate closest IT and read-modify-write ALS_CONF */
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(ms);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t vcnl4040_light_set_persistence(cfn_sal_light_sensor_t *driver, uint8_t consec_hits)
{
    /* Mocked: Real implementation would read-modify-write ALS_CONF (ALS_PERS bits) */
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(consec_hits);
    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Proximity Implementation                                                   */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t vcnl4040_prox_read_raw(cfn_sal_prox_sensor_t *driver, uint32_t *raw_out)
{
    if (!driver || !raw_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_vcnl4040_t   *vcnl = CFN_HAL_CONTAINER_OF(driver, cfn_sal_vcnl4040_t, prox);
    cfn_hal_error_code_t err  = vcnl4040_perform_read(vcnl);
    if (err == CFN_HAL_ERROR_OK)
    {
        *raw_out = (uint32_t) vcnl->cached_prox;
    }
    return err;
}

static cfn_hal_error_code_t vcnl4040_prox_read_distance(cfn_sal_prox_sensor_t *driver, float *mm_out)
{
    /* VCNL4040 returns counts, converting to mm requires a LUT or curve fit.
       Returning raw casted as dummy data for the abstract interface. */
    uint32_t             raw = 0;
    cfn_hal_error_code_t err = vcnl4040_prox_read_raw(driver, &raw);
    if (err == CFN_HAL_ERROR_OK && mm_out)
    {
        *mm_out = (float) raw;
    }
    return err;
}

static cfn_hal_error_code_t vcnl4040_prox_set_led_current(cfn_sal_prox_sensor_t *driver, uint32_t ma)
{
    /* Mocked: Read-modify-write PS_MS reg */
    CFN_HAL_UNUSED(driver);
    CFN_HAL_UNUSED(ma);
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_light_sensor_api_t LIGHT_API = {
    .base = {
        .init = vcnl4040_shared_init,
        .deinit = vcnl4040_shared_deinit,
    },
    .read_lux = vcnl4040_light_read_lux,
    .read_raw = vcnl4040_light_read_raw,
    .set_thresholds = vcnl4040_light_set_thresholds,
    .set_integration_time = vcnl4040_light_set_integration_time,
    .set_persistence = vcnl4040_light_set_persistence,
};

static const cfn_sal_prox_sensor_api_t PROX_API = {
    .base = {
        .init = vcnl4040_shared_init,
        .deinit = vcnl4040_shared_deinit,
    },
    .read_distance_mm = vcnl4040_prox_read_distance,
    .read_raw = vcnl4040_prox_read_raw,
    .set_led_current = vcnl4040_prox_set_led_current,
    .set_led_duty_cycle = NULL,
    .set_integration_time = NULL,
};

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t
cfn_sal_vcnl4040_construct(cfn_sal_vcnl4040_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    sensor->combined_state.phy            = phy;
    sensor->combined_state.init_ref_count = 0;
    sensor->combined_state.hw_initialized = false;
    sensor->last_read_timestamp_ms        = 0;

    cfn_sal_light_sensor_populate(&sensor->light, 0, &LIGHT_API, phy, NULL, NULL, NULL);
    cfn_sal_prox_sensor_populate(&sensor->prox, 0, &PROX_API, phy, NULL, NULL, NULL);

    /* Inject time source as a dependency for caching */
    sensor->light.base.dependency = time_source;
    sensor->prox.base.dependency  = time_source;

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_vcnl4040_destruct(const cfn_sal_vcnl4040_t *sensor)
{
    CFN_HAL_UNUSED(sensor);
    return CFN_HAL_ERROR_OK;
}
