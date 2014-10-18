#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_five_bands_int32.h"
#include "eap_warped_delay_line_int32.h"

struct _EAP_WfirFiveBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_leftMemory[15];
  int32 m_rightMemory[15];
  int32 m_leftCompMem[8];
  int32 m_rightCompMem[8];
};

typedef struct _EAP_WfirFiveBandsInt32 EAP_WfirFiveBandsInt32;

void
EAP_WfirFiveBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirFiveBandsInt32 *inst = (EAP_WfirFiveBandsInt32 *)instance;
  int i = 0;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 2;

  for (i = 0; i < 15; i ++)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  for (i = 0; i < 8; ++i)
  {
    inst->m_leftCompMem[i] = 0;
    inst->m_rightCompMem[i] = 0;
  }
}

void
EAP_WfirFiveBandsInt32_Process(EAP_WfirInt32 *instance,
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
  EAP_WfirFiveBandsInt32 *inst = (EAP_WfirFiveBandsInt32 *)instance;
  int warpingShift = inst->common.warpingShift;
  int32 *leftMem = inst->m_leftMemory;
  int32 *rightMem = inst->m_rightMemory;
  int32 *ptr_w_left = inst->common.w_left;
  int32 *ptr_w_right = inst->common.w_right;
  int i = 0;

  while (i < frames)
  {
    EAP_WarpedDelayLineInt32_Process(
          warpingShift, leftMem, 15, leftLowInput[i]);
    EAP_WarpedDelayLineInt32_Process(
          warpingShift, rightMem, 15, rightLowInput[i]);

    ptr_w_left[0] = leftMem[7];
    ptr_w_left[4] = leftMem[6] + leftMem[8];
    ptr_w_left[8] = leftMem[5] + leftMem[9];
    ptr_w_left[12] = leftMem[10] + leftMem[4];
    ptr_w_left[16] = leftMem[11] + leftMem[3];
    ptr_w_left[20] = leftMem[12] + leftMem[2];
    ptr_w_left[24] = leftMem[13] + leftMem[1];
    ptr_w_left[28] = leftMem[14] + leftMem[0];

    ptr_w_right[0] = rightMem[7];
    ptr_w_right[4] = rightMem[6] + rightMem[8];
    ptr_w_right[8] = rightMem[5] + rightMem[9];
    ptr_w_right[12] = rightMem[4] + rightMem[10];
    ptr_w_right[16] = rightMem[3] + rightMem[11];
    ptr_w_right[20] = rightMem[2] + rightMem[12];
    ptr_w_right[24] = rightMem[1] + rightMem[13];
    ptr_w_right[28] = rightMem[0] + rightMem[14];

    ptr_w_left ++;
    ptr_w_right ++;
    i ++;

    if (!(i % 4))
    {
      ptr_w_left += 28;
      ptr_w_right += 28;
    }
  }

  EAP_WfirFiveBandsInt32_filter(inst->common.w_left,
                                inst->common.w_right,
                                leftLowOutputBuffers,
                                rightLowOutputBuffers);

  for (i = 0; i < frames; i ++)
  {
    EAP_WarpedDelayLineInt32_Process(warpingShift,
                                     inst->m_leftCompMem,
                                     8,
                                     leftHighInput[i]);
    leftHighOutput[i] = inst->m_leftCompMem[7];

    EAP_WarpedDelayLineInt32_Process(warpingShift,
                                     inst->m_rightCompMem,
                                     8,
                                     rightHighInput[i]);
    rightHighOutput[i] = inst->m_rightCompMem[7];
  }
}

