#ifndef CFN_SVC_COLLECTION_LINKEDLIST_CONSTRUCT_H
#define CFN_SVC_COLLECTION_LINKEDLIST_CONSTRUCT_H

#include "utilities/cfn_svc_collection.h"

#ifdef __cplusplus
extern "C" {
#endif

cfn_hal_error_code_t cfn_svc_collection_linkedlist_construct(cfn_svc_collection_t *driver, const cfn_svc_collection_config_t *config, const cfn_svc_collection_phy_t *phy);
cfn_hal_error_code_t cfn_svc_collection_linkedlist_destruct(cfn_svc_collection_t *driver);

#ifdef __cplusplus
}
#endif

#endif
