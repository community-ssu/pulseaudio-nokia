#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_two_bands_int32.h"

void
EAP_WfirTwoBandsInt32_Init(EAP_WfirInt32 *fir, int sampleRate)
{
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  fir->warpingShift = 4;

  for (i = 0; i < 3; i++)
  {
    fir[i + 3841].warpingShift = 0;
    fir[i + 3844].warpingShift = 0;
  }

  fir[3847].warpingShift = 0;
  fir[3848].warpingShift = 0;
  fir[3849].warpingShift = 0;
  fir[3850].warpingShift = 0;
}

#ifdef __ARM_NEON__
#define BAND_FILTER_8(fir, channel) \
{ \
  int32x4_t __p = vshrq_n_s32(vld1q_s32(fir), 1); \
  int32x4_t __q = vshrq_n_s32(vld1q_s32(fir + 4), 2); \
  vst1q_s32 (&channel[0][i], __p + __q); \
  vst1q_s32 (&channel[1][i], __p - __q); \
}

#define BAND_FILTER_OTHERS(rem, fir, channel) \
{ \
  int32x4_t __p = vshrq_n_s32(vld1q_s32(fir), 1); \
  int32x4_t __q = vshrq_n_s32(vld1q_s32(fir + 4), 2); \
  int32x4_t __r = __p + __q; \
  int32x4_t __s = __p - __q; \
  channel[0][i] = vgetq_lane_s32(__r, 0); \
  channel[1][i] = vgetq_lane_s32(__s, 0); \
  if(rem == 2) \
  { \
    channel[0][i + 1] = vgetq_lane_s32(__r, 1); \
    channel[1][i + 1] = vgetq_lane_s32(__s, 1); \
  } \
  if(rem == 3) \
  { \
    channel[0][i + 1] = vgetq_lane_s32(__r, 1); \
    channel[1][i + 1] = vgetq_lane_s32(__s, 1); \
    channel[0][i + 2] = vgetq_lane_s32(__r, 2); \
    channel[1][i + 2] = vgetq_lane_s32(__s, 2); \
  } \
}

void
EAP_WfirTwoBandsInt32_filter(int *fl, int *fr, int **l, int **r,
                             int samplesCount)
{
  int rem = samplesCount & 3;
  samplesCount -= rem;
  int i;

  for(i = 0; i < samplesCount; i += 4, fl += 8, fr += 8)
  {
    BAND_FILTER_8(fl, l);
    BAND_FILTER_8(fr, r);
  }

  if (rem)
  {
    BAND_FILTER_OTHERS(rem, fl, l);
    BAND_FILTER_OTHERS(rem, fr, r);
  }
}
#else
#error You need __ARM_NEON__ defined
#endif


