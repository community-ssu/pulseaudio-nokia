#ifndef EAP_AVERAGE_AMPLITUDE_INT32_H
#define EAP_AVERAGE_AMPLITUDE_INT32_H

#include "eap_data_types.h"

struct _EAP_AverageAmplitudeInt32
{
  int32 m_memory;
  int m_downSamplingFactor;
  int m_downSamplingCounter;
  int m_shift;
};
typedef struct _EAP_AverageAmplitudeInt32 EAP_AverageAmplitudeInt32;

void
EAP_AverageAmplitudeInt32_Init(EAP_AverageAmplitudeInt32 *avgFilter,
                               int downSamplingFactor, int shift);

#endif // EAP_AVERAGE_AMPLITUDE_INT32_H
