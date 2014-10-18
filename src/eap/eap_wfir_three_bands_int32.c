#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_three_bands_int32.h"

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
EAP_WfirThreeBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirThreeBandsInt32 *inst = (EAP_WfirThreeBandsInt32 *)instance;
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 3;

  for (i = 0; i < 7; ++i)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  inst->m_leftCompMem[0] = 0;
  inst->m_leftCompMem[1] = 0;
  inst->m_leftCompMem[2] = 0;
  inst->m_leftCompMem[3] = 0;
  inst->m_rightCompMem[0] = 0;
  inst->m_rightCompMem[1] = 0;
  inst->m_rightCompMem[2] = 0;
  inst->m_rightCompMem[3] = 0;
}

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
                                int frames)
{
  EAP_WfirThreeBandsInt32 *inst = (EAP_WfirThreeBandsInt32 *)instance;
  int warpingShift = inst->common.warpingShift;
  int32 *leftMem = inst->m_leftMemory;
  int32 *rightMem = inst->m_rightMemory;
  int32 *ptr_w_left = inst->common.w_left;
  int32 *ptr_w_right = inst->common.w_right;
  int i = 0;

  while (i < frames)
  {
    EAP_WarpedDelayLineInt32_Process(warpingShift,
                                     leftMem,
                                     7,
                                     leftLowInput[i]);
    EAP_WarpedDelayLineInt32_Process(warpingShift,
                                     rightMem,
                                     7,
                                     rightLowInput[i]);

    ptr_w_left[0] = leftMem[3];
    ptr_w_left[4] = leftMem[2] + leftMem[4];
    ptr_w_left[8] = leftMem[5] + leftMem[1];
    ptr_w_left[12] = leftMem[0] + leftMem[6];

    ptr_w_right[0] = rightMem[3];
    ptr_w_right[4] = rightMem[4] + rightMem[2];
    ptr_w_right[8] = rightMem[5] + rightMem[1];
    ptr_w_right[12] = rightMem[6] + rightMem[0];

    ptr_w_left ++;
    ptr_w_right ++;
    i ++;

    if (!(i % 4))
    {
      ptr_w_left += 12;
      ptr_w_right += 12;
    }
  }

  EAP_WfirThreeBandsInt32_filter(inst->common.w_left,
                                 inst->common.w_right,
                                 leftLowOutputBuffers,
                                 rightLowOutputBuffers);

  for (i = 0; i < frames; i ++)
  {
    EAP_WarpedDelayLineInt32_Process(warpingShift,
                                     inst->m_leftCompMem,
                                     4,
                                     leftHighInput[i]);
    leftHighOutput[i] = inst->m_leftCompMem[3];

    EAP_WarpedDelayLineInt32_Process(warpingShift,
                                     inst->m_rightCompMem,
                                     4,
                                     rightHighInput[i]);
    rightHighOutput[i] = inst->m_rightCompMem[3];
  }
}
