#ifndef EAP_WFIR_THREE_BANDS_INT32_H
#define EAP_WFIR_THREE_BANDS_INT32_H

#include "eap_wfir_int32.h"

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

#endif // EAP_WFIR_THREE_BANDS_INT32_H
