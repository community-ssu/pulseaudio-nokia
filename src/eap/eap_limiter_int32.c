#include "eap_limiter_int32.h"

void
EAP_LimiterInt32_Init(EAP_LimiterInt32 *instance, int32 *lookaheadMemBuffer1,
                      int32 *lookaheadMemBuffer2, int memSize,
                      void *scratchBuffer)
{
  int i;

  instance->m_threshold = 0x8000000u;
  instance->m_attCoeff = 0;
  instance->m_relCoeff = 0;
  instance->m_prevGainQ30 = 0x40000000u;
  instance->m_lookaheadMem1 = lookaheadMemBuffer1;
  instance->m_lookaheadMem2 = lookaheadMemBuffer2;
  instance->m_memSize = memSize;
  instance->m_scratch = scratchBuffer;

  for ( i = 0; i < memSize; ++i )
  {
    lookaheadMemBuffer1[i] = 0;
    lookaheadMemBuffer2[i] = 0;
  }
}
