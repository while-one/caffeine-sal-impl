#ifndef CFN_SAL_CLI_CONSTRUCT_H
#define CFN_SAL_CLI_CONSTRUCT_H

#include "utilities/cfn_sal_cli.h"

#ifdef __cplusplus
extern "C"
{
#endif

cfn_hal_error_code_t cfn_sal_cli_construct(cfn_sal_cli_t              *driver,
                                           const cfn_sal_cli_config_t *config,
                                           const cfn_sal_phy_t        *phy,
                                           cfn_sal_cli_callback_t      callback,
                                           void                       *user_arg);
cfn_hal_error_code_t cfn_sal_cli_destruct(cfn_sal_cli_t *driver);

#ifdef __cplusplus
}
#endif

#endif
