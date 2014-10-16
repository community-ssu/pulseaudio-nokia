#include <assert.h>

#include "eap_mdrc_delays_and_gains_int32.h"
#include "eap_clip.h"

void
EAP_MdrcDelaysAndGainsInt32_Init(EAP_MdrcDelaysAndGainsInt32 *instance,
                                 int bandCount, int delay,
                                 int downSamplingFactor,
                                 int32 *const *memoryBuffers)
{
  int i;

  assert((bandCount >= 1) && (bandCount <= EAP_MDRC_MAX_BAND_COUNT));

  instance->m_bandCount = bandCount;
  instance->m_delay = delay;
  instance->m_downSamplingFactor = downSamplingFactor;
  instance->m_downSamplingCounter = 0;
  instance->m_oneOverFactorQ15 = EAP_Clip16(EAP_INT16_MAX / downSamplingFactor);

  for ( i = 0; i < EAP_MDRC_MAX_BAND_COUNT; ++i )
  {
    instance->m_currGainQ15[i] = EAP_INT16_MAX;
    instance->m_currDeltaQ15[i] = 0;
  }

  for (i = 0; i < 2 * (bandCount + 1); i++)
    instance->m_memBuffers[i] = memoryBuffers[i];

  while ( i <= 11 )
    instance->m_memBuffers[i++] = 0;
}
