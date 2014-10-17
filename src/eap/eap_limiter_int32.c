#include "eap_limiter_int32.h"
#include "eap_long_multiplications.h"

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

  for ( i = 0; i < memSize; ++i )
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

  for (i = 0; i < frames; i ++)
  {
    int ampl = EAP_MAX(abs(*in1), abs(*in2));
    int32 gain = 0x40000000u;

    in1 ++;
    in2 ++;

    if (ampl > threshold)
    {
      int inShift = EAP_Norm32(ampl);
      int inExp = - (7 - inShift);
      int inMantQ15 = (ampl << inShift) >> 15;
      int inFracQ15 = inMantQ15 - 32768;

      int inverse = inFracQ15 * ((11469 * inFracQ15 - 27853) >> 15) + 32768;

      gain = EAP_LongMult32x32(
            (7 - inShift >= 0) ? inverse >> -inExp : inverse << inExp,
            threshold >> 8);
    }

    prevGain += EAP_LongMultPosQ15x32(
          (gain - prevGain <= 0) ? attCoeff : relCoeff, gain - prevGain);

    *gainOutput = prevGain >> 16;

    gainOutput ++;
  }

  instance->m_prevGainQ30 = prevGain;
}
