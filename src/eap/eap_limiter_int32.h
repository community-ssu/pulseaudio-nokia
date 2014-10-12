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
EAP_LimiterInt32_Init(EAP_LimiterInt32 *limiter, int *lookaheadMem1,
                      int *lookaheadMem2, int memSize, int16 *scratch);

#endif // EAP_LIMITER_INT32_H
