#ifndef EAP_LONG_MULTIPLICATIONS_H
#define EAP_LONG_MULTIPLICATIONS_H

#include <assert.h>
#include <stdint.h>
#include "eap_data_types.h"

static inline int32
EAP_LongMultPosQ15x32(int16 positiveIn1, int32 in2)
{
  assert(positiveIn1 >= 0);

  return in2 * (int64_t)positiveIn1 >> 15;
}

static inline int32
EAP_LongMult32x32(int32 in1, int32 in2)
{
  return in2 * in1;
}

static inline int32
EAP_LongMultPosQ14x32(int16 positiveIn1, int32 in2)
{
  assert(positiveIn1 >= 0);

  return ((int64_t)positiveIn1 * (int64_t)in2) >> 14;
}

static inline int32
EAP_LongMult32Q15x32(int32 in1, int32 in2)
{
  return (uint64_t)(in2 * (int64_t)in1) >> 15;
}

static inline int32
EAP_LongMultQ15x32(int16 in1, int32 in2)
{
  return (uint64_t)(in2 * (int64_t)in1) >> 15;
}

static inline void
Inverse(int16 *frac, int16 *exp)
{
  int mantissa;
  int16 f = *frac;
  int16 f035 = 11469 * f >> 15;
  int16 f035minus085 = f035 -27853;

  *exp = -*exp;
  mantissa = ((f035minus085 * f) >> 15) - EAP_INT16_MIN;

  if ( mantissa <= EAP_INT16_MAX )
  {
    mantissa <<= 1;
    --*exp;
  }

  *frac = mantissa + EAP_INT16_MIN;
}

#endif // EAP_LONG_MULTIPLICATIONS_H
