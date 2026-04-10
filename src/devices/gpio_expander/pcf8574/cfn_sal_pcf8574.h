/**
 * @file cfn_sal_pcf8574.h
 * @brief PCF8574 I2C GPIO Expander SAL implementation.
 *
 * This driver implements the standard cfn_hal_gpio_api_t VMT, allowing the
 * expander's pins to be used transparently as standard HAL GPIO handles.
 */

#ifndef CFN_SAL_PCF8574_H
#define CFN_SAL_PCF8574_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ---------------------------------------------------------*/
#include "cfn_hal_gpio.h"
#include "cfn_sal.h"

/* Defines ----------------------------------------------------------*/

#define CFN_SAL_PCF8574_ADDR_DEFAULT 0x20

/* Types Enums ------------------------------------------------------*/

/* Types Structs ----------------------------------------------------*/

/**
 * @brief PCF8574 driver instance structure.
 */
typedef struct
{
    cfn_hal_gpio_t hal_gpio;            /*!< Polymorphic HAL GPIO Interface */
    cfn_sal_phy_t  phy;                 /*!< I2C Physical Interface */
    uint8_t        port_output_shadow;  /*!< Software shadow of the output state */
    uint8_t        port_direction_mask; /*!< 1 = Input (High impedance), 0 = Output */
} cfn_sal_pcf8574_t;

/* Functions prototypes ---------------------------------------------*/

/**
 * @brief Constructs the PCF8574 driver.
 * @param driver Pointer to the driver structure.
 * @param phy Pointer to the I2C physical interface.
 * @param dependency Pointer to an optional dependency.
 * @param callback Optional event callback.
 * @param user_arg Optional user-defined argument for the callback.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_pcf8574_construct(
    cfn_sal_pcf8574_t *driver, const cfn_sal_phy_t *phy, void *dependency, cfn_hal_callback_t callback, void *user_arg);

/**
 * @brief Destructs the PCF8574 driver.
 * @param driver Pointer to the driver structure.
 * @return CFN_HAL_ERROR_OK on success.
 */
cfn_hal_error_code_t cfn_sal_pcf8574_destruct(cfn_sal_pcf8574_t *driver);

#ifdef __cplusplus
}
#endif

#endif /* CFN_SAL_PCF8574_H */
