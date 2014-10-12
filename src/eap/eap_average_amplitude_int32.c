#include <assert.h>

#include "eap_average_amplitude_int32.h"

void
EAP_AverageAmplitudeInt32_Init(EAP_AverageAmplitudeInt32 *avgFilter,
                               int downSamplingFactor, int shift)
{
  assert(downSamplingFactor > 1);
  assert(shift > 0);

  avgFilter->m_shift = 0;
  avgFilter->m_memory = 0;
  avgFilter->m_downSamplingCounter = 0;
  avgFilter->m_downSamplingFactor = downSamplingFactor;
  avgFilter->m_shift = shift;
}
