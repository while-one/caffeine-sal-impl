/**
 * @file cfn_sal_sht30.h
 * @brief SHT30 composite sensor (Temperature and Humidity) implementation.
 */

#ifndef CFN_SAL_SHT30_H
#define CFN_SAL_SHT30_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ---------------------------------------------------------*/
#include "cfn_sal.h"
#include "devices/cfn_sal_composite.h"
#include "devices/cfn_sal_hum_sensor.h"
#include "devices/cfn_sal_temp_sensor.h"

/* Constants --------------------------------------------------------*/

#define CFN_SAL_SHT30_ADDR_DEFAULT 0x44 /*!< Standard SHT30 I2C Address */

/* Types ------------------------------------------------------------*/

/**
 * @brief SHT30 composite sensor structure.
 */
typedef struct
{
    cfn_sal_temp_sensor_t temp; /*!< Polymorphic Temperature Interface */
    cfn_sal_hum_sensor_t  hum;  /*!< Polymorphic Humidity Interface */

    cfn_sal_composite_shared_t shared; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    uint32_t last_read_timestamp_ms;
    float    cached_temp_celsius;
    float    cached_hum_rh;
} cfn_sal_sht30_t;

/* Public API -------------------------------------------------------*/

/**
 * @brief SHT30 monolithic constructor.
 *
 * @param sensor     Pointer to the composite sensor structure.
 * @param phy        Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @param dependency Optional pointer to a dependency for hardware binding.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_sht30_construct(cfn_sal_sht30_t *sensor, const cfn_sal_phy_t *phy, void *dependency);

/**
 * @brief SHT30 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_sht30_destruct(cfn_sal_sht30_t *sensor);

/* Getters ----------------------------------------------------------*/

/**
 * @brief Get the abstract Temperature interface from the SHT30 composite sensor.
 *
 * @param driver Pointer to the SHT30 composite sensor structure.
 * @return Pointer to the Temperature interface, or NULL if driver is NULL.
 */
static inline cfn_sal_temp_sensor_t *cfn_sal_sht30_get_temp(cfn_sal_sht30_t *driver)
{
    return (driver == NULL) ? NULL : &driver->temp;
}

/**
 * @brief Get the abstract Humidity interface from the SHT30 composite sensor.
 *
 * @param driver Pointer to the SHT30 composite sensor structure.
 * @return Pointer to the Humidity interface, or NULL if driver is NULL.
 */
static inline cfn_sal_hum_sensor_t *cfn_sal_sht30_get_hum(cfn_sal_sht30_t *driver)
{
    return (driver == NULL) ? NULL : &driver->hum;
}

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_SHT30_H */
