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
 * @brief BME280 shared context.
 * Must be allocated by the user and passed to the generic PHY's user_arg.
 */
typedef struct
{
    cfn_sal_shared_ctx_t base;   /*!< Must be first member for shared ref-counting */
    int32_t              t_fine; /*!< BME280 internal temperature calibration state */
    bool                 hw_initialized;
    uint32_t             last_read_ticks;
} cfn_sal_bme280_ctx_t;

/**
 * @brief BME280 constructor for the Temperature Sensor interface.
 */
cfn_hal_error_code_t cfn_sal_bme280_temp_construct(cfn_sal_temp_sensor_t       *driver,
                                                   const cfn_sal_temp_config_t *config,
                                                   const cfn_sal_phy_t         *phy,
                                                   cfn_sal_temp_callback_t      callback,
                                                   void                        *user_arg);

/**
 * @brief BME280 constructor for the Humidity Sensor interface.
 */
cfn_hal_error_code_t cfn_sal_bme280_hum_construct(cfn_sal_hum_sensor_t       *driver,
                                                  const cfn_sal_hum_config_t *config,
                                                  const cfn_sal_phy_t        *phy,
                                                  cfn_sal_hum_callback_t      callback,
                                                  void                       *user_arg);

/**
 * @brief BME280 constructor for the Pressure Sensor interface.
 */
cfn_hal_error_code_t cfn_sal_bme280_pressure_construct(cfn_sal_pressure_sensor_t       *driver,
                                                       const cfn_sal_pressure_config_t *config,
                                                       const cfn_sal_phy_t             *phy,
                                                       cfn_sal_pressure_callback_t      callback,
                                                       void                            *user_arg);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_BME280_H */
