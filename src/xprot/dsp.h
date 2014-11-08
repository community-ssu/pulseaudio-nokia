#ifndef DSP_H
#define DSP_H

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

#endif // DSP_H
