#ifndef EAP_WFIR_TWO_BANDS_INT32_H
#define EAP_WFIR_TWO_BANDS_INT32_H

#include "eap_wfir_int32.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _EAP_WfirTwoBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_leftMemory[3];
  int32 m_rightMemory[3];
  int32 m_leftCompMem[2];
  int32 m_rightCompMem[2];
};

typedef struct _EAP_WfirTwoBandsInt32 EAP_WfirTwoBandsInt32;

void
EAP_WfirTwoBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate);

void
EAP_WfirTwoBandsInt32_filter(int *fl, int *fr, int32 *const *l, int32 *const *r,
                             int samplesCount);

void EAP_WfirTwoBandsInt32_Process(EAP_WfirInt32 *instance,
                                   int32 *const *leftLowOutputBuffers,
                                   int32 *const *rightLowOutputBuffers,
                                   int32 *leftHighOutput,
                                   int32 *rightHighOutput,
                                   const int32 *leftLowInput,
                                   const int32 *rightLowInput,
                                   const int32 *leftHighInput,
                                   const int32 *rightHighInput,
                                   int32 frames);

#ifdef __cplusplus
}
#endif

#endif // EAP_WFIR_TWO_BANDS_INT32_H
