#ifndef CFN_SAL_TEMP_SHT30_CONSTRUCT_H
#define CFN_SAL_TEMP_SHT30_CONSTRUCT_H

#include "devices/cfn_sal_temp_sensor.h"

#ifdef __cplusplus
extern "C"
{
#endif

cfn_hal_error_code_t cfn_sal_temp_sht30_construct(cfn_sal_temp_sensor_t       *driver,
                                                  const cfn_sal_temp_config_t *config,
                                                  const cfn_sal_phy_t         *phy,
                                                  cfn_sal_temp_callback_t      callback,
                                                  void                        *user_arg);
cfn_hal_error_code_t cfn_sal_temp_sht30_destruct(cfn_sal_temp_sensor_t *driver);

#ifdef __cplusplus
}
#endif

#endif
