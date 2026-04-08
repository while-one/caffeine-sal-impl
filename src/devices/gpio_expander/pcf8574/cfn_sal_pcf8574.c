/**
 * @file cfn_sal_pcf8574.c
 * @brief PCF8574 I2C GPIO Expander SAL implementation.
 */

#include "cfn_sal_pcf8574.h"
#include "cfn_hal_i2c.h"

/* Private Functions ------------------------------------------------*/

static cfn_hal_error_code_t pcf8574_write_port(cfn_sal_pcf8574_t *driver, uint8_t data)
{
    cfn_hal_i2c_device_t     *dev = (cfn_hal_i2c_device_t *) driver->phy.instance;
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = &data,
                                      .nbr_of_tx_bytes = 1,
                                      .rx_payload      = NULL,
                                      .nbr_of_rx_bytes = 0 };
    return cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
}

static cfn_hal_error_code_t pcf8574_read_port(cfn_sal_pcf8574_t *driver, uint8_t *data)
{
    cfn_hal_i2c_device_t     *dev = (cfn_hal_i2c_device_t *) driver->phy.instance;
    cfn_hal_i2c_transaction_t xfr = { .slave_address   = dev->address,
                                      .tx_payload      = NULL,
                                      .nbr_of_tx_bytes = 0,
                                      .rx_payload      = data,
                                      .nbr_of_rx_bytes = 1 };
    return cfn_hal_i2c_xfr_polling(dev->i2c, &xfr, 100);
}

/* VMT Implementations ----------------------------------------------*/

static cfn_hal_error_code_t pcf8574_init(cfn_hal_gpio_t *driver)
{
    cfn_sal_pcf8574_t *pcf   = CFN_HAL_CONTAINER_OF(driver, cfn_sal_pcf8574_t, hal_gpio);

    /* Initial state: All pins as inputs (high-Z) per PCF8574 datasheet */
    pcf->port_output_shadow  = 0xFF;
    pcf->port_direction_mask = 0xFF;

    return pcf8574_write_port(pcf, pcf->port_output_shadow);
}

static cfn_hal_error_code_t pcf8574_deinit(cfn_hal_gpio_t *driver)
{
    CFN_HAL_UNUSED(driver);
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t pcf8574_pin_config(cfn_hal_gpio_t *driver, const cfn_hal_gpio_pin_config_t *config)
{
    cfn_sal_pcf8574_t *pcf = CFN_HAL_CONTAINER_OF(driver, cfn_sal_pcf8574_t, hal_gpio);

    for (uint8_t i = 0; i < 8; ++i)
    {
        if (config->pin_mask & CFN_HAL_BIT(i))
        {
            if (config->mode == CFN_HAL_GPIO_CONFIG_MODE_INPUT)
            {
                pcf->port_direction_mask |= CFN_HAL_BIT(i);
                pcf->port_output_shadow |= CFN_HAL_BIT(i); /* PCF8574: Write 1 for input mode */
            }
            else
            {
                pcf->port_direction_mask &= ~CFN_HAL_BIT(i);
            }
        }
    }

    return pcf8574_write_port(pcf, pcf->port_output_shadow);
}

static cfn_hal_error_code_t pcf8574_pin_write(cfn_hal_gpio_t *driver, uint32_t pin_mask, cfn_hal_gpio_state_t state)
{
    cfn_sal_pcf8574_t *pcf = CFN_HAL_CONTAINER_OF(driver, cfn_sal_pcf8574_t, hal_gpio);

    for (uint8_t i = 0; i < 8; ++i)
    {
        if (pin_mask & CFN_HAL_BIT(i))
        {
            /* Check if pin is configured as output */
            if (pcf->port_direction_mask & CFN_HAL_BIT(i))
            {
                return CFN_HAL_ERROR_BAD_PARAM;
            }

            if (state == CFN_HAL_GPIO_STATE_HIGH)
            {
                pcf->port_output_shadow |= CFN_HAL_BIT(i);
            }
            else
            {
                pcf->port_output_shadow &= ~CFN_HAL_BIT(i);
            }
        }
    }

    return pcf8574_write_port(pcf, pcf->port_output_shadow);
}

static cfn_hal_error_code_t pcf8574_pin_read(cfn_hal_gpio_t *driver, uint32_t pin_mask, cfn_hal_gpio_state_t *state)
{
    cfn_sal_pcf8574_t   *pcf = CFN_HAL_CONTAINER_OF(driver, cfn_sal_pcf8574_t, hal_gpio);
    uint8_t              raw_data;
    cfn_hal_error_code_t error = pcf8574_read_port(pcf, &raw_data);

    if (error == CFN_HAL_ERROR_OK && state)
    {
        *state = (raw_data & pin_mask) ? CFN_HAL_GPIO_STATE_HIGH : CFN_HAL_GPIO_STATE_LOW;
    }

    return error;
}

static cfn_hal_error_code_t pcf8574_pin_toggle(cfn_hal_gpio_t *driver, uint32_t pin_mask)
{
    cfn_sal_pcf8574_t *pcf = CFN_HAL_CONTAINER_OF(driver, cfn_sal_pcf8574_t, hal_gpio);

    /* PCF8574 cannot toggle inputs safely without knowing current state,
     * so we only toggle outputs from shadow. */
    if (pcf->port_direction_mask & pin_mask)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    pcf->port_output_shadow ^= (uint8_t) pin_mask;
    return pcf8574_write_port(pcf, pcf->port_output_shadow);
}

static const cfn_hal_gpio_api_t PCF8574_API = {
    .base = {
        .init   = (cfn_hal_error_code_t (*)(cfn_hal_driver_t *))pcf8574_init,
        .deinit = (cfn_hal_error_code_t (*)(cfn_hal_driver_t *))pcf8574_deinit,
    },
    .pin_config = pcf8574_pin_config,
    .pin_write  = pcf8574_pin_write,
    .pin_read   = pcf8574_pin_read,
    .pin_toggle = pcf8574_pin_toggle,
};

/* Public Functions -------------------------------------------------*/

cfn_hal_error_code_t cfn_sal_pcf8574_construct(cfn_sal_pcf8574_t *driver, const cfn_sal_phy_t *phy)
{
    if (!driver || !phy)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    driver->phy = *phy;
    cfn_hal_gpio_populate(&driver->hal_gpio, 0, NULL, &PCF8574_API, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_pcf8574_destruct(cfn_sal_pcf8574_t *driver)
{
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    return cfn_hal_gpio_deinit(&driver->hal_gpio);
}
