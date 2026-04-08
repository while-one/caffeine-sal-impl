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
#include "utilities/cfn_sal_timekeeping.h"

/* -------------------------------------------------------------------------- */
/* Constants                                                                  */
/* -------------------------------------------------------------------------- */

#define CFN_SAL_BME280_ADDR_0 0x76 /*!< SDO pin tied to GND */
#define CFN_SAL_BME280_ADDR_1 0x77 /*!< SDO pin tied to VDD */

#define CFN_SAL_BME280_ADDR_DEFAULT CFN_SAL_BME280_ADDR_0

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief BME280 factory calibration data.
 */
typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;

    int32_t t_fine; /*!< Required for P and H compensation */
} cfn_sal_bme280_calibration_t;

/**
 * @brief BME280 composite sensor structure.
 */
typedef struct
{
    cfn_sal_temp_sensor_t     temp;  /*!< Polymorphic Temperature Interface */
    cfn_sal_hum_sensor_t      hum;   /*!< Polymorphic Humidity Interface */
    cfn_sal_pressure_sensor_t press; /*!< Polymorphic Pressure Interface */

    cfn_sal_combined_state_t combined_state; /*!< Standard Framework Shared State */

    cfn_sal_bme280_calibration_t calib; /*!< Factory trim parameters */

    /* Cached configuration to avoid redundant register writes */
    uint8_t ctrl_hum_reg;
    uint8_t ctrl_meas_reg;
    uint8_t config_reg;

    /* Cached measurement results */
    uint64_t last_read_timestamp_ms;
    float    cached_temp;
    float    cached_hum;
    float    cached_press;
} cfn_sal_bme280_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief BME280 monolithic constructor.
 * Initializes all three polymorphic interfaces and links them to the shared physical mapping.
 *
 * @param sensor      Pointer to the composite sensor structure.
 * @param phy         Pointer to the physical interface mapping.
 * @param time_source Optional pointer to a timekeeping service for caching logic.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t
cfn_sal_bme280_construct(cfn_sal_bme280_t *sensor, const cfn_sal_phy_t *phy, cfn_sal_timekeeping_t *time_source);

/**
 * @brief BME280 destructor.
 *
 * @param sensor Pointer to the composite sensor structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_bme280_destruct(const cfn_sal_bme280_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_BME280_H */
