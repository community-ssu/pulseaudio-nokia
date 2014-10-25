#ifndef EAP_LONG_MULTIPLICATIONS_H
#define EAP_LONG_MULTIPLICATIONS_H

#include <assert.h>
#include <stdint.h>
#include "eap_data_types.h"

static inline int32
EAP_LongMultPosQ15x32(int16 positiveIn1, int32 in2)
{
  assert(positiveIn1 >= 0);

  return ((uint64_t)(in2 * (int64_t)positiveIn1)) >> 15;
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

  return (((int)positiveIn1) * in2) >> 14;
}

static inline int32
EAP_LongMult32Q15x32(int32 in1, int32 in2)
{
  return ((uint64_t)(in2 * (int64_t)in1)) >> 15;
}

static inline int32
EAP_LongMultQ15x32(int16 in1, int32 in2)
{
  return ((uint64_t)(in2 * (int64_t)in1)) >> 15;
}
#endif // EAP_LONG_MULTIPLICATIONS_H
