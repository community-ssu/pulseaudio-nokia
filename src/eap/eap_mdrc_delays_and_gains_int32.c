#include <assert.h>

#include "eap_mdrc_delays_and_gains_int32.h"

static int16 saturate16(int v)
{
  if ( v < EAP_INT16_MIN )
    return EAP_INT16_MIN;

  if ( v > EAP_INT16_MAX )
    return EAP_INT16_MAX;

  return v;
}

void
EAP_MdrcDelaysAndGainsInt32_Init(EAP_MdrcDelaysAndGainsInt32 *gains,
                                 int bandCount, int companderLookahead,
                                 int downSamplingFactor, int32 **memBuffers)
{
  int i;

  assert((bandCount >= 1) && (bandCount <= EAP_MDRC_MAX_BAND_COUNT));

  gains->m_bandCount = bandCount;
  gains->m_delay = companderLookahead;
  gains->m_downSamplingFactor = downSamplingFactor;
  gains->m_downSamplingCounter = 0;
  gains->m_oneOverFactorQ15 = saturate16(32768 / downSamplingFactor);

  for ( i = 0; i < EAP_MDRC_MAX_BAND_COUNT; ++i )
  {
    gains->m_currGainQ15[i] = 32768;
    gains->m_currDeltaQ15[i] = 0;
  }

  for (i = 0; i < 2 * (bandCount + 1); i++)
    gains->m_memBuffers[i] = memBuffers[i];

  while ( i <= 11 )
    gains->m_memBuffers[i++] = 0;
}
