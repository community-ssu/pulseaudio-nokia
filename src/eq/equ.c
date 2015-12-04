#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "equ.h"
#include <assert.h>

static inline int32_t CLIP_64_32(int64_t a)
{
    if ((a + 0x80000000u) & ~UINT64_C(0xFFFFFFFF))
      return (a >> 63) ^ 0x7FFFFFFF;
    else
      return a;
}

#define MUL64(a, b) ((a) * (int64_t)(b))

void
a_equ(int16_t *src, int16_t *coeff, int32_t *delay, int16_t *dst,
      int32_t samples, int32_t *scratch)
{
  int32_t *y;
  int shift;
  int16_t *a, *b;
  int64_t in, y64;
  int i, j, yt;

  if (!samples)
    return;

  for (i = 0; i < samples; i ++)
  {
    in = src[i] << 16;
    y = delay;
    shift = coeff[2];
    a = coeff + 3;
    b = coeff + coeff[1] + 2;

    for(j = 0; j < *coeff + 1; j ++)
    {
      in = (shift < 0) ? in >> -shift : in << shift;
      shift = a[3];
      y64 = in + ((MUL64(a[1], y[0]) + MUL64(a[2], y[1])) >> 15);
      yt = (a[0] < 0) ? y64 >> -a[0] : y64 << a[0];
      in = ((MUL64(b[0], y[0]) + MUL64(b[1], y[1])) >> 15) +
          (MUL64(b[2], yt) >> 15);
      y[0] = y[1];
      y[1] = yt;
      y += 2;
      b += 3;
      a += 4;
    }

    if (shift < 0)
      dst[i] = CLIP_64_32(in >> -shift) >> 16;
    else
      dst[i] = CLIP_64_32(in << shift) >> 16;
  }
}

void
filter_samples2(int16_t *samples_left, int16_t *samples_right,
                int16_t *out_left, int16_t *out_right, int32_t numsamples,
                int16_t stereo, fir_eq_conf *fir_eq_conf_var,
                int32_t *scratch)
{
  assert(0 && "TODO filter_samples2 address 0x00024CAC");
};
