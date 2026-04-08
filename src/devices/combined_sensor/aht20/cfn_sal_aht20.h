/**
 * @file cfn_sal_aht20.h
 * @brief AHT20 combined sensor (Temperature and Humidity) implementation.
 */

#ifndef CFN_SAL_AHT20_H
#define CFN_SAL_AHT20_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cfn_sal.h"
#include "devices/cfn_sal_temp_sensor.h"
#include "devices/cfn_sal_hum_sensor.h"
#include "utilities/cfn_sal_timekeeping.h"

/* -------------------------------------------------------------------------- */
/* Constants                                                                  */
/* -------------------------------------------------------------------------- */

#define CFN_SAL_AHT20_ADDR_DEFAULT 0x38 /*!< Standard AHT20 I2C Address */

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief AHT20 composite sensor structure.
 */
typedef struct
{
    cfn_sal_temp_sensor_t temp; /*!< Polymorphic Temperature Interface */
    cfn_sal_hum_sensor_t  hum;  /*!< Polymorphic Humidity Interface */

    cfn_sal_combined_state_t combined_state; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    uint64_t last_read_timestamp_ms;
    float    cached_temp_celsius;
    float    cached_hum_rh;
} cfn_sal_aht20_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief AHT20 monolithic constructor.
 *
 * @param sensor      Pointer to the composite sensor structure.
 * @param phy         Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @param time_source Optional pointer to a timekeeping service for caching logic.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t
cfn_sal_aht20_construct(cfn_sal_aht20_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source);

/**
 * @brief AHT20 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_aht20_destruct(const cfn_sal_aht20_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_AHT20_H */
