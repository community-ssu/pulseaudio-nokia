#include <string.h>

#include "eap_wfir_dummy_int32.h"

void
EAP_WfirDummyInt32_Init(EAP_WfirInt32 *fir, int sampleRate)
{
  fir->warpingShift = 0;
}

void
EAP_WfirDummyInt32_Process(EAP_WfirInt32 *fir,
                           int32 *const *leftChannel,
                           int32 *const *rightChannel,
                           int32 *unk1, int32 *unk2, const int32 *unk3,
                           const int32 *unk4, const int32 *unk5,
                           const int32 *unk6, int sampleCount)
{
  sampleCount *= sizeof(int32);

  if (leftChannel[0] != unk3)
    memcpy(leftChannel[0], unk3, sampleCount);

  if ( *(const int32 **)rightChannel != unk4 )
    memcpy(*rightChannel, unk4, sampleCount);

  if ( unk5 != unk1 )
    memcpy(unk1, unk5, sampleCount);

  if ( unk6 != unk2 )
    memcpy(unk2, unk6, sampleCount);
}

