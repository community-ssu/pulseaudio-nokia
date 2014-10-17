#ifndef EAP_LIMITER_INT32_H
#define EAP_LIMITER_INT32_H

#include "eap_data_types.h"

struct _EAP_LimiterInt32
{
  int32 m_threshold;
  int16 m_attCoeff;
  int16 m_relCoeff;
  int32 m_prevGainQ30;
  int32 *m_lookaheadMem1;
  int32 *m_lookaheadMem2;
  int16 *m_scratch;
  int m_memSize;
};
typedef struct _EAP_LimiterInt32 EAP_LimiterInt32;

void
EAP_LimiterInt32_Init(EAP_LimiterInt32 *instance, int32 *lookaheadMemBuffer1,
                      int32 *lookaheadMemBuffer2, int memSize,
                      void *scratchBuffer);

void
EAP_LimiterInt32_AmplToGain(EAP_LimiterInt32 *instance, const int32 *in1,
                            const int32 *in2, int frames);

void
EAP_LimiterInt32_UpdateAttackCoeff(EAP_LimiterInt32 *instance, int16 attack);

void
EAP_LimiterInt32_UpdateReleaseCoeff(EAP_LimiterInt32 *instance, int16 release);

void
EAP_LimiterInt32_UpdateThreshold(EAP_LimiterInt32 *instance, int32 threshold);

void
EAP_LimiterInt32_Gain_Scal(const int32 *in1, const int32 *in2, int16 *gainVector,
                           int32 *out1, int32 *out2, int32 loop_count);

void
EAP_LimiterInt32_Process(EAP_LimiterInt32 *instance, int32 *output1,
                         int32 *output2, const int32 *input1,
                         const int32 *input2, int frames);

#endif // EAP_LIMITER_INT32_H
