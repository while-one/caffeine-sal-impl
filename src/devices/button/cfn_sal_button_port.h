#ifndef CFN_SAL_BUTTON_CONSTRUCT_H
#define CFN_SAL_BUTTON_CONSTRUCT_H

#include "devices/cfn_sal_button.h"

#ifdef __cplusplus
extern "C"
{
#endif

cfn_hal_error_code_t cfn_sal_button_construct(cfn_sal_button_t              *driver,
                                              const cfn_sal_button_config_t *config,
                                              const cfn_sal_phy_t           *phy,
                                              cfn_sal_button_callback_t      callback,
                                              void                          *user_arg);
cfn_hal_error_code_t cfn_sal_button_destruct(cfn_sal_button_t *driver);

#ifdef __cplusplus
}
#endif

#endif
