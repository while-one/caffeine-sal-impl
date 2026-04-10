/**
 * @file cfn_sal_opt4048.h
 * @brief OPT4048 composite sensor (Ambient Light and Color) implementation.
 */

#ifndef CFN_SAL_OPT4048_H
#define CFN_SAL_OPT4048_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ---------------------------------------------------------*/
#include "cfn_sal.h"
#include "devices/cfn_sal_color_sensor.h"
#include "devices/cfn_sal_composite.h"
#include "devices/cfn_sal_light_sensor.h"
#include "utilities/cfn_sal_timekeeping.h"

/* Constants --------------------------------------------------------*/

#define CFN_SAL_OPT4048_ADDR_0 0x44 /*!< ADDR pin tied to GND */
#define CFN_SAL_OPT4048_ADDR_1 0x45 /*!< ADDR pin tied to VDD */
#define CFN_SAL_OPT4048_ADDR_2 0x46 /*!< ADDR pin tied to SDA */
#define CFN_SAL_OPT4048_ADDR_3 0x47 /*!< ADDR pin tied to SCL */

#define CFN_SAL_OPT4048_ADDR_DEFAULT CFN_SAL_OPT4048_ADDR_0

/* Types ------------------------------------------------------------*/

/**
 * @brief OPT4048 composite sensor structure.
 */
typedef struct
{
    cfn_sal_light_sensor_t als;   /*!< Polymorphic Ambient Light Interface */
    cfn_sal_color_sensor_t color; /*!< Polymorphic Color Interface */

    cfn_sal_composite_shared_t shared; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    uint64_t last_read_timestamp_ms;
    float    cached_lux;
    float    cached_x;
    float    cached_y;
    float    cached_z;
    float    cached_cct;
    uint32_t cached_raw_ch0;
    uint32_t cached_raw_ch1;
    uint32_t cached_raw_ch2;
    uint32_t cached_raw_ch3;
} cfn_sal_opt4048_t;

/* Public API -------------------------------------------------------*/

/**
 * @brief OPT4048 monolithic constructor.
 *
 * @param sensor      Pointer to the composite sensor structure.
 * @param phy         Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @param time_source Optional pointer to a timekeeping service for caching logic.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t
cfn_sal_opt4048_construct(cfn_sal_opt4048_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source);

/**
 * @brief OPT4048 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_opt4048_destruct(cfn_sal_opt4048_t *sensor);

/* Getters ----------------------------------------------------------*/

/**
 * @brief Get the abstract Ambient Light interface from the OPT4048 composite sensor.
 *
 * @param driver Pointer to the OPT4048 composite sensor structure.
 * @return Pointer to the Ambient Light interface, or NULL if driver is NULL.
 */
static inline cfn_sal_light_sensor_t *cfn_sal_opt4048_get_als(cfn_sal_opt4048_t *driver)
{
    return (driver == NULL) ? NULL : &driver->als;
}

/**
 * @brief Get the abstract Color interface from the OPT4048 composite sensor.
 *
 * @param driver Pointer to the OPT4048 composite sensor structure.
 * @return Pointer to the Color interface, or NULL if driver is NULL.
 */
static inline cfn_sal_color_sensor_t *cfn_sal_opt4048_get_color(cfn_sal_opt4048_t *driver)
{
    return (driver == NULL) ? NULL : &driver->color;
}

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_OPT4048_H */
