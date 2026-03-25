#ifndef CFN_SAL_ACCEL_LIS2DH12_CONSTRUCT_H
#define CFN_SAL_ACCEL_LIS2DH12_CONSTRUCT_H

#include "devices/cfn_sal_accel.h"

#ifdef __cplusplus
extern "C"
{
#endif

cfn_hal_error_code_t cfn_sal_accel_lis2dh12_construct(cfn_sal_accel_t              *driver,
                                                      const cfn_sal_accel_config_t *config,
                                                      const cfn_sal_phy_t          *phy,
                                                      cfn_sal_accel_callback_t      callback,
                                                      void                         *user_arg);
cfn_hal_error_code_t cfn_sal_accel_lis2dh12_destruct(cfn_sal_accel_t *driver);

#ifdef __cplusplus
}
#endif

#endif
