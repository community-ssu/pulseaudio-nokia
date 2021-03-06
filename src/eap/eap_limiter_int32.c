#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "eap_limiter_int32.h"
#include "eap_long_multiplications.h"
#include "eap_normalize.h"

void
EAP_LimiterInt32_Init(EAP_LimiterInt32 *instance, int32 *lookaheadMemBuffer1,
                      int32 *lookaheadMemBuffer2, int memSize,
                      void *scratchBuffer)
{
  int i;

  instance->m_threshold = 0x8000000u;
  instance->m_attCoeff = 0;
  instance->m_relCoeff = 0;
  instance->m_prevGainQ30 = 0x40000000u;
  instance->m_lookaheadMem1 = lookaheadMemBuffer1;
  instance->m_lookaheadMem2 = lookaheadMemBuffer2;
  instance->m_memSize = memSize;
  instance->m_scratch = scratchBuffer;

  for (i = 0; i < memSize; ++i)
  {
    lookaheadMemBuffer1[i] = 0;
    lookaheadMemBuffer2[i] = 0;
  }
}

void
EAP_LimiterInt32_AmplToGain(EAP_LimiterInt32 *instance, const int32 *in1,
                            const int32 *in2, int frames)
{
  int32 prevGain = instance->m_prevGainQ30;
  int16 attCoeff = instance->m_attCoeff;
  int16 relCoeff = instance->m_relCoeff;
  int threshold = instance->m_threshold;
  int16 *gainOutput = instance->m_scratch;
  int i;

  for (i = 0; i < frames; i ++, gainOutput ++)
  {
    int ampl = EAP_MAX(abs(*in1), abs(*in2));
    int32 gain = 0x40000000u;

    in1 ++;
    in2 ++;

    if (ampl > threshold)
    {
      int inShift = __builtin_clz(ampl);
      int inMantQ15 = ((ampl << (inShift - 1))) >> 15;
      int inFracQ15 = inMantQ15 + EAP_INT16_MIN;
      int inExp = - (8 - inShift);
      int inFrac035 = (inFracQ15 * 11469) >> 15;
      int inFrac035minus085 = inFrac035 - 27853;
      int invMantQ15 = ((inFracQ15 * inFrac035minus085) >> 15) - EAP_INT16_MIN;

      gain = EAP_LongMult32x32(
            (inExp < 0) ? invMantQ15 >> -inExp : invMantQ15 << inExp,
            threshold >> 8);
    }

    prevGain += EAP_LongMultPosQ15x32((gain >  prevGain) ? attCoeff : relCoeff,
                                      gain - prevGain);

    *gainOutput = prevGain >> 16;
  }

  instance->m_prevGainQ30 = prevGain;
}

void
EAP_LimiterInt32_UpdateAttackCoeff(EAP_LimiterInt32 *instance, int16 attack)
{
  assert(instance);
  assert(attack >= 0);

  instance->m_attCoeff = attack;
}

void
EAP_LimiterInt32_UpdateReleaseCoeff(EAP_LimiterInt32 *instance, int16 release)
{
  assert(instance);
  assert(release >= 0);

  instance->m_relCoeff = release;
}

void
EAP_LimiterInt32_UpdateThreshold(EAP_LimiterInt32 *instance, int32 threshold)
{
  assert(instance);
  assert(threshold > 0);

  instance->m_threshold = threshold;
}


void
EAP_LimiterInt32_Gain_Scal(int32 const *in1, int32 const *in2,
                           int16 *gainVector,
                           int32 *out1, int32 *out2, int32 loop_count)
{
  int residue = loop_count & 3;
  int32 i = 0;

  for (i = 0; i < (loop_count - residue); i += 4)
  {
    /* I guess NEON would help here */
    out1[0] = EAP_LongMultPosQ14x32(gainVector[0], in1[0]);
    out2[0] = EAP_LongMultPosQ14x32(gainVector[0], in2[0]);

    out1[1] = EAP_LongMultPosQ14x32(gainVector[1], in1[1]);
    out2[1] = EAP_LongMultPosQ14x32(gainVector[1], in2[1]);

    out1[2] = EAP_LongMultPosQ14x32(gainVector[2], in1[2]);
    out2[2] = EAP_LongMultPosQ14x32(gainVector[2], in2[2]);

    out1[3] = EAP_LongMultPosQ14x32(gainVector[3], in1[3]);
    out2[3] = EAP_LongMultPosQ14x32(gainVector[3], in2[3]);

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

      *out1 = EAP_LongMultPosQ14x32(*gainVector, *in1);
      ++in1;
      ++out1;

      *out2 = EAP_LongMultPosQ14x32(*gainVector, *in2);
      ++in2;
      ++out2;

      ++gainVector;
      ++i;
    }
  }
}

void
EAP_LimiterInt32_Process(EAP_LimiterInt32 *instance, int32 *output1,
                         int32 *output2, const int32 *input1,
                         const int32 *input2, int frames)
{
  int framesMemoryToOutput;
  int framesInputToOutput;
  int framesMemoryToMemory;
  int framesInputToMemory;
  int16 *gainVector;

  assert(instance);

  EAP_LimiterInt32_AmplToGain(instance, input1, input2, frames);

  if (instance->m_memSize <= frames)
    framesMemoryToOutput = instance->m_memSize;
  else
    framesMemoryToOutput = frames;

  framesInputToOutput = frames - framesMemoryToOutput;
  framesMemoryToMemory = instance->m_memSize - framesMemoryToOutput;
  framesInputToMemory = instance->m_memSize - framesMemoryToMemory;
  gainVector = instance->m_scratch;

  EAP_LimiterInt32_Gain_Scal(instance->m_lookaheadMem1,
                             instance->m_lookaheadMem2,
                             instance->m_scratch,
                             output1,
                             output2,
                             framesMemoryToOutput);
  EAP_LimiterInt32_Gain_Scal(input1, input2, &gainVector[framesMemoryToOutput],
                             &output1[framesMemoryToOutput],
                             &output2[framesMemoryToOutput],
                             framesInputToOutput);

  memmove(instance->m_lookaheadMem1,
          &instance->m_lookaheadMem1[framesMemoryToOutput],
          4 * framesMemoryToMemory);
  memmove(instance->m_lookaheadMem2,
          &instance->m_lookaheadMem2[framesMemoryToOutput],
          4 * framesMemoryToMemory);
  memcpy(&instance->m_lookaheadMem1[framesMemoryToMemory],
         &input1[framesInputToOutput],
         4 * framesInputToMemory);
  memcpy(&instance->m_lookaheadMem2[framesMemoryToMemory],
        &input2[framesInputToOutput],
        4 * framesInputToMemory);
}
