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
    cfn_sal_bme280_t bme;
    cfn_sal_phy_t    phy = {.handle = NULL, .instance = NULL, .type = 0, .user_arg = NULL};

    cfn_hal_error_code_t err = cfn_sal_bme280_construct(&bme, &phy);
    (void) err;

    return 0;
}
