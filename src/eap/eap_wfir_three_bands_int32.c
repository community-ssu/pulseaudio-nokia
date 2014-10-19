#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_three_bands_int32.h"
#include "eap_warped_delay_line_int32.h"

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

#ifdef __ARM_NEON__
static const int32x2_t h0_opt_1 = {0x20000000, 0x1B400000};
static const int32x2_t h0_opt_2 = {0xFFB0000, 0x4C40000};
static const int32x2_t h1_opt_1 = {0x40000000, 0x1FF60000};

#define PROCESS_CHANNEL(ptr, out) \
  W0 = vld1q_s32(&ptr[0]); \
  W8 = vld1q_s32(&ptr[8]); \
\
  A = vaddq_s32(vqdmulhq_lane_s32 (W0, h0_opt_1, 0), \
                vqdmulhq_lane_s32 (W8, h0_opt_2, 0)); \
\
  vst1q_s32(&out[1][i], \
            vsubq_s32(vqdmulhq_lane_s32 (W0, h1_opt_1, 0), \
                      vqdmulhq_lane_s32 (W8, h1_opt_1, 1))); \
\
  B = vaddq_s32(vqdmulhq_lane_s32 (vld1q_s32(&ptr[4]), h0_opt_1, 1), \
                vqdmulhq_lane_s32 (vld1q_s32(&ptr[12]), h0_opt_2, 1)); \
\
  vst1q_s32(&out[0][i], vaddq_s32(A, B)); \
  vst1q_s32(&out[2][i], vsubq_s32(A, B));

#define PROCESS_CHANNEL_RESIDUE(ptr, out) \
  W0 = vld1q_s32(&ptr[0]); \
  W8 = vld1q_s32(&ptr[8]); \
\
  A = vaddq_s32(vqdmulhq_lane_s32 (W0, h0_opt_1, 0), \
                vqdmulhq_lane_s32 (W8, h0_opt_2, 0)); \
\
  OUT1 = vsubq_s32(vqdmulhq_lane_s32 (W0, h1_opt_1, 0), \
                      vqdmulhq_lane_s32 (W8, h1_opt_1, 1)); \
\
  B = vaddq_s32(vqdmulhq_lane_s32 (vld1q_s32(&ptr[4]), h0_opt_1, 1), \
                vqdmulhq_lane_s32 (vld1q_s32(&ptr[12]), h0_opt_2, 1)); \
\
  OUT0 = vaddq_s32(A, B); \
  OUT2 = vsubq_s32(A, B); \
\
  out[0][i] = vgetq_lane_s32(OUT0, 0); \
  out[1][i] = vgetq_lane_s32(OUT1, 0); \
  out[2][i] = vgetq_lane_s32(OUT2, 0); \
\
  if(residue == 2) \
  { \
    out[0][i + 1] = vgetq_lane_s32(OUT0, 1); \
    out[1][i + 1] = vgetq_lane_s32(OUT1, 1); \
    out[2][i + 1] = vgetq_lane_s32(OUT2, 1); \
  } \
\
  if(residue == 3) \
  { \
    out[0][i + 1] = vgetq_lane_s32(OUT0, 1); \
    out[1][i + 1] = vgetq_lane_s32(OUT1, 1); \
    out[2][i + 1] = vgetq_lane_s32(OUT2, 1); \
    out[0][i + 2] = vgetq_lane_s32(OUT0, 2); \
    out[1][i + 2] = vgetq_lane_s32(OUT1, 2); \
    out[2][i + 2] = vgetq_lane_s32(OUT2, 2); \
  }

void
EAP_WfirThreeBandsInt32_filter(int32 *w_left, int32 *w_right,
                               int32 *const *leftLowOutputBuffers,
                               int32 *const *rightLowOutputBuffers,
                               int sample_count)
{
  int residue = sample_count & 3;
  int i;
  int32x4_t W0, W8, A, B;

  for (i = 0; i < sample_count - residue; i += 4, w_left += 16, w_right += 16)
  {
    PROCESS_CHANNEL(w_left, leftLowOutputBuffers);
    PROCESS_CHANNEL(w_right, rightLowOutputBuffers);
  }

  if (residue)
  {
    int32x4_t OUT0, OUT1, OUT2;
    PROCESS_CHANNEL_RESIDUE(w_left, leftLowOutputBuffers);
    PROCESS_CHANNEL_RESIDUE(w_right, rightLowOutputBuffers);
  }
}
#else
#error TODO - x86 implementation. You need __ARM_NEON__ defined :)
#endif

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
                                 rightLowOutputBuffers,
                                 frames);

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
