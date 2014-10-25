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
EAP_AverageAmplitudeInt32_Init(EAP_AverageAmplitudeInt32 *instance,
                               int downSamplingFactor, int shift);

int
EAP_AverageAmplitudeInt32_MaxOutputCount(int factor, int blockSize);

void
EAP_AverageAmplitudeInt32_SetDownSamplingFactor(
    EAP_AverageAmplitudeInt32 *instance, int factor);

void
EAP_AverageAmplitudeInt32_SetShiftValue(EAP_AverageAmplitudeInt32 *instance,
                                        int shift);

int
EAP_AverageAmplitudeInt32_Process(EAP_AverageAmplitudeInt32 *instance,
                                  int32 *output, const int32 *inputLeft,
                                  const int32 *inputRight, int inputFrames,
                                  int addFlag);

#endif // EAP_AVERAGE_AMPLITUDE_INT32_H
