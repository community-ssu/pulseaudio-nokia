#include <string.h>

#include "eap_wfir_dummy_int32.h"

struct EAP_WfirDummyFloat
{
  EAP_WfirInt32 common;
};

typedef struct EAP_WfirDummyFloat EAP_WfirDummyInt32;

void
EAP_WfirDummyInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  ((EAP_WfirDummyInt32 *)instance)->common.warpingShift = 0;
}

void
EAP_WfirDummyInt32_Process(EAP_WfirInt32 *instance,
                           int32 *const *leftLowOutputBuffers,
                           int32 *const *rightLowOutputBuffers,
                           int32 *leftHighOutput,
                           int32 *rightHighOutput,
                           const int32 *leftLowInput,
                           const int32 *rightLowInput,
                           const int32 *leftHighInput,
                           const int32 *rightHighInput,
                           int frames)
{
  frames *= 4;

  if (leftLowOutputBuffers[0] != leftLowInput )
    memcpy(leftLowOutputBuffers[0], leftLowInput, frames);

  if (rightLowOutputBuffers[0] != rightLowInput )
    memcpy(rightLowOutputBuffers[0], rightLowInput, frames);

  if ( leftHighInput != leftHighOutput )
    memcpy(leftHighOutput, leftHighInput, frames);

  if ( rightHighInput != rightHighOutput )
    memcpy(rightHighOutput, rightHighInput, frames);
}
