/**
 * @file cfn_sal_bme280.h
 * @brief BME280 combined sensor (Temperature, Humidity, Pressure) implementation.
 */

#ifndef CFN_SAL_BME280_H
#define CFN_SAL_BME280_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cfn_sal.h"
#include "devices/cfn_sal_temp_sensor.h"
#include "devices/cfn_sal_hum_sensor.h"
#include "devices/cfn_sal_pressure_sensor.h"

/**
 * @brief BME280 composite sensor structure.
 * Standardizes access to all three sensor interfaces through a single container.
 */
typedef struct
{
    cfn_sal_temp_sensor_t     temp;           /*!< Polymorphic Temperature Interface */
    cfn_sal_hum_sensor_t      hum;            /*!< Polymorphic Humidity Interface */
    cfn_sal_pressure_sensor_t press;          /*!< Polymorphic Pressure Interface */

    cfn_sal_combined_state_t  combined_state; /*!< Standard Framework Shared State */

    int32_t                   t_fine;         /*!< BME280 internal temperature calibration state */
} cfn_sal_bme280_t;

/**
 * @brief BME280 monolithic constructor.
 * Initializes all three polymorphic interfaces and links them to the shared physical mapping.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @param phy    Pointer to the physical interface mapping (I2C/SPI).
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_bme280_construct(cfn_sal_bme280_t *sensor, const cfn_sal_phy_t *phy);

/**
 * @brief BME280 destructor.
 * Safely tears down all three interfaces.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_bme280_destruct(cfn_sal_bme280_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_BME280_H */
