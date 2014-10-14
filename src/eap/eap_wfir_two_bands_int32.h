#ifndef EAP_WFIR_TWO_BANDS_INT32_H
#define EAP_WFIR_TWO_BANDS_INT32_H

#include "eap_wfir_int32.h"

void
EAP_WfirTwoBandsInt32_Init(EAP_WfirInt32 *fir, int sampleRate);

void
EAP_WfirTwoBandsInt32_filter(int *fl, int *fr, int32 *const *l, int32 *const *r,
                             int samplesCount);
void
EAP_WfirTwoBandsInt32_Process(EAP_WfirInt32 *fir, int32 *const *leftChannel,
                              int32 *const *rightChannel, int32 *unk1,
                              int32 *unk2, const int32 *unk3,
                              const int32 *unk4, const int32 *unk5,
                              const int32 *unk6, int sampleCount);

#endif // EAP_WFIR_TWO_BANDS_INT32_H
