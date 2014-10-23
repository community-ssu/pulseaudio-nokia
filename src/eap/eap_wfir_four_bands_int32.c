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

#ifdef __ARM_NEON__
static const int32x2_t h0_opt[3] =
{
  {0x15550000, 0x13D70000},
  {0xFD60000, 0xA8B0000},
  {0x5700000, 0x1AC0000}
};

static const int32x2_t h1_opt[3] =
{
  {0x2AAB0000, 0x13D70000},
  {0xFD60000, 0x15170000},
  {0x5700000, 0x1AC0000}
};

#define PROCESS_CHANNEL(ptr, out) \
  W0 = vld1q_s32(&ptr[0]); \
  W4 = vld1q_s32(&ptr[4]); \
  W8 = vld1q_s32(&ptr[8]); \
  W12 = vld1q_s32(&ptr[12]); \
  W16 = vld1q_s32(&ptr[16]); \
\
  A = vaddq_s32(vaddq_s32(vqdmulhq_lane_s32 (W0, h0_opt[0], 0), \
                           vqdmulhq_lane_s32 (W8, h0_opt[1], 0)), \
                 vqdmulhq_lane_s32 (W16, h0_opt[2], 0)); \
  B = vaddq_s32(vaddq_s32(vqdmulhq_lane_s32 (W4, h0_opt[0], 1), \
                           vqdmulhq_lane_s32 (W12, h0_opt[1], 1)), \
                 vqdmulhq_lane_s32 (W16, h0_opt[2], 1)); \
\
  vst1q_s32(&out[0][i], vaddq_s32(A, B)); \
  vst1q_s32(&out[3][i], vsubq_s32(A, B)); \
\
  A = vsubq_s32(vsubq_s32(vqdmulhq_lane_s32 (W0, h1_opt[0], 0), \
                           vqdmulhq_lane_s32 (W8, h1_opt[1], 0)), \
                 vqdmulhq_lane_s32 (W16, h1_opt[2], 0)); \
\
  B = vaddq_s32(vsubq_s32(vqdmulhq_lane_s32 (W4, h1_opt[0], 1), \
                           vqdmulhq_lane_s32 (W12, h1_opt[1], 1)), \
                 vqdmulhq_lane_s32 (vld1q_s32(&w_left[20]), h1_opt[2], 1)); \
\
  vst1q_s32(&out[1][i], vaddq_s32(A, B)); \
  vst1q_s32(&out[2][i], vsubq_s32(A, B));

#define PROCESS_CHANNEL_RESIDUE(ptr, out) \
  W0 = vld1q_s32(&ptr[0]); \
  W4 = vld1q_s32(&ptr[4]); \
  W8 = vld1q_s32(&ptr[8]); \
  W12 = vld1q_s32(&ptr[12]); \
  W16 = vld1q_s32(&ptr[16]); \
\
  A = vaddq_s32(vaddq_s32(vqdmulhq_lane_s32 (W0, h0_opt[0], 0), \
                           vqdmulhq_lane_s32 (W8, h0_opt[1], 0)), \
                 vqdmulhq_lane_s32 (W16, h0_opt[2], 0)); \
  B = vaddq_s32(vaddq_s32(vqdmulhq_lane_s32 (W4, h0_opt[0], 1), \
                           vqdmulhq_lane_s32 (W12, h0_opt[1], 1)), \
                 vqdmulhq_lane_s32 (W16, h0_opt[2], 1)); \
\
  OUT0 = vaddq_s32(A, B); \
  OUT3 = vsubq_s32(A, B); \
\
  A = vsubq_s32(vsubq_s32(vqdmulhq_lane_s32 (W0, h1_opt[0], 0), \
                           vqdmulhq_lane_s32 (W8, h1_opt[1], 0)), \
                 vqdmulhq_lane_s32 (W16, h1_opt[2], 0)); \
\
  B = vaddq_s32(vsubq_s32(vqdmulhq_lane_s32 (W4, h1_opt[0], 1), \
                           vqdmulhq_lane_s32 (W12, h1_opt[1], 1)), \
                 vqdmulhq_lane_s32 (vld1q_s32(&w_left[20]), h1_opt[2], 1)); \
\
  OUT1 = vaddq_s32(A, B); \
  OUT2 = vsubq_s32(A, B); \
\
  out[0][i] = vgetq_lane_s32(OUT0, 0); \
  out[1][i] = vgetq_lane_s32(OUT1, 0); \
  out[2][i] = vgetq_lane_s32(OUT2, 0); \
  out[3][i] = vgetq_lane_s32(OUT3, 0); \
  if(residue == 2) \
  { \
    out[0][i + 1] = vgetq_lane_s32(OUT0, 1); \
    out[1][i + 1] = vgetq_lane_s32(OUT1, 1); \
    out[2][i + 1] = vgetq_lane_s32(OUT2, 1); \
    out[3][i + 1] = vgetq_lane_s32(OUT3, 1); \
  } \
\
  if(residue == 3) \
  { \
    out[0][i + 1] = vgetq_lane_s32(OUT0, 1); \
    out[1][i + 1] = vgetq_lane_s32(OUT1, 1); \
    out[2][i + 1] = vgetq_lane_s32(OUT2, 1); \
    out[3][i + 1] = vgetq_lane_s32(OUT3, 1); \
    out[0][i + 2] = vgetq_lane_s32(OUT0, 2); \
    out[1][i + 2] = vgetq_lane_s32(OUT1, 2); \
    out[2][i + 2] = vgetq_lane_s32(OUT2, 2); \
    out[3][i + 2] = vgetq_lane_s32(OUT3, 2); \
  }

void
EAP_WfirFourBandsInt32_filter(int32 *w_left, int32 *w_right,
                              int32 *const *leftLowOutputBuffers,
                              int32 *const *rightLowOutputBuffers,
                              int sample_count)
{
  int residue = sample_count & 3;
  int i;

  int32x4_t W0, W4, W8, W12, W16, A, B;

  for (i = 0; sample_count - residue > i; i += 4, w_left += 24, w_right += 24)
  {
    PROCESS_CHANNEL(w_left, leftLowOutputBuffers);
    PROCESS_CHANNEL(w_right, rightLowOutputBuffers);
  }

  if (residue)
  {
    int32x4_t OUT0, OUT1, OUT2, OUT3;

    PROCESS_CHANNEL_RESIDUE(w_left, leftLowOutputBuffers);
    PROCESS_CHANNEL_RESIDUE(w_right, rightLowOutputBuffers);
  }
}
#else
#error TODO - x86 implementation. You need __ARM_NEON__ defined :)
#endif

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
                                rightLowOutputBuffers,
                                frames);

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

#ifdef EAP_WFIR_FOUR_BANDS_INT32_TEST
#define timing(a) \
{ \
  clock_t start=clock(), diff; \
  int msec; \
  do { \
    a; \
  } \
  while(0); \
  diff = clock() - start; \
  msec = diff * 1000 / CLOCKS_PER_SEC; \
  printf("msecs: %d\n",msec); \
}

#define SAMPLES 1920/6
#define BANDS 4
#define BENCHCNT 10000

int32 a[BANDS][SAMPLES] __attribute__((aligned(4096)));
int32 b[BANDS][SAMPLES] __attribute__((aligned(4096)));

int32 an[BANDS][SAMPLES] __attribute__((aligned(4096)));
int32 bn[BANDS][SAMPLES] __attribute__((aligned(4096)));

int32 *leftLowOutputBuffers[BANDS] __attribute__((aligned(4096)));
int32 *rightLowOutputBuffers[BANDS] __attribute__((aligned(4096)));
int32 *leftLowOutputBuffersn[BANDS] __attribute__((aligned(4096)));
int32 *rightLowOutputBuffersn[BANDS] __attribute__((aligned(4096)));
int32 leftHighOutput[SAMPLES] __attribute__((aligned(4096)));
int32 rightHighOutput[SAMPLES] __attribute__((aligned(4096)));
int32 leftHighOutputn[SAMPLES] __attribute__((aligned(4096)));
int32 rightHighOutputn[SAMPLES] __attribute__((aligned(4096)));
int32 lowInput[SAMPLES] __attribute__((aligned(4096)));

EAP_WfirFourBandsInt32 inst, instn;

void initOutput()
{
  int i, j;

  for (i = 0; i < BANDS; i++)
  {
    for (j = 0; j < SAMPLES; j ++)
    {
      a[i][j] = an[i][j] = (j % 3) ? j : -j;
      b[i][j] = bn[i][j] = (j % 3) ? j : -j;
    }

    leftLowOutputBuffers[i] = a[i];
    rightLowOutputBuffers[i] = b[i];

    leftLowOutputBuffersn[i] = an[i];;
    rightLowOutputBuffersn[i] = bn[i];;
  }

  for (i = 0; i < SAMPLES; i++)
  {
    leftHighOutput[i] = rightHighOutput[i] =
        leftHighOutputn[i] = rightHighOutputn[i] = 1;
    lowInput[i] = i;
  }

  for (i = 0; i < sizeof(inst.common.w_left) / sizeof(int32); i++)
  {
    inst.common.w_left[i] = instn.common.w_left[i] = (i % 3) ? i : -i;
    inst.common.w_right[i] = instn.common.w_right[i] = (i % 3) ? i : -i;
  }
}

int main(int argc, char *argv[])
{
  int i, j;

  initOutput();

  void *h = dlopen("/usr/lib/pulse-0.9.15/modules/module-nokia-record.so", RTLD_LAZY);
  EAP_WfirInt32_ProcessFptr process = (EAP_WfirInt32_ProcessFptr)dlsym(h,"EAP_WfirFourBandsInt32_Process");
  EAP_WfirInt32_InitFptr filterinit = (EAP_WfirInt32_InitFptr)dlsym(h,"EAP_WfirFourBandsInt32_Init");

  filterinit((EAP_WfirInt32 *)&instn, 22050);
  EAP_WfirFourBandsInt32_Init((EAP_WfirInt32 *)&inst, 22050);

  if(memcmp(&inst, &instn, sizeof(inst)))
  {
    printf("filterbanks differ after init");
    abort();
  }

  process((EAP_WfirInt32 *)&instn,
          leftLowOutputBuffersn,
          rightLowOutputBuffersn,
          leftHighOutputn,
          rightHighOutputn,
          lowInput,
          lowInput,
          lowInput,
          lowInput,
         SAMPLES);

  EAP_WfirFourBandsInt32_Process((EAP_WfirInt32 *)&inst,
                                  leftLowOutputBuffers,
                                  rightLowOutputBuffers,
                                  leftHighOutput,
                                  rightHighOutput,
                                  lowInput,
                                  lowInput,
                                  lowInput,
                                  lowInput,
                                  SAMPLES);

  if(memcmp(&inst, &instn, sizeof(inst)))
  {
    printf("filterbanks differ after init");
    abort();
  }

  for (i = 0; i < BANDS; i++)
  {
    for(j = 0; j < SAMPLES; j++)
    {
      if(leftLowOutputBuffers[i][j] != leftLowOutputBuffersn[i][j])
      {
        printf("leftLowOutputBuffers[%d][%d] differ [%x] - [%x]\n", i, j,
               leftLowOutputBuffers[i][j], leftLowOutputBuffersn[i][j]);
        abort();
      }
      if(rightLowOutputBuffers[i][j] != rightLowOutputBuffersn[i][j])
      {
        printf("rightLowOutputBuffers[%d][%d] differ [%x] - [%x]\n", i, j,
               rightLowOutputBuffers[i][j], rightLowOutputBuffersn[i][j]);
        abort();
      }
    }
  }

  for (i = 0; i < SAMPLES; i++)
  {
    if(leftHighOutput[i] != leftHighOutputn[i])
    {
      printf("leftHighOutput[%d] differ [%x] - [%x]\n", i,
             leftHighOutput[i], leftHighOutputn[i]);
      abort();
    }
    if(rightHighOutput[i] != rightHighOutputn[i])
    {
      printf("rightHighOutput[%d] differ [%x] - [%x]\n", i,
             rightHighOutput[i], rightHighOutputn[i]);
      abort();
    }
  }
  printf("Timing EAP_WfirFourBandsInt32_Process with 3 x %d runs, %d samples\n", BENCHCNT, SAMPLES);

  printf("FOSS replacement:\n");
  for (i = 0; i < 3; i++)
  {
      timing(
        for (j = 0; j < BENCHCNT; j++)
          EAP_WfirFourBandsInt32_Process((EAP_WfirInt32 *)&inst,
                                          leftLowOutputBuffers,
                                          rightLowOutputBuffers,
                                          leftHighOutput,
                                          rightHighOutput,
                                          lowInput,
                                          lowInput,
                                          lowInput,
                                          lowInput,
                                          SAMPLES)
      );
  }
  printf("Nokia stock:\n");
  for (i = 0; i < 3; i++)
  {
    timing(
      for (j = 0; j < BENCHCNT; j++)
        process((EAP_WfirInt32 *)&inst,
                leftLowOutputBuffers,
                rightLowOutputBuffers,
                leftHighOutput,
                rightHighOutput,
                lowInput,
                lowInput,
                lowInput,
                lowInput,
               SAMPLES)
        );
  }
}
#endif
