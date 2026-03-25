/**
 * @file compliance.c
 * @brief Rigorous C file to verify C11 compliance for Service Implementations.
 */

#include "devices/combined_sensor/bme280/cfn_sal_bme280.h"
#include "devices/temp_sensor/sht30/cfn_sal_temp_sht30_port.h"
#include "devices/led/cfn_sal_led_port.h"
#include <stddef.h>

int main(void)
{
    /* Verify BME280 construction */
    cfn_sal_temp_sensor_t temp_driver;
    cfn_sal_bme280_ctx_t bme_ctx = {0};
    cfn_sal_phy_t phy = { .user_arg = &bme_ctx, .handle = NULL };
    
    cfn_hal_error_code_t err = cfn_sal_bme280_temp_construct(&temp_driver, NULL, &phy, NULL, NULL);
    (void)err;

    return 0;
}
