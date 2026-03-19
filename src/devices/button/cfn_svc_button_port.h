#ifndef CFN_SVC_BUTTON_CONSTRUCT_H
#define CFN_SVC_BUTTON_CONSTRUCT_H

#include "devices/cfn_svc_button.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_button_construct(cfn_svc_button_t *driver, const cfn_svc_button_config_t *config, const cfn_svc_button_phy_t *phy);
cfn_hal_error_code_t cfn_svc_button_destruct(cfn_svc_button_t *driver);

#ifdef __cplusplus
}
#endif

#endif
