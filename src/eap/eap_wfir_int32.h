#ifndef EAP_WFIR_INT32_H
#define EAP_WFIR_INT32_H

#include "eap_data_types.h"

struct _EAP_WfirInt32
{
  int warpingShift;
  int32 w_left[1920];
  int32 w_right[1920];
};

typedef struct _EAP_WfirInt32 EAP_WfirInt32;

typedef void (*EAP_WfirInt32_ProcessFptr)(EAP_WfirInt32 *, int32 *const *,
                                          int32 *const *, int32 *, int32 *,
                                          const int32 *, const int32 *,
                                          const int32 *, const int32 *, int32);
typedef void (*EAP_WfirInt32_InitFptr)(EAP_WfirInt32 *, int32);

#endif // EAP_WFIR_INT32_H
