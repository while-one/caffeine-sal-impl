/**
 * @file cfn_sal_vcnl4040.h
 * @brief VCNL4040 composite sensor (Ambient Light and Proximity) implementation.
 */

#ifndef CFN_SAL_VCNL4040_H
#define CFN_SAL_VCNL4040_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ---------------------------------------------------------*/
#include "cfn_sal.h"
#include "devices/cfn_sal_composite.h"
#include "devices/cfn_sal_light_sensor.h"
#include "devices/cfn_sal_prox_sensor.h"
#include "utilities/cfn_sal_timekeeping.h"

/* Constants --------------------------------------------------------*/

#define CFN_SAL_VCNL4040_ADDR_DEFAULT 0x60 /*!< Standard VCNL4040 I2C Address */

/* Types ------------------------------------------------------------*/

/**
 * @brief VCNL4040 composite sensor structure.
 */
typedef struct
{
    cfn_sal_light_sensor_t als;  /*!< Polymorphic Ambient Light Interface */
    cfn_sal_prox_sensor_t  prox; /*!< Polymorphic Proximity Interface */

    cfn_sal_composite_shared_t shared; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    uint64_t last_read_timestamp_ms;
    uint16_t cached_lux;
    uint16_t cached_prox;
} cfn_sal_vcnl4040_t;

/* Public API -------------------------------------------------------*/

/**
 * @brief VCNL4040 monolithic constructor.
 *
 * @param sensor      Pointer to the composite sensor structure.
 * @param phy         Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @param time_source Optional pointer to a timekeeping service for caching logic.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t
cfn_sal_vcnl4040_construct(cfn_sal_vcnl4040_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source);

/**
 * @brief VCNL4040 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_vcnl4040_destruct(cfn_sal_vcnl4040_t *sensor);

/* Getters ----------------------------------------------------------*/

/**
 * @brief Get the abstract Ambient Light interface from the VCNL4040 composite sensor.
 *
 * @param driver Pointer to the VCNL4040 composite sensor structure.
 * @return Pointer to the Ambient Light interface, or NULL if driver is NULL.
 */
static inline cfn_sal_light_sensor_t *cfn_sal_vcnl4040_get_als(cfn_sal_vcnl4040_t *driver)
{
    return (driver == NULL) ? NULL : &driver->als;
}

/**
 * @brief Get the abstract Proximity interface from the VCNL4040 composite sensor.
 *
 * @param driver Pointer to the VCNL4040 composite sensor structure.
 * @return Pointer to the Proximity interface, or NULL if driver is NULL.
 */
static inline cfn_sal_prox_sensor_t *cfn_sal_vcnl4040_get_prox(cfn_sal_vcnl4040_t *driver)
{
    return (driver == NULL) ? NULL : &driver->prox;
}

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_VCNL4040_H */
