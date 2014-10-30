#ifndef EAP_NORMALIZE_H
#define EAP_NORMALIZE_H

#include <math.h>

inline static int
EAP_Norm32(int32 input)
{
  int exponent = 0;

  frexp(input, &exponent);
  return 31 - exponent;
}

#endif // EAP_NORMALIZE_H
