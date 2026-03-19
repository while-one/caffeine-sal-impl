#ifndef CFN_SVC_LED_CONSTRUCT_H
#define CFN_SVC_LED_CONSTRUCT_H

#include "devices/cfn_svc_led.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_led_construct(cfn_svc_led_t *driver, const cfn_svc_led_config_t *config, const cfn_svc_led_phy_t *phy);
cfn_hal_error_code_t cfn_svc_led_destruct(cfn_svc_led_t *driver);

#ifdef __cplusplus
}
#endif

#endif
