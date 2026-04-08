/**
 * @file cfn_sal_opt4048.h
 * @brief OPT4048 combined sensor (Ambient Light and Color) implementation.
 */

#ifndef CFN_SAL_OPT4048_H
#define CFN_SAL_OPT4048_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cfn_sal.h"
#include "devices/cfn_sal_light_sensor.h"
#include "devices/cfn_sal_color_sensor.h"

/* -------------------------------------------------------------------------- */
/* Constants                                                                  */
/* -------------------------------------------------------------------------- */

#define CFN_SAL_OPT4048_ADDR_0 0x44 /*!< ADDR pin tied to GND */
#define CFN_SAL_OPT4048_ADDR_1 0x45 /*!< ADDR pin tied to VDD */
#define CFN_SAL_OPT4048_ADDR_2 0x46 /*!< ADDR pin tied to SDA */
#define CFN_SAL_OPT4048_ADDR_3 0x47 /*!< ADDR pin tied to SCL */

#define CFN_SAL_OPT4048_ADDR_DEFAULT CFN_SAL_OPT4048_ADDR_0

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief OPT4048 composite sensor structure.
 */
typedef struct
{
    cfn_sal_light_sensor_t light; /*!< Polymorphic Ambient Light Interface */
    cfn_sal_color_sensor_t color; /*!< Polymorphic Color Interface */

    cfn_sal_combined_state_t combined_state; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
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

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief OPT4048 monolithic constructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @param phy    Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t in instance).
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_opt4048_construct(cfn_sal_opt4048_t *sensor, const cfn_sal_phy_t *phy);

/**
 * @brief OPT4048 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_opt4048_destruct(const cfn_sal_opt4048_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_OPT4048_H */
