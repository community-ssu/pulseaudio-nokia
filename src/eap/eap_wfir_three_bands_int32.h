#ifndef EAP_WFIR_THREE_BANDS_INT32_H
#define EAP_WFIR_THREE_BANDS_INT32_H

#include "eap_wfir_int32.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _EAP_WfirThreeBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_leftMemory[7];
  int32 m_rightMemory[7];
  int32 m_leftCompMem[4];
  int32 m_rightCompMem[4];
};

typedef struct _EAP_WfirThreeBandsInt32 EAP_WfirThreeBandsInt32;

void
EAP_WfirThreeBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate);

void
EAP_WfirThreeBandsInt32_Process(EAP_WfirInt32 *instance,
                                int32 *const *leftLowOutputBuffers,
                                int32 *const *rightLowOutputBuffers,
                                int32 *leftHighOutput,
                                int32 *rightHighOutput,
                                const int32 *leftLowInput,
                                const int32 *rightLowInput,
                                const int32 *leftHighInput,
                                const int32 *rightHighInput,
                                int frames);

void
EAP_WfirThreeBandsInt32_filter(int32 *w_left, int32 *w_right,
                               int32 *const *leftLowOutputBuffers,
                               int32 *const *rightLowOutputBuffers,
                               int sample_count);

#ifdef __cplusplus
}
#endif

#endif // EAP_WFIR_THREE_BANDS_INT32_H
