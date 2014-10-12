#include "eap_limiter_int32.h"

void
EAP_LimiterInt32_Init(EAP_LimiterInt32 *limiter, int *lookaheadMem1,
                      int *lookaheadMem2, int memSize, int16 *scratch)
{
  int i;

  limiter->m_threshold = 0x8000000u;
  limiter->m_attCoeff = 0;
  limiter->m_relCoeff = 0;
  limiter->m_prevGainQ30 = 0x40000000u;
  limiter->m_lookaheadMem1 = lookaheadMem1;
  limiter->m_lookaheadMem2 = lookaheadMem2;
  limiter->m_memSize = memSize;
  limiter->m_scratch = scratch;

  for ( i = 0; i < memSize; ++i )
  {
    lookaheadMem1[i] = 0;
    lookaheadMem2[i] = 0;
  }
}
