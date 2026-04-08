/**
 * @file cfn_sal_sht30.h
 * @brief SHT30 combined sensor (Temperature and Humidity) implementation.
 */

#ifndef CFN_SAL_SHT30_H
#define CFN_SAL_SHT30_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cfn_sal.h"
#include "devices/cfn_sal_temp_sensor.h"
#include "devices/cfn_sal_hum_sensor.h"

/* -------------------------------------------------------------------------- */
/* Constants                                                                  */
/* -------------------------------------------------------------------------- */

#define CFN_SAL_SHT30_ADDR_DEFAULT 0x44 /*!< Standard SHT30 I2C Address */

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief SHT30 composite sensor structure.
 */
typedef struct
{
    cfn_sal_temp_sensor_t temp; /*!< Polymorphic Temperature Interface */
    cfn_sal_hum_sensor_t  hum;  /*!< Polymorphic Humidity Interface */

    cfn_sal_combined_state_t combined_state; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    uint32_t last_read_timestamp_ms;
    float    cached_temp_celsius;
    float    cached_hum_rh;
} cfn_sal_sht30_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief SHT30 monolithic constructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @param phy    Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_sht30_construct(cfn_sal_sht30_t *sensor, const cfn_sal_phy_t *phy);

/**
 * @brief SHT30 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_sht30_destruct(const cfn_sal_sht30_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_SHT30_H */
