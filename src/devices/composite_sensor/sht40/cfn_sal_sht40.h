/**
 * @file cfn_sal_sht40.h
 * @brief SHT40 composite sensor (Temperature and Humidity) implementation.
 */

#ifndef CFN_SAL_SHT40_H
#define CFN_SAL_SHT40_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ---------------------------------------------------------*/
#include "cfn_sal.h"
#include "devices/cfn_sal_composite.h"
#include "devices/cfn_sal_hum_sensor.h"
#include "devices/cfn_sal_temp_sensor.h"
#include "utilities/cfn_sal_timekeeping.h"

/* Constants --------------------------------------------------------*/

#define CFN_SAL_SHT40_ADDR_DEFAULT 0x44 /*!< Standard SHT40 I2C Address */

/* Types ------------------------------------------------------------*/

/**
 * @brief SHT40 composite sensor structure.
 * Standardizes access to both Temperature and Humidity interfaces.
 */
typedef struct
{
    cfn_sal_temp_sensor_t temp; /*!< Polymorphic Temperature Interface */
    cfn_sal_hum_sensor_t  hum;  /*!< Polymorphic Humidity Interface */

    cfn_sal_composite_shared_t shared; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    uint64_t last_read_timestamp_ms;
    float    cached_temp_celsius;
    float    cached_hum_rh;
} cfn_sal_sht40_t;

/* Public API -------------------------------------------------------*/

/**
 * @brief SHT40 monolithic constructor.
 * Initializes both polymorphic interfaces and links them to the shared physical mapping.
 *
 * @param sensor      Pointer to the composite sensor structure.
 * @param phy         Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @param time_source Optional pointer to a timekeeping service for caching logic.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t
cfn_sal_sht40_construct(cfn_sal_sht40_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source);

/**
 * @brief SHT40 destructor.
 * Safely tears down the sensor interfaces.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_sht40_destruct(cfn_sal_sht40_t *sensor);

/* Getters ----------------------------------------------------------*/

/**
 * @brief Get the abstract Temperature interface from the SHT40 composite sensor.
 *
 * @param driver Pointer to the SHT40 composite sensor structure.
 * @return Pointer to the Temperature interface, or NULL if driver is NULL.
 */
static inline cfn_sal_temp_sensor_t *cfn_sal_sht40_get_temp(cfn_sal_sht40_t *driver)
{
    return (driver == NULL) ? NULL : &driver->temp;
}

/**
 * @brief Get the abstract Humidity interface from the SHT40 composite sensor.
 *
 * @param driver Pointer to the SHT40 composite sensor structure.
 * @return Pointer to the Humidity interface, or NULL if driver is NULL.
 */
static inline cfn_sal_hum_sensor_t *cfn_sal_sht40_get_hum(cfn_sal_sht40_t *driver)
{
    return (driver == NULL) ? NULL : &driver->hum;
}

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_SHT40_H */
