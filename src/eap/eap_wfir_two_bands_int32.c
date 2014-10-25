#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_two_bands_int32.h"
#include "eap_warped_delay_line_int32.h"

void
EAP_WfirTwoBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirTwoBandsInt32 *inst = (EAP_WfirTwoBandsInt32 *)instance;
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 4;

  for (i = 0; i < 3; i ++)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  inst->m_leftCompMem[0] = 0;
  inst->m_leftCompMem[1] = 0;
  inst->m_rightCompMem[0] = 0;
  inst->m_rightCompMem[1] = 0;
}

#ifdef __ARM_NEON__
#define BAND_FILTER_8(channel, output) \
{ \
  int32x4_t __p = vshrq_n_s32(vld1q_s32(channel), 1); \
  int32x4_t __q = vshrq_n_s32(vld1q_s32(channel + 4), 2); \
  vst1q_s32 (&output[0][i], __p + __q); \
  vst1q_s32 (&output[1][i], __p - __q); \
}

#define BAND_FILTER_OTHERS(residue, channel, output) \
{ \
  int32x4_t __p = vshrq_n_s32(vld1q_s32(channel), 1); \
  int32x4_t __q = vshrq_n_s32(vld1q_s32(channel + 4), 2); \
  int32x4_t __r = __p + __q; \
  int32x4_t __s = __p - __q; \
  output[0][i] = vgetq_lane_s32(__r, 0); \
  output[1][i] = vgetq_lane_s32(__s, 0); \
  if(residue == 2) \
  { \
    output[0][i + 1] = vgetq_lane_s32(__r, 1); \
    output[1][i + 1] = vgetq_lane_s32(__s, 1); \
  } \
  if(residue == 3) \
  { \
    output[0][i + 1] = vgetq_lane_s32(__r, 1); \
    output[1][i + 1] = vgetq_lane_s32(__s, 1); \
    output[0][i + 2] = vgetq_lane_s32(__r, 2); \
    output[1][i + 2] = vgetq_lane_s32(__s, 2); \
  } \
}

void
EAP_WfirTwoBandsInt32_filter(int32 *w_left, int32 *w_right,
                             int32 *const *leftLowOutputBuffers,
                             int32 *const *rightLowOutputBuffers,
                             int sample_count)
{
  int32 residue = sample_count & 3;
  sample_count -= residue;
  int i;

  for(i = 0; i < sample_count; i += 4, w_left += 8, w_right += 8)
  {
    BAND_FILTER_8(w_left, leftLowOutputBuffers);
    BAND_FILTER_8(w_right, rightLowOutputBuffers);
  }

  if (residue)
  {
    BAND_FILTER_OTHERS(residue, w_left, leftLowOutputBuffers);
    BAND_FILTER_OTHERS(residue, w_right, rightLowOutputBuffers);
  }
}
#else
#error TODO - x86 implementation. You need __ARM_NEON__ defined :)
#endif

void EAP_WfirTwoBandsInt32_Process(EAP_WfirInt32 *instance,
                                   int32 *const *leftLowOutputBuffers,
                                   int32 *const *rightLowOutputBuffers,
                                   int32 *leftHighOutput,
                                   int32 *rightHighOutput,
                                   const int32 *leftLowInput,
                                   const int32 *rightLowInput,
                                   const int32 *leftHighInput,
                                   const int32 *rightHighInput,
                                   int32 frames)

{
  EAP_WfirTwoBandsInt32 *inst = (EAP_WfirTwoBandsInt32 *)instance;
  int32 warpingShift = inst->common.warpingShift;
  int32 *leftMem = inst->m_leftMemory;
  int32 *ptr_w_left = inst->common.w_left;
  int32 *ptr_w_right = inst->common.w_right;
  int32 *rightMem = inst->m_rightMemory;
  int32 leftMem0, leftMem1, rightMem0, rightMem1;
  int i = 0;

  while ( i < frames )
  {
    EAP_WarpedDelayLineInt32_Process(
          warpingShift, leftMem, 3, leftLowInput[i]);
    EAP_WarpedDelayLineInt32_Process(
          warpingShift, rightMem, 3, rightLowInput[i]);

    ptr_w_left[0] = leftMem[1];
    ptr_w_left[4] = leftMem[0] + leftMem[2];

    ptr_w_right[0] = rightMem[1];
    ptr_w_right[4] = rightMem[0] + rightMem[2];

    i ++;
    ptr_w_left ++;
    ptr_w_right ++;

    if (!(i % 4))
    {
      ptr_w_left += 4;
      ptr_w_right += 4;
    }
  }

  EAP_WfirTwoBandsInt32_filter(inst->common.w_left, inst->common.w_right,
                               leftLowOutputBuffers,
                               rightLowOutputBuffers,
                               frames);

  leftMem0 = inst->m_leftCompMem[0];
  leftMem1 = inst->m_leftCompMem[1];
  rightMem0 = inst->m_rightCompMem[0];
  rightMem1 = inst->m_rightCompMem[1];

  for (i = 0; i < frames; i ++)
  {
    int32 mem0;
    int32 s;

    mem0 = leftMem0;
    leftMem0 = leftHighInput[i];
    s = leftMem1 - leftMem0;
    leftMem1 = s - (s >> warpingShift) + mem0;
    leftHighOutput[i] = leftMem1;

    mem0 = rightMem0;
    s = rightMem1 - rightMem0;
    rightMem0 = rightHighInput[i];
    rightMem1 = s - (s >> warpingShift) + mem0;
    rightHighOutput[i] = rightMem1;
  }

  inst->m_leftCompMem[0] = leftMem0;
  inst->m_leftCompMem[1] = leftMem1;
  inst->m_rightCompMem[0] = rightMem0;
  inst->m_rightCompMem[1] = rightMem1;
}

#ifdef EAP_WFIR_TWO_BANDS_INT32_TEST
/*
  This is what I used to check if "my implementation" behaves in the same way
  as stock. I used QtCreator, so the code won't compile outside of it.
*/

/*
  This code can be safely removed once we are sure eveything works on the device
*/

#define SAMPLES 1024
//#define SAMPLES 1024 - 1
//#define SAMPLES 1024 - 2
//#define SAMPLES 1024 - 3

#define FIRSIZE 8192 /*give it some room*/

EAP_WfirInt32 firnok[FIRSIZE] = {{-1},};
EAP_WfirInt32 fir[FIRSIZE] = {{-1},};

int32 a[8][SAMPLES];
int32 b[2][SAMPLES];

int32 an[8][SAMPLES];
int32 bn[2][SAMPLES];

#define COMPARE(x, index) \
  if(x[index][i] != x##n[index][i]) \
    qDebug() << #x"["#index"]" << i;

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);

  int32 *aa[2] = {a[0], a[1]};
  int32 *bb[2] = {b[0], b[1]};
  int32 *anok[2] = {an[0], an[1]};
  int32 *bnok[2] = {bn[0], bn[1]};

  memset(a,1, sizeof(a));
  memset(b,1, sizeof(b));

  memset(an,1, sizeof(an));
  memset(bn,1, sizeof(bn));

  int i;

  for(i = 0; i < FIRSIZE; i++)
  {
    fir[i].warpingShift = firnok[i].warpingShift = 1;
  }

  void *h = dlopen("/usr/lib/pulse-0.9.15/modules/module-nokia-record.so", RTLD_LAZY);
  EAP_WfirInt32_ProcessFptr f = (EAP_WfirInt32_ProcessFptr)dlsym(h,"EAP_WfirTwoBandsInt32_Process");
  EAP_WfirInt32_InitFptr finit = (EAP_WfirInt32_InitFptr)dlsym(h,"EAP_WfirTwoBandsInt32_Init");

  finit(firnok, 22050);
  EAP_WfirTwoBandsInt32_Init(fir, 22050);

  qDebug() << "BEFORE!!!";
  for(i = 0; i < FIRSIZE; i++)
  {
    if (fir[i].warpingShift != firnok[i].warpingShift)
      qDebug() << i;
  }

  for(i=0; i < SAMPLES; i++)
  {
    COMPARE(a,0);
    COMPARE(a,1);
    COMPARE(a,2);
    COMPARE(a,3);
    COMPARE(a,4);
    COMPARE(a,5);
    COMPARE(a,6);
    COMPARE(a,7);
    COMPARE(b,0);
    COMPARE(b,1);
  }

  f(firnok, anok, bnok, an[2], an[3], an[4], an[5], an[6], an[7], SAMPLES);

  EAP_WfirTwoBandsInt32_Process(fir, aa, bb, a[2], a[3], a[4], a[5], a[6], a[7],
                                SAMPLES);

  qDebug() << "AFTER!!!";
  for(i = 0; i < FIRSIZE; i++)
  {
    if (fir[i].warpingShift != firnok[i].warpingShift)
      qDebug() << i;
  }

  for(i=0; i < SAMPLES; i++)
  {
    COMPARE(a,0);
    COMPARE(a,1);
    COMPARE(a,2);
    COMPARE(a,3);
    COMPARE(a,4);
    COMPARE(a,5);
    COMPARE(a,6);
    COMPARE(a,7);
    COMPARE(b,0);
    COMPARE(b,1);
  }
}
#endif
