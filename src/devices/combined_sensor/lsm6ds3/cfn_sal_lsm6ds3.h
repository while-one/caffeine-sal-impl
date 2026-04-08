/**
 * @file cfn_sal_lsm6ds3.h
 * @brief LSM6DS3 combined sensor (Accelerometer and Gyroscope) implementation.
 */

#ifndef CFN_SAL_LSM6DS3_H
#define CFN_SAL_LSM6DS3_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cfn_sal.h"
#include "devices/cfn_sal_accel.h"
#include "devices/cfn_sal_gyro_sensor.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_spi.h"

/* -------------------------------------------------------------------------- */
/* Constants                                                                  */
/* -------------------------------------------------------------------------- */

#define CFN_SAL_LSM6DS3_I2C_ADDR_0 0x6A /*!< SDO/SA0 pin tied to GND */
#define CFN_SAL_LSM6DS3_I2C_ADDR_1 0x6B /*!< SDO/SA0 pin tied to VDD */

#define CFN_SAL_LSM6DS3_I2C_ADDR_DEFAULT CFN_SAL_LSM6DS3_I2C_ADDR_0

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief LSM6DS3 composite sensor structure.
 */
typedef struct
{
    cfn_sal_accel_t       accel; /*!< Polymorphic Accelerometer Interface */
    cfn_sal_gyro_sensor_t gyro;  /*!< Polymorphic Gyroscope Interface */

    cfn_sal_combined_state_t combined_state; /*!< Shared Framework State (PHY, ref count) */

    /* Internal Caching */
    cfn_sal_accel_data_t cached_accel_raw;
    cfn_sal_gyro_data_t  cached_gyro_raw;
    
    /* Config tracking */
    cfn_sal_accel_range_t current_accel_range;
    cfn_sal_gyro_range_t  current_gyro_range;
} cfn_sal_lsm6ds3_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief LSM6DS3 monolithic constructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @param phy    Pointer to the physical interface mapping (Must point to cfn_hal_i2c_device_t or cfn_hal_spi_device_t).
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_lsm6ds3_construct(cfn_sal_lsm6ds3_t *sensor, const cfn_sal_phy_t *phy);

/**
 * @brief LSM6DS3 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_lsm6ds3_destruct(const cfn_sal_lsm6ds3_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_LSM6DS3_H */
