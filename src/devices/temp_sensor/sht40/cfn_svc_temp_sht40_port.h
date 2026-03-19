#ifndef CFN_SVC_TEMP_SHT40_CONSTRUCT_H
#define CFN_SVC_TEMP_SHT40_CONSTRUCT_H

#include "devices/cfn_svc_temp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_temp_sht40_construct(cfn_svc_temp_sensor_t *driver, const cfn_svc_temp_config_t *config, const cfn_svc_temp_phy_t *phy);
cfn_hal_error_code_t cfn_svc_temp_sht40_destruct(cfn_svc_temp_sensor_t *driver);

#ifdef __cplusplus
}
#endif

#endif
