#ifndef CFN_SAL_COLLECTION_LINKEDLIST_CONSTRUCT_H
#define CFN_SAL_COLLECTION_LINKEDLIST_CONSTRUCT_H

#include "utilities/cfn_sal_collection.h"

#ifdef __cplusplus
extern "C"
{
#endif

cfn_hal_error_code_t cfn_sal_collection_linkedlist_construct(cfn_sal_collection_t              *driver,
                                                             const cfn_sal_collection_config_t *config,
                                                             const cfn_sal_phy_t               *phy,
                                                             cfn_sal_collection_callback_t      callback,
                                                             void                              *user_arg);
cfn_hal_error_code_t cfn_sal_collection_linkedlist_destruct(cfn_sal_collection_t *driver);

#ifdef __cplusplus
}
#endif

#endif
