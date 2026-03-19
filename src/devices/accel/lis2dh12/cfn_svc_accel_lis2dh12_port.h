#ifndef CFN_SVC_ACCEL_LIS2DH12_CONSTRUCT_H
#define CFN_SVC_ACCEL_LIS2DH12_CONSTRUCT_H

#include "devices/cfn_svc_accel.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_accel_lis2dh12_construct(cfn_svc_accel_t *driver, const cfn_svc_accel_config_t *config, const cfn_svc_accel_phy_t *phy);
cfn_hal_error_code_t cfn_svc_accel_lis2dh12_destruct(cfn_svc_accel_t *driver);

#ifdef __cplusplus
}
#endif

#endif
