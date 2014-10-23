#include <assert.h>
#include <stdlib.h>

#include "eap_average_amplitude_int32.h"

void
EAP_AverageAmplitudeInt32_Init(EAP_AverageAmplitudeInt32 *instance,
                               int downSamplingFactor, int shift)
{
  assert(downSamplingFactor > 1);
  assert(shift > 0);

  instance->m_memory = 0;
  instance->m_downSamplingCounter = 0;
  instance->m_downSamplingFactor = downSamplingFactor;
  instance->m_shift = shift;
}

int
EAP_AverageAmplitudeInt32_MaxOutputCount(int factor, int blockSize)
{
  return (blockSize + factor - 1) / factor;
}

void
EAP_AverageAmplitudeInt32_SetDownSamplingFactor(
    EAP_AverageAmplitudeInt32 *instance, int factor)
{
  assert(factor >= 1);

  instance->m_downSamplingFactor = factor;
  instance->m_downSamplingCounter = 0;
}

void
EAP_AverageAmplitudeInt32_SetShiftValue(EAP_AverageAmplitudeInt32 *instance,
                                        int shift)
{
  assert(shift > 0 && shift <= 15);

  instance->m_shift = shift;
}

int
EAP_AverageAmplitudeInt32_Process(EAP_AverageAmplitudeInt32 *instance,
                                  int32 *output, const int32 *inputLeft,
                                  const int32 *inputRight, int inputFrames,
                                  int addFlag)
{
  int factor = instance->m_downSamplingFactor;
  int counter = instance->m_downSamplingCounter;
  int shift = instance->m_shift;
  int32 mem = instance->m_memory;
  int outputSamples = 0;

  while (inputFrames > 0)
  {
    int tbd = factor - counter;
    int i;

    if (tbd > inputFrames)
      tbd = inputFrames;

    for (i = tbd; i > 0; i --)
    {
      mem = mem + abs(*inputLeft) + abs(*inputRight) - (mem >> shift);
      ++inputLeft;
      ++inputRight;
    }

    counter += tbd;
    inputFrames -= tbd;

    if (counter == factor)
    {
      if (addFlag)
        *output += mem >> shift;
      else
        *output = mem >> shift;

      output ++;
      counter = 0;
      outputSamples ++;
    }
  }

  instance->m_downSamplingCounter = counter;
  instance->m_memory = mem;

  return outputSamples;
}
