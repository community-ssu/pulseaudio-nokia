#include <assert.h>

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

void
EAP_MdrcDelaysAndGainsInt32_Process(EAP_MdrcDelaysAndGainsInt32 *instance, int32 *leftLowOutput, int32 *rightLowOutput, int32 *leftHighOutput, int32 *rightHighOutput, int32 *const *leftLowInputs, int32 *const *rightLowInputs, const int32 *leftHighInput, const int32 *rightHighInput, int32 *const *gainInputs, int frames)
{
	//todo
}

void
EAP_MdrcDelaysAndGainsInt32_Gain_Scal1(int32 const *in1, int32 const *in2,
                                      int32 const *gainVector,
                                      int32 *out1, int32 *out2,
                                      int32 loop_count)
{
	//todo
}
