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
EAP_WfirTwoBandsInt32_filter(int *fl, int *fr, int32 *const *l, int32 *const *r,
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
#error TODO - x86 implementation. You need __ARM_NEON__ defined :)
#endif

static void
shift_filter(char shift, int *x, int numSamples, int oldest)
{
  int sample = x[0];
  int i;

  x[0] = oldest;

  for (i = 1; i < numSamples; ++i)
  {
    int old = x[i];
    int diff = old - x[i - 1];

    x[i] = diff - (diff >> shift) + sample;
    sample = old;
  }
}

void
EAP_WfirTwoBandsInt32_Process(EAP_WfirInt32 *fir, int32 *const *leftChannel,
                              int32 *const *rightChannel, int32 *unk1,
                              int32 *unk2, const int32 *unk3,
                              const int32 *unk4, const int32 *unk5,
                              const int32 *unk6, int sampleCount)
{
  int32 shift;
  int32 *p, *q, *r, *s;
  int32 a, b, c, d;

  int i;

  shift = fir->warpingShift;
  p = &fir[3841].warpingShift;
  q = &fir[1].warpingShift;
  r = &fir[1921].warpingShift;
  s = &fir[3844].warpingShift;

  i = 0;
  while ( i < sampleCount )
  {
    shift_filter(shift, p, 3, unk3[i]);
    shift_filter(shift, s, 3, unk4[i]);

    q[0] = p[1];
    q[4] = p[0] + p[2];

    r[0] = s[1];
    r[4] = s[0] + s[2];

    ++i;
    ++q;
    ++r;

    if ( !(i % 4) )
    {
      q += 4;
      r += 4;
    }
  }

  EAP_WfirTwoBandsInt32_filter(&fir[1].warpingShift,&fir[1921].warpingShift,
                               leftChannel, rightChannel, sampleCount);

  a = fir[3847].warpingShift;
  b = fir[3848].warpingShift;
  c = fir[3849].warpingShift;
  d = fir[3850].warpingShift;

  for (i = 0; i < sampleCount; i++)
  {
    int a_old = a;
    int c_old = c;

    a = unk5[i];
    b = b - a - ((b - a) >> shift) + a_old;
    unk1[i] = b;

    c = unk6[i];
    d = d - c - ((d - c) >> shift) + c_old;
    unk2[i] = d;
  }

  fir[3847].warpingShift = a;
  fir[3848].warpingShift = b;
  fir[3849].warpingShift = c;
  fir[3850].warpingShift = d;
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
