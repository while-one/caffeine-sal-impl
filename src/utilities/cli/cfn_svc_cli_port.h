#ifndef CFN_SVC_CLI_CONSTRUCT_H
#define CFN_SVC_CLI_CONSTRUCT_H

#include "utilities/cfn_svc_cli.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_cli_construct(cfn_svc_cli_t *driver, const cfn_svc_cli_config_t *config, const cfn_svc_cli_phy_t *phy);
cfn_hal_error_code_t cfn_svc_cli_destruct(cfn_svc_cli_t *driver);

#ifdef __cplusplus
}
#endif

#endif
