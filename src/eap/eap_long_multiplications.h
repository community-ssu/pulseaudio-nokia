#ifndef EAP_LONG_MULTIPLICATIONS_H
#define EAP_LONG_MULTIPLICATIONS_H

#include <assert.h>
#include <stdint.h>

static int32
EAP_LongMultPosQ15x32(int16 positiveIn1, int32 in2)
{
  assert(positiveIn1 >= 0);

  return (uint64_t)(in2 * (int64_t)positiveIn1) >> 15;
}

#endif // EAP_LONG_MULTIPLICATIONS_H
