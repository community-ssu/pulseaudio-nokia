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
    instance->m_currGainQ15[i] = EAP_INT16_MAX + 1;
    instance->m_currDeltaQ15[i] = 0;
  }

  for (i = 0; i < 2 * (bandCount + 1); i ++)
    instance->m_memBuffers[i] = memoryBuffers[i];

  while (i < 2 * (EAP_MDRC_MAX_BAND_COUNT + 1))
    instance->m_memBuffers[i ++] = 0;
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

  for(i = 0; i < loop_count - residue; i += 4)
  {
    int32 gain;

    gain = gainVector[0];
    out1[0] = EAP_LongMult32Q15x32(gain, in1[0]);
    out2[0] = EAP_LongMult32Q15x32(gain, in2[0]);

    gain = gainVector[1];
    out1[1] = EAP_LongMult32Q15x32(gain, in1[1]);
    out2[1] = EAP_LongMult32Q15x32(gain, in2[1]);

    gain = gainVector[2];
    out1[2] = EAP_LongMult32Q15x32(gain, in1[2]);
    out2[2] = EAP_LongMult32Q15x32(gain, in2[2]);

    gain = gainVector[3];
    out1[3] = EAP_LongMult32Q15x32(gain, in1[3]);
    out2[3] = EAP_LongMult32Q15x32(gain, in2[3]);

    gainVector += 4;
    in1 += 4;
    in2 += 4;
    out1 += 4;
    out2 += 4;
  }

  if (residue)
  {
    for (i  = 0; i < residue; i ++)
    {
      int32 gain = *gainVector;

      *out1 = EAP_LongMult32Q15x32(gain, *in1);
      *out2 = EAP_LongMult32Q15x32(gain, *in2);

      gainVector ++;
      in1 ++;
      in2 ++;
      out1 ++;
      out2 ++;
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

  for(i = 0; i < loop_count - residue; i += 4)
  {
    int32 gain;

    gain = gainVector[0];
    out1[0] += EAP_LongMult32Q15x32(gain, *in1);
    out2[0] += EAP_LongMult32Q15x32(gain, *in2);

    gain = gainVector[1];
    out1[1] += EAP_LongMult32Q15x32(gain, in1[1]);
    out2[1] += EAP_LongMult32Q15x32(gain, in2[1]);

    gain = gainVector[2];
    out1[2] += EAP_LongMult32Q15x32(gain, in1[2]);
    out2[2] += EAP_LongMult32Q15x32(gain, in2[2]);

    gain = gainVector[3];
    out1[3] += EAP_LongMult32Q15x32(gain, in1[3]);
    out2[3] += EAP_LongMult32Q15x32(gain, in2[3]);

    gainVector += 4;
    in1 += 4;
    in2 += 4;
    out1 += 4;
    out2 += 4;
  }

  if (residue)
  {
    for (i  = 0; i < residue; i ++)
    {
       int32 gain = *gainVector;

      *out1 += EAP_LongMult32Q15x32(gain, *in1);
      *out2 += EAP_LongMult32Q15x32(gain, *in2);

      in1 ++;
      in2 ++;
      gainVector ++;
      out1 ++;
      out2 ++;
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
                                    int32 *leftHighInput,
                                    int32 *rightHighInput,
                                    int32 *const *gainInputs,
                                    int frames)
{
  int counter;
  int framesMemoryToOutput;
  int framesInputToOutput;
  int framesMemoryToMemory;
  int framesInputToMemory;
  int i;
  const int32 *lMem;
  const int32 *rMem;

  if (instance->m_delay > frames)
    framesMemoryToOutput = frames;
  else
    framesMemoryToOutput = instance->m_delay;

  framesInputToOutput = frames - framesMemoryToOutput;
  framesMemoryToMemory = instance->m_delay - framesMemoryToOutput;
  framesInputToMemory = instance->m_delay - framesMemoryToMemory;

  lMem = instance->m_memBuffers[0];
  rMem = instance->m_memBuffers[1];

  counter = instance->m_downSamplingCounter;

  CalcGainVector(instance,
                 leftHighOutput,
                 gainInputs[0],
                 &counter,
                 instance->m_currGainQ15,
                 instance->m_currDeltaQ15,
                 frames);

  EAP_MdrcDelaysAndGainsInt32_Gain_Scal(lMem,
                                        rMem,
                                        leftHighOutput,
                                        leftLowOutput,
                                        rightLowOutput,
                                        framesMemoryToOutput);

  lMem = &lMem[framesMemoryToOutput];
  rMem = &rMem[framesMemoryToOutput];

  EAP_MdrcDelaysAndGainsInt32_Gain_Scal(leftLowInputs[0],
                                        rightLowInputs[0],
                                        &leftHighOutput[framesMemoryToOutput],
                                        &leftLowOutput[framesMemoryToOutput],
                                        &rightLowOutput[framesMemoryToOutput],
                                        framesInputToOutput);

  if (framesMemoryToMemory)
  {
    memmove(instance->m_memBuffers[0],
            lMem,
            sizeof(int32) * framesMemoryToMemory);

    memmove(instance->m_memBuffers[1],
            rMem,
            sizeof(int32) * framesMemoryToMemory);
  }

  memcpy(&instance->m_memBuffers[0][framesMemoryToMemory],
         &leftLowInputs[0][framesInputToOutput],
         sizeof(int32) * framesInputToMemory);

  memcpy(&instance->m_memBuffers[1][framesMemoryToMemory],
         &rightLowInputs[0][framesInputToOutput],
         sizeof(int32) * framesInputToMemory);

  for (i = 1; instance->m_bandCount > i; i ++)
  {
    lMem = instance->m_memBuffers[2 * i];
    rMem = instance->m_memBuffers[2 * i + 1];

    counter = instance->m_downSamplingCounter;

    CalcGainVector(instance,
                   leftHighOutput,
                   gainInputs[i],
                   &counter,
                   &instance->m_currGainQ15[i],
                   &instance->m_currDeltaQ15[i],
                   frames);

    EAP_MdrcDelaysAndGainsInt32_Gain_Scal1(lMem,
                                           rMem,
                                           leftHighOutput,
                                           leftLowOutput,
                                           rightLowOutput,
                                           framesMemoryToOutput);

    EAP_MdrcDelaysAndGainsInt32_Gain_Scal1(
                leftLowInputs[i],
                rightLowInputs[i],
                &leftHighOutput[framesMemoryToOutput],
                &leftLowOutput[framesMemoryToOutput],
                &rightLowOutput[framesMemoryToOutput],
                framesInputToOutput);

    if (framesMemoryToMemory)
    {
      memmove(instance->m_memBuffers[2 * i], &lMem[framesMemoryToOutput],
              sizeof(int32) * framesMemoryToMemory);
      memmove(instance->m_memBuffers[2 * i + 1], &rMem[framesMemoryToOutput],
              sizeof(int32) * framesMemoryToMemory);
    }

    memcpy(&instance->m_memBuffers[2 * i][framesMemoryToMemory],
           &leftLowInputs[i][framesInputToOutput],
           sizeof(int32) * framesInputToMemory);

    memcpy(&instance->m_memBuffers[2 * i + 1][framesMemoryToMemory],
           &rightLowInputs[i][framesInputToOutput],
           sizeof(int32) * framesInputToMemory);
  }

  lMem = instance->m_memBuffers[2 * instance->m_bandCount];
  rMem = instance->m_memBuffers[2 * instance->m_bandCount + 1];

  EAP_MdrcDelaysAndGainsInt32_Gain_Scal(lMem,
                                        rMem,
                                        leftHighOutput,
                                        leftHighOutput,
                                        rightHighOutput,
                                        framesMemoryToOutput);

  EAP_MdrcDelaysAndGainsInt32_Gain_Scal(leftHighInput,
                                        rightHighInput,
                                        &leftHighOutput[framesMemoryToOutput],
                                        &leftHighOutput[framesMemoryToOutput],
                                        &rightHighOutput[framesMemoryToOutput],
                                        framesInputToOutput);

  if (framesMemoryToMemory)
  {
    memmove(instance->m_memBuffers[2 * instance->m_bandCount],
            &lMem[framesMemoryToOutput],
            sizeof(int32) * framesMemoryToMemory);
    memmove(instance->m_memBuffers[2 * instance->m_bandCount + 1],
            &rMem[framesMemoryToOutput],
            sizeof(int32) * framesMemoryToMemory);
  }

  memcpy(&instance->
                  m_memBuffers[2 * instance->m_bandCount][framesMemoryToMemory],
         &leftHighInput[framesInputToOutput],
         sizeof(int32) * framesInputToMemory);
  memcpy(&instance->
              m_memBuffers[2 * instance->m_bandCount + 1][framesMemoryToMemory],
         &rightHighInput[framesInputToOutput],
         sizeof(int32) * framesInputToMemory);

  instance->m_downSamplingCounter = counter;
}
