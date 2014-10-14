#ifndef EAP_WFIR_INT32_H
#define EAP_WFIR_INT32_H

#include "eap_data_types.h"

struct _EAP_WfirInt32
{
  int warpingShift;
};
typedef struct _EAP_WfirInt32 EAP_WfirInt32;

typedef void (*EAP_WfirInt32_ProcessFptr)(EAP_WfirInt32 *, int32 *const *,
                                          int32 *const *, int32 *, int32 *,
                                          const int32 *, const int32 *,
                                          const int32 *, const int32 *, int);
typedef void (*EAP_WfirInt32_InitFptr)(EAP_WfirInt32 *, int);

#endif // EAP_WFIR_INT32_H
