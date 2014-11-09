#ifndef DSP_H
#define DSP_H

#ifdef __cplusplus
#undef _STDINT_H
#define __STDC_LIMIT_MACROS 1
#endif

#include <stdint.h>

#define ARM_DSP_FUNCTION(f) \
inline static int32_t __ ##f(int32_t a, int32_t b) __attribute__((always_inline));\
inline static int32_t __ ##f(int32_t a, int32_t b) \
{ \
  int rv; \
  __asm__ __volatile__( \
        #f" %0, %1, %2" \
        : \
        "=r"(rv) : "r"(a), "r"(b) \
        ); \
  return rv; \
}

ARM_DSP_FUNCTION(qadd)
ARM_DSP_FUNCTION(qadd16)
ARM_DSP_FUNCTION(qsub)
ARM_DSP_FUNCTION(qsub16)
ARM_DSP_FUNCTION(qdadd)
ARM_DSP_FUNCTION(smulbb)
ARM_DSP_FUNCTION(smulbt)

inline static int32_t __sat_mul_add_16(int16_t a, int16_t b) __attribute__((always_inline));
inline static int32_t __sat_mul_add_16(int16_t a, int16_t b)
{
  int32_t __res = __smulbb(a, b);
  return __qadd(__res, __res);
}

inline static int32_t __ssat_16(int32_t val) __attribute__((always_inline));
inline static int32_t __ssat_16(int32_t val)
{
  int rv;

  __asm__ __volatile__(
      "ssat %0, #16, %1"
      :
      "=r"(rv) : "r"(val)
      );

  return rv;
}

inline static int32_t __sbfx_16(int32_t val) __attribute__((always_inline));
inline static int32_t __sbfx_16(int32_t val)
{
  int rv;

  __asm__ __volatile__(
      "sbfx %0, %1, #15, #16"
      :
      "=r"(rv) : "r"(val)
      );

  return rv;
}

/*
 NFC what it does, despite it looks like saturating shift. Give it a better name
 if you can.
 */
inline static int32_t __sat_unk_32(int32_t a, int32_t b) __attribute__((always_inline));
inline static int32_t __sat_unk_32(int32_t a, int32_t b)
{
  int32_t rv;
  int32_t blz = __builtin_clz(b ^ (b << 1));

  if (blz > 0)
  {
    if ((__builtin_clz(a ^ (a << 1)) < blz) && a)
    {
      if (a < 0)
        rv = 0x80000000;
      else
        rv = 0x7fffffff;
    }
    else
      rv = a << blz;
  }
  else
    rv = a >> -blz;

  return rv;
}

#endif // DSP_H
