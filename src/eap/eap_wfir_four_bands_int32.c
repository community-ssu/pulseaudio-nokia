#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_four_bands_int32.h"
#include "eap_warped_delay_line_int32.h"

struct _EAP_WfirFourBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_warpingShift2;
  int32 m_leftMemory[11];
  int32 m_rightMemory[11];
  int32 m_leftCompMem[6];
  int32 m_rightCompMem[6];
};

typedef struct _EAP_WfirFourBandsInt32 EAP_WfirFourBandsInt32;

void
EAP_WfirFourBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirFourBandsInt32 *inst = (EAP_WfirFourBandsInt32 *)instance;
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 3;
  inst->m_warpingShift2 = 4;

  for (i = 0; i < 11; i ++)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  for (i = 0; i < 6; i ++)
  {
    inst->m_leftCompMem[i] = 0;
    inst->m_rightCompMem[i] = 0;
  }
}

void
EAP_WfirFourBandsInt32_Process(EAP_WfirInt32 *instance,
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
  EAP_WfirFourBandsInt32 *inst = (EAP_WfirFourBandsInt32 *)instance;
  int warpingShift1 = inst->common.warpingShift;
  int warpingShift2 = inst->m_warpingShift2;
  int32 *leftMem = inst->m_leftMemory;
  int32 *rightMem = inst->m_rightMemory;
  int32 *ptr_w_left = inst->common.w_left;
  int32 *ptr_w_right = inst->common.w_right;
  int i = 0;

  while (i < frames)
  {
    EAP_WarpedDelayLineInt32_Process2(warpingShift1,
                                      warpingShift2,
                                      leftMem,
                                      11,
                                      leftLowInput[i]);
    EAP_WarpedDelayLineInt32_Process2(warpingShift1,
                                      warpingShift2,
                                      rightMem,
                                      11,
                                      rightLowInput[i]);
    ptr_w_left[0] = leftMem[5];
    ptr_w_left[4] = leftMem[4] + leftMem[6];
    ptr_w_left[8] = leftMem[3] + leftMem[7];
    ptr_w_left[12] = leftMem[2] + leftMem[8];
    ptr_w_left[16] = leftMem[1] + leftMem[9];
    ptr_w_left[20] = leftMem[0] + leftMem[10];

    ptr_w_right[0] = rightMem[5];
    ptr_w_right[4] = rightMem[4] + rightMem[6];
    ptr_w_right[8] = rightMem[3] + rightMem[7];
    ptr_w_right[12] = rightMem[2] + rightMem[8];
    ptr_w_right[16] = rightMem[1] + rightMem[9];
    ptr_w_right[20] = rightMem[0] + rightMem[10];

    ptr_w_left ++;
    ptr_w_right ++;
    i ++;

    if (!(i % 4))
    {
      ptr_w_left += 20;
      ptr_w_right += 20;
    }
  }

  EAP_WfirFourBandsInt32_filter(inst->common.w_left,
                                inst->common.w_right,
                                leftLowOutputBuffers,
                                rightLowOutputBuffers);

  for (i = 0; i < frames; i ++)
  {
    EAP_WarpedDelayLineInt32_Process2(warpingShift1,
                                      warpingShift2,
                                      inst->m_leftCompMem,
                                      6,
                                      leftHighInput[i]);
    leftHighOutput[i] = inst->m_leftCompMem[5];

    EAP_WarpedDelayLineInt32_Process2(warpingShift1,
                                      warpingShift2,
                                      inst->m_rightCompMem,
                                      6,
                                      rightHighInput[i]);
    rightHighOutput[i] = inst->m_rightCompMem[5];
  }
}
