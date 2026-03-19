#ifndef CFN_SVC_ACCEL_BMA530_CONSTRUCT_H
#define CFN_SVC_ACCEL_BMA530_CONSTRUCT_H

#include "devices/cfn_svc_accel.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_accel_bma530_construct(cfn_svc_accel_t *driver, const cfn_svc_accel_config_t *config, const cfn_svc_accel_phy_t *phy);
cfn_hal_error_code_t cfn_svc_accel_bma530_destruct(cfn_svc_accel_t *driver);

#ifdef __cplusplus
}
#endif

#endif
