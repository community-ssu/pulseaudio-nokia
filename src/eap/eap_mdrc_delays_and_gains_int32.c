#include <assert.h>
#include <string.h>

#include "eap_mdrc_delays_and_gains_int32.h"
#include "eap_clip.h"
#include "eap_long_multiplications.h"

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

/* FIXME - NEON-optimize me */
void
EAP_MdrcDelaysAndGainsInt32_Gain_Scal(int32 const *in1, int32 const *in2,
                                      int32 const *gainVector,
                                      int32 *out1, int32 *out2,
                                      int32 loop_count)
{
  int residue = loop_count & 3;
  int i = 0;

  while (loop_count - residue > i)
  {
    out1[0] = EAP_LongMult32Q15x32(gainVector[0], in1[0]);
    out2[0] = EAP_LongMult32Q15x32(gainVector[0], in2[0]);

    out1[1] = EAP_LongMult32Q15x32(gainVector[1], in1[1]);
    out2[1] = EAP_LongMult32Q15x32(gainVector[1], in2[1]);

    out1[2] = EAP_LongMult32Q15x32(gainVector[2], in1[2]);
    out2[2] = EAP_LongMult32Q15x32(gainVector[2], in2[2]);

    out1[3] = EAP_LongMult32Q15x32(gainVector[3], in1[3]);
    out2[3] = EAP_LongMult32Q15x32(gainVector[3], in2[3]);

    i += 4;
    gainVector += 4;
    in1 += 4;
    in2 += 4;
    out1 += 4;
    out2 += 4;
  }

  if (residue)
  {
    while (i < loop_count)
    {
      *out1 = EAP_LongMult32Q15x32(*gainVector, *in1);
      *out2 = EAP_LongMult32Q15x32(*gainVector, *in2);

      gainVector ++;
      in1 ++;
      in2 ++;
      out1 ++;
      out2 ++;
      i ++;
    }
  }
}

/* FIXME - NEON-optimize me */
void
EAP_MdrcDelaysAndGainsInt32_Gain_Scal1(int32 const *in1, int32 const *in2,
                                       int32 const *gainVector,
                                       int32 *out1, int32 *out2,
                                       int32 loop_count)
{
  int residue = loop_count & 3;
  int32 i = 0;

  while (loop_count - residue > i)
  {
    out1[0] += EAP_LongMult32Q15x32(gainVector[0], *in1);
    out2[0] += EAP_LongMult32Q15x32(gainVector[0], *in2);

    out1[1] += EAP_LongMult32Q15x32(gainVector[1], in1[1]);
    out2[1] += EAP_LongMult32Q15x32(gainVector[1], in2[1]);

    out1[2] += EAP_LongMult32Q15x32(gainVector[2], in1[2]);
    out2[2] += EAP_LongMult32Q15x32(gainVector[2], in2[2]);

    out1[3] += EAP_LongMult32Q15x32(gainVector[3], in1[3]);
    out2[3] += EAP_LongMult32Q15x32(gainVector[3], in2[3]);

    i += 4;
    gainVector += 4;
    in1 += 4;
    in2 += 4;
    out1 += 4;
    out2 += 4;
  }

  if (residue)
  {
    while (i < loop_count)
    {
      *out1 += EAP_LongMult32Q15x32(*gainVector, *in1);
      *out2 += EAP_LongMult32Q15x32(*gainVector, *in2);

      ++in1;
      ++in2;
      ++gainVector;
      ++out1;
      ++out2;
      ++i;
    }
  }
}

void
CalcGainVector(const EAP_MdrcDelaysAndGainsInt32 *instance,
               int32 *output,
               const int32 *input,
               int *currCounter,
               int32 *currGain,
               int32 *currDelta,
               int outputFrames)
{
  int i;
  int factor = instance->m_downSamplingFactor;
  int counter = *currCounter;
  int32 gain = *currGain;
  int32 delta = *currDelta;

  while (outputFrames > 0)
  {
    int tbd = factor - counter;

    if (tbd > outputFrames)
      tbd = outputFrames;

    for (i = tbd; i > 0; i --)
    {
      gain += delta;
      *output = gain;
      output ++;
    }

    counter += tbd;
    outputFrames -= tbd;

    if (counter == factor)
    {
      delta = EAP_LongMultQ15x32(instance->m_oneOverFactorQ15, *input - gain);
      input ++;
      counter = 0;
    }
  }

  *currCounter = counter;
  *currGain = gain;
  *currDelta = delta;
}

void
EAP_MdrcDelaysAndGainsInt32_Process(EAP_MdrcDelaysAndGainsInt32 *instance,
                                    int32 *leftLowOutput,
                                    int32 *rightLowOutput,
                                    int32 *leftHighOutput,
                                    int32 *rightHighOutput,
                                    int32 *const *leftLowInputs,
                                    int32 *const *rightLowInputs,
                                    const int32 *leftHighInput,
                                    const int32 *rightHighInput,
                                    int32 *const *gainInputs,
                                    int frames)
{
  int counter;
  int framesMemoryToOutput;
  int framesInputToOutput;
  int framesMemoryToMemory;
  int framesInputToMemory;
  int i;
  int b;
  int32 *lOut;
  int32 *rOut;
  int32 *gainVector;
  const int32 *lMem;
  const int32 *rMem;
  const int32 *lIn;
  const int32 *rIn;

  if (instance->m_delay > frames)
    framesMemoryToOutput = frames;
  else
    framesMemoryToOutput = instance->m_delay;

  framesInputToOutput = frames - framesMemoryToOutput;
  framesMemoryToMemory = instance->m_delay - framesMemoryToOutput;
  framesInputToMemory = instance->m_delay - framesMemoryToMemory;
  lOut = leftLowOutput;
  rOut = rightLowOutput;
  gainVector = leftHighOutput;
  lMem = instance->m_memBuffers[0];
  rMem = instance->m_memBuffers[1];
  lIn = *leftLowInputs;
  rIn = *rightLowInputs;
  counter = instance->m_downSamplingCounter;

  CalcGainVector(instance,
                 leftHighOutput,
                 gainInputs[0],
                 &counter,
                 instance->m_currGainQ15,
                 instance->m_currDeltaQ15,
                 frames);

  for (i = 0; i < framesMemoryToOutput; i ++)
  {
    int32 gain = *gainVector;

    *lOut = EAP_LongMult32Q15x32(gain, *lMem);
    *rOut = EAP_LongMult32Q15x32(gain, *rMem);

    gainVector ++;
    lMem ++;
    rMem ++;
    lOut ++;
    rOut ++;
  }

  for (i = 0; i < framesInputToOutput; i ++)
  {
    int32 gain = *gainVector;

    *lOut = EAP_LongMult32Q15x32(gain, *lIn);
    *rOut = EAP_LongMult32Q15x32(gain, *rIn);

    gainVector ++;
    lIn ++;
    rIn ++;
    lOut ++;
    rOut ++;
  }

  if (framesMemoryToMemory)
  {
    memmove(instance->m_memBuffers[0], lMem,
            sizeof(int32) * framesMemoryToMemory);
    memmove(instance->m_memBuffers[1], rMem,
            sizeof(int32) * framesMemoryToMemory);
  }

  memcpy(&instance->m_memBuffers[0][framesMemoryToMemory], lIn,
         sizeof(int32) * framesInputToMemory);
  memcpy(&instance->m_memBuffers[1][framesMemoryToMemory], rIn,
         sizeof(int32) * framesInputToMemory);

  for (b = 1; instance->m_bandCount > b; b ++)
  {
    lOut = leftLowOutput;
    rOut = rightLowOutput;
    gainVector = leftHighOutput;
    lMem = instance->m_memBuffers[2 * b];
    rMem = instance->m_memBuffers[2 * b + 1];
    lIn = leftLowInputs[b];
    rIn = rightLowInputs[b];
    counter = instance->m_downSamplingCounter;

    CalcGainVector(instance,
                   leftHighOutput,
                   gainInputs[b],
                   &counter,
                   &instance->m_currGainQ15[b],
                   &instance->m_currDeltaQ15[b],
                   frames);

    for (i = 0; i < framesMemoryToOutput; i ++)
    {
      int32 gain = *gainVector;

      *lOut = *lOut + EAP_LongMult32Q15x32(gain, *lMem);
      *rOut = *rOut + EAP_LongMult32Q15x32(gain, *rMem);

      gainVector ++;
      lMem ++;
      rMem ++;
      lOut ++;
      rOut ++;
    }

    for (i = 0; i < framesInputToOutput; i ++)
    {
      int32 gain = *gainVector;
      *lOut = *lOut + EAP_LongMult32Q15x32(gain, *lIn);
      *rOut = *rOut + EAP_LongMult32Q15x32(gain, *rIn);

      gainVector ++;
      lIn ++;
      rIn ++;
      lOut ++;
      rOut ++;
    }

    if (framesMemoryToMemory)
    {
      memmove(instance->m_memBuffers[2 * b], lMem,
              sizeof(int32) * framesMemoryToMemory);
      memmove(instance->m_memBuffers[2 * b + 1], rMem,
              sizeof(int32) * framesMemoryToMemory);
    }

    memcpy(&instance->m_memBuffers[2 * b][framesMemoryToMemory], lIn,
           sizeof(int32) * framesInputToMemory);
    memcpy(&instance->m_memBuffers[2 * b + 1][framesMemoryToMemory], rIn,
           sizeof(int32) * framesInputToMemory);
  }

  lOut = leftHighOutput;
  rOut = rightHighOutput;
  gainVector = leftHighOutput;
  lMem = instance->m_memBuffers[2 * instance->m_bandCount];
  rMem = instance->m_memBuffers[2 * instance->m_bandCount + 1];
  lIn = leftHighInput;
  rIn = rightHighInput;

  for (i = 0; i < framesMemoryToOutput; i ++)
  {
    int32 gain = *gainVector;

    *lOut = EAP_LongMult32Q15x32(gain, *lMem);
    *rOut = EAP_LongMult32Q15x32(gain, *rMem);

    gainVector ++;
    lMem ++;
    rMem ++;
    lOut ++;
    rOut ++;
  }

  for (i = 0; i < framesInputToOutput; i ++)
  {
    int32 gain = *gainVector;

    *lOut = EAP_LongMult32Q15x32(gain, *lIn);
    *rOut = EAP_LongMult32Q15x32(gain, *rIn);

    gainVector ++;
    lMem ++;
    rMem ++;
    lOut ++;
    rOut ++;
  }

  if (framesMemoryToMemory)
  {
    memmove(instance->m_memBuffers[2 * instance->m_bandCount], lMem,
            sizeof(int32) * framesMemoryToMemory);
    memmove(instance->m_memBuffers[2 * instance->m_bandCount + 1], rMem,
            sizeof(int32) * framesMemoryToMemory);
  }

  memcpy(&instance->
                  m_memBuffers[2 * instance->m_bandCount][framesMemoryToMemory],
         lIn,
         sizeof(int32) * framesInputToMemory);
  memcpy(&instance->
              m_memBuffers[2 * instance->m_bandCount + 1][framesMemoryToMemory],
         rIn,
         sizeof(int32) * framesInputToMemory);

  instance->m_downSamplingCounter = counter;
}
