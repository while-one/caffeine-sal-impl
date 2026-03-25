#ifndef CFN_SAL_LED_CONSTRUCT_H
#define CFN_SAL_LED_CONSTRUCT_H

#include "devices/cfn_sal_led.h"

#ifdef __cplusplus
extern "C"
{
#endif

cfn_hal_error_code_t cfn_sal_led_construct(cfn_sal_led_t              *driver,
                                           const cfn_sal_led_config_t *config,
                                           const cfn_sal_phy_t        *phy,
                                           cfn_sal_led_callback_t      callback,
                                           void                       *user_arg);
cfn_hal_error_code_t cfn_sal_led_destruct(cfn_sal_led_t *driver);

#ifdef __cplusplus
}
#endif

#endif
