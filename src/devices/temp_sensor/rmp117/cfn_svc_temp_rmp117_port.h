#ifndef CFN_SVC_TEMP_RMP117_CONSTRUCT_H
#define CFN_SVC_TEMP_RMP117_CONSTRUCT_H

#include "devices/cfn_svc_temp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_temp_rmp117_construct(cfn_svc_temp_sensor_t *driver, const cfn_svc_temp_config_t *config, const cfn_svc_temp_phy_t *phy);
cfn_hal_error_code_t cfn_svc_temp_rmp117_destruct(cfn_svc_temp_sensor_t *driver);

#ifdef __cplusplus
}
#endif

#endif
