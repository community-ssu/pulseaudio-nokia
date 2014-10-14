#ifndef EAP_WFIR_DUMMY_INT32_H
#define EAP_WFIR_DUMMY_INT32_H

#include "eap_wfir_int32.h"

void
EAP_WfirDummyInt32_Init(EAP_WfirInt32 *fir, int sampleRate);

void
EAP_WfirDummyInt32_Process(EAP_WfirInt32 *fir,
                           int32 *const *leftChannel,
                           int32 *const *rightChannel,
                           int32 *unk1, int32 *unk2, const int32 *unk3,
                           const int32 *unk4, const int32 *unk5,
                           const int32 *unk6, int sampleCount);

#endif // EAP_WFIR_DUMMY_INT32_H
