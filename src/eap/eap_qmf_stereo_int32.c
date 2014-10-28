#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#include "eap_long_multiplications.h"
#endif

#include "eap_qmf_stereo_int32.h"

void
EAP_QmfStereoInt32_Init(EAP_QmfStereoInt32 *qmf, int16 coeff01, int16 coeff02,
                        int16 coeff11, int16 coeff12)
{
  qmf->m_leftAnalysisFilter0.m_mem1 = 0;
  qmf->m_leftAnalysisFilter0.m_mem2 = 0;
  qmf->m_leftAnalysisFilter1.m_mem1 = 0;
  qmf->m_leftAnalysisFilter1.m_mem2 = 0;
  qmf->m_rightAnalysisFilter0.m_mem1 = 0;
  qmf->m_rightAnalysisFilter0.m_mem2 = 0;
  qmf->m_rightAnalysisFilter1.m_mem1 = 0;
  qmf->m_rightAnalysisFilter1.m_mem2 = 0;
  qmf->m_leftSynthesisFilter0.m_mem1 = 0;
  qmf->m_leftSynthesisFilter0.m_mem2 = 0;
  qmf->m_leftSynthesisFilter1.m_mem1 = 0;
  qmf->m_leftSynthesisFilter1.m_mem2 = 0;
  qmf->m_rightSynthesisFilter0.m_mem1 = 0;
  qmf->m_rightSynthesisFilter0.m_mem2 = 0;
  qmf->m_rightSynthesisFilter1.m_mem1 = 0;
  qmf->m_rightSynthesisFilter1.m_mem2 = 0;
  qmf->m_prevInputSampleCount = 0;
  qmf->m_analysisOdd = 0;
  qmf->m_prevOutputSampleCount = 0;
  qmf->m_synthesisOdd = 0;
  qmf->m_leftAnalysisMem = 0;
  qmf->m_rightAnalysisMem = 0;
  qmf->m_leftSynthesisMem = 0;
  qmf->m_rightSynthesisMem = 0;
  qmf->m_coeff01 = coeff01;
  qmf->m_coeff02 = coeff02;
  qmf->m_coeff11 = coeff11;
  qmf->m_coeff12 = coeff12;
}

#ifndef __ARM_NEON__
static int32
AllpassUpdate(int16 K1, int16 K2, int32 *mem1, int32 *mem2, int32 input)
{
  int32 temp;
  int32 output;

  temp = input - EAP_LongMultPosQ15x32(K2, *mem2);
  output = EAP_LongMultPosQ15x32(K2, temp) + *mem2;
  temp = temp - EAP_LongMultPosQ15x32(K1, *mem1);

  *mem2 = *mem1 + EAP_LongMultPosQ15x32(K1, temp);
  *mem1 = temp;

  return output;
}
#endif

int
EAP_QmfStereoInt32_Analyze(EAP_QmfStereoInt32 *instance,
                           int32 *leftLowOutput,
                           int32 *rightLowOutput,
                           int32 *leftHighOutput,
                           int32 *rightHighOutput,
                           const int32 *leftInput,
                           const int32 *rightInput,
                           int inputSampleCount)
{
  int32 outputSampleCount =
      (inputSampleCount + 1 - instance->m_analysisOdd) >> 1;
#ifdef __ARM_NEON__
  int32 Ktmp;
  int32x4_t K;
  int32x4_t MEM1;
  int32x4_t MEM2;
  int32x4_t OUT_HIGH, OUT_LOW, V;
  int32x4_t K1, K2;
  int32 i = 0;

  Ktmp = instance->m_coeff01;
  K = vsetq_lane_s32(Ktmp, K, 0);
  K = vsetq_lane_s32(Ktmp, K, 1);

  Ktmp = instance->m_coeff11;
  K = vsetq_lane_s32(Ktmp, K, 2);
  K = vsetq_lane_s32(Ktmp, K, 3);

  K1 = vshlq_n_s32(K, 16);

  Ktmp = instance->m_coeff02;
  K = vsetq_lane_s32(Ktmp, K, 0);
  K = vsetq_lane_s32(Ktmp, K, 1);

  Ktmp = instance->m_coeff12;
  K = vsetq_lane_s32(Ktmp, K, 2);
  K = vsetq_lane_s32(Ktmp, K, 3);

  K2 = vshlq_n_s32(K, 16);

  MEM1 = vsetq_lane_s32(instance->m_leftAnalysisFilter0.m_mem1, MEM1, 0);
  MEM1 = vsetq_lane_s32(instance->m_rightAnalysisFilter0.m_mem1, MEM1, 1);
  MEM1 = vsetq_lane_s32(instance->m_leftAnalysisFilter1.m_mem1, MEM1, 2);
  MEM1 = vsetq_lane_s32(instance->m_rightAnalysisFilter1.m_mem1, MEM1, 3);

  MEM2 = vsetq_lane_s32(instance->m_leftAnalysisFilter0.m_mem2, MEM2, 0);
  MEM2 = vsetq_lane_s32(instance->m_rightAnalysisFilter0.m_mem2, MEM2, 1);
  MEM2 = vsetq_lane_s32(instance->m_leftAnalysisFilter1.m_mem2, MEM2, 2);
  MEM2 = vsetq_lane_s32(instance->m_rightAnalysisFilter1.m_mem2, MEM2, 3);

  if (!instance->m_analysisOdd)
  {
    OUT_LOW = vsetq_lane_s32(*leftInput ++, OUT_LOW, 0);
    OUT_LOW = vsetq_lane_s32(*rightInput ++, OUT_LOW, 1);
    OUT_LOW = vsetq_lane_s32(instance->m_leftAnalysisMem, OUT_LOW, 2);
    OUT_LOW = vsetq_lane_s32(instance->m_rightAnalysisMem, OUT_LOW, 3);

    OUT_LOW = vsubq_s32(OUT_LOW, vqdmulhq_s32(K2, MEM2));
    OUT_HIGH = vaddq_s32(vqdmulhq_s32(K2, OUT_LOW), MEM2);
    OUT_LOW = vsubq_s32(OUT_LOW, vqdmulhq_s32(K1, MEM1));
    MEM2 = vqdmulhq_s32(K1, OUT_LOW);
    OUT_HIGH = vshrq_n_s32(OUT_HIGH, 1);
    MEM2 = vaddq_s32(MEM2, MEM1);
    V = vreinterpretq_s32_s8(
          vextq_s8(vreinterpretq_s8_s32(OUT_HIGH),
                   vreinterpretq_s8_s32(OUT_LOW), 8));
    MEM1 = OUT_LOW;
    OUT_LOW = vaddq_s32(OUT_HIGH, V);
    OUT_HIGH = vsubq_s32(OUT_HIGH, V);

    vst1q_lane_s32(leftLowOutput ++, OUT_LOW, 0);
    vst1q_lane_s32(rightLowOutput ++, OUT_LOW, 1);
    vst1q_lane_s32(leftHighOutput ++, OUT_HIGH, 0);
    vst1q_lane_s32(rightHighOutput ++, OUT_HIGH, 1);

    i = 1;
  }

  while (i < outputSampleCount)
  {
    OUT_LOW = vsetq_lane_s32(*leftInput ++, OUT_LOW, 2);
    OUT_LOW = vsetq_lane_s32(*rightInput ++, OUT_LOW, 3);
    OUT_LOW = vsetq_lane_s32(*leftInput ++,  OUT_LOW, 0);
    OUT_LOW = vsetq_lane_s32(*rightInput ++, OUT_LOW, 1);

    OUT_LOW = vsubq_s32(OUT_LOW, vqdmulhq_s32(K2, MEM2));

    V = vqdmulhq_s32(K2, OUT_LOW);

    OUT_LOW = vsubq_s32(OUT_LOW, vqdmulhq_s32(K1, MEM1));

    OUT_HIGH = vshrq_n_s32(vaddq_s32(V, MEM2), 1);

    MEM2 = vaddq_s32(vqdmulhq_s32(K1, OUT_LOW), MEM1);

    V = vreinterpretq_s32_s8(
          vextq_s8(vreinterpretq_s8_s32(OUT_HIGH),
                   vreinterpretq_s8_s32(OUT_LOW), 8));

    MEM1 = OUT_LOW;

    OUT_LOW = vaddq_s32(OUT_HIGH, V);
    OUT_HIGH = vsubq_s32(OUT_HIGH, V);

    vst1q_lane_s32(leftLowOutput ++, OUT_LOW, 0);
    vst1q_lane_s32(rightLowOutput ++, OUT_LOW, 1);
    vst1q_lane_s32(leftHighOutput ++, OUT_HIGH, 0);
    vst1q_lane_s32(rightHighOutput ++, OUT_HIGH, 1);

    i ++;
  }

  instance->m_leftAnalysisFilter0.m_mem1 = vgetq_lane_s32(MEM1, 0);
  instance->m_rightAnalysisFilter0.m_mem1 = vgetq_lane_s32(MEM1, 1);
  instance->m_leftAnalysisFilter1.m_mem1 = vgetq_lane_s32(MEM1, 2);
  instance->m_rightAnalysisFilter1.m_mem1 = vgetq_lane_s32(MEM1, 3);

  instance->m_leftAnalysisFilter0.m_mem2 = vgetq_lane_s32(MEM2, 0);
  instance->m_rightAnalysisFilter0.m_mem2 = vgetq_lane_s32(MEM2, 1);
  instance->m_leftAnalysisFilter1.m_mem2 = vgetq_lane_s32(MEM2, 2);
  instance->m_rightAnalysisFilter1.m_mem2 = vgetq_lane_s32(MEM2, 3);

  if (inputSampleCount > 0)
  {
    instance->m_leftAnalysisMem = *leftInput;
    instance->m_rightAnalysisMem = *rightInput;
  }
#else
  int32 leftMem1 = instance->m_leftAnalysisFilter0.m_mem1;
  int32 leftMem2 = instance->m_leftAnalysisFilter0.m_mem2;
  int32 rightMem1 = instance->m_rightAnalysisFilter0.m_mem1;
  int32 rightMem2 = instance->m_rightAnalysisFilter0.m_mem2;
  int32 K1 = instance->m_coeff01;
  int32 K2 = instance->m_coeff02;
  int32 i = instance->m_analysisOdd;
  int32 o = 0;

  while (i < inputSampleCount)
  {
    int32 outputL = AllpassUpdate(K1, K2, &leftMem1,
                                  &leftMem2, leftInput[i]);
    int32 outputR = AllpassUpdate(K1, K2, &rightMem1,
                                  &rightMem2, rightInput[i]);

    leftLowOutput[o] = leftHighOutput[o] = outputL >> 1;
    rightLowOutput[o] = rightHighOutput[o] = outputR >> 1;

    i += 2;
    o ++;
  }

  instance->m_leftAnalysisFilter0.m_mem1 = leftMem1;
  instance->m_leftAnalysisFilter0.m_mem2 = leftMem2;
  instance->m_rightAnalysisFilter0.m_mem1 = rightMem1;
  instance->m_rightAnalysisFilter0.m_mem2 = rightMem2;

  leftMem1 = instance->m_leftAnalysisFilter1.m_mem1;
  leftMem2 = instance->m_leftAnalysisFilter1.m_mem2;
  rightMem1 = instance->m_rightAnalysisFilter1.m_mem1;
  rightMem2 = instance->m_rightAnalysisFilter1.m_mem2;
  K1 = instance->m_coeff11;
  K2 = instance->m_coeff12;

  if (!instance->m_analysisOdd && inputSampleCount)
  {
    int32 outputL = AllpassUpdate(K1, K2, &leftMem1, &leftMem2,
                                  instance->m_leftAnalysisMem) >> 1;
    int32 outputR = AllpassUpdate(K1, K2, &rightMem1, &rightMem2,
                                  instance->m_rightAnalysisMem) >> 1;

    leftLowOutput[0] += outputL;
    leftHighOutput[0] -= outputL;

    rightLowOutput[0] += outputR;
    rightHighOutput[0] -= outputR;
  }

  o = i = 1 - instance->m_analysisOdd;

  while (o < outputSampleCount)
  {
    int32 outputL = AllpassUpdate(K1, K2, &leftMem1, &leftMem2,
                                  leftInput[i]) >> 1;
    int32 outputR = AllpassUpdate(K1, K2, &rightMem1, &rightMem2,
                                  rightInput[i]) >> 1;

    leftLowOutput[o] += outputL;
    leftHighOutput[o] -= outputL;

    rightLowOutput[o] += outputR;
    rightHighOutput[o] -= outputR;

    i += 2;
    o ++;
  }

  instance->m_leftAnalysisFilter1.m_mem1 = leftMem1;
  instance->m_leftAnalysisFilter1.m_mem2 = leftMem2;
  instance->m_rightAnalysisFilter1.m_mem1 = rightMem1;
  instance->m_rightAnalysisFilter1.m_mem2 = rightMem2;

  if (inputSampleCount > 0)
  {
    instance->m_leftAnalysisMem = leftInput[inputSampleCount - 1];
    instance->m_rightAnalysisMem = rightInput[inputSampleCount - 1];
  }
#endif

  instance->m_analysisOdd =
      (inputSampleCount + instance->m_analysisOdd) & 1;
  instance->m_prevInputSampleCount = inputSampleCount;
  instance->m_prevOutputSampleCount = outputSampleCount;

  return outputSampleCount;
}

int
EAP_QmfStereoInt32_MaxOutputSampleCount(int inSamples)
{
  return (inSamples + 1) >> 1;
}

void
EAP_QmfStereoInt32_Resynthesize(EAP_QmfStereoInt32 *instance,
                                int32 *leftOutput,
                                int32 *rightOutput,
                                const int32 *leftLowInput,
                                const int32 *rightLowInput,
                                const int32 *leftHighInput,
                                const int32 *rightHighInput)
{
#ifdef __ARM_NEON__
  int32 Ktmp;
  int32x4_t K;
  int32x4_t MEM1, MEM2, IN, OUT;
  int32x4_t K1, K2, tmp;
  int32x2_t lowInput, highInput;
  int32 i = 0;

  Ktmp = instance->m_coeff01;
  K = vsetq_lane_s32(Ktmp, K, 0);
  K = vsetq_lane_s32(Ktmp, K, 1);

  Ktmp = instance->m_coeff11;
  K = vsetq_lane_s32(Ktmp, K, 2);
  K = vsetq_lane_s32(Ktmp, K, 3);

  K1 = vshlq_n_s32(K, 16);

  Ktmp = instance->m_coeff02;
  K = vsetq_lane_s32(Ktmp, K, 0);
  K = vsetq_lane_s32(Ktmp, K, 1);

  Ktmp = instance->m_coeff12;
  K = vsetq_lane_s32(Ktmp, K, 2);
  K = vsetq_lane_s32(Ktmp, K, 3);

  K2 = vshlq_n_s32(K, 16);

  MEM2 = vsetq_lane_s32(instance->m_leftSynthesisFilter1.m_mem2, MEM2, 2);
  MEM2 = vsetq_lane_s32(instance->m_rightSynthesisFilter1.m_mem2, MEM2, 3);

  MEM1 = vsetq_lane_s32(instance->m_leftSynthesisFilter1.m_mem1, MEM1, 2);
  MEM1 = vsetq_lane_s32(instance->m_rightSynthesisFilter1.m_mem1, MEM1, 3);

  if (instance->m_synthesisOdd)
  {
    OUT = vsetq_lane_s32(instance->m_leftSynthesisMem, OUT, 2);
    OUT = vsetq_lane_s32(instance->m_rightSynthesisMem, OUT, 3);

    OUT = vsubq_s32(OUT, vqdmulhq_s32(K2, MEM2));
    tmp = vsubq_s32(OUT, vqdmulhq_s32(K1, MEM1));
    OUT = vaddq_s32(vqdmulhq_s32(K2, OUT), MEM2);

    vst1q_lane_s32(leftOutput ++, OUT, 2);
    vst1q_lane_s32(rightOutput ++, OUT, 3);

    MEM2 = vaddq_s32(vqdmulhq_s32(K1, tmp), MEM1);

    tmp = vsetq_lane_s32(instance->m_leftSynthesisFilter0.m_mem1, tmp, 0);
    tmp = vsetq_lane_s32(instance->m_rightSynthesisFilter0.m_mem1, tmp, 1);

    MEM2 = vsetq_lane_s32(instance->m_leftSynthesisFilter0.m_mem2, MEM2, 0);
    MEM2 = vsetq_lane_s32(instance->m_rightSynthesisFilter0.m_mem2, MEM2, 1);
    i = instance->m_synthesisOdd;

    while (i < instance->m_prevInputSampleCount - 2)
    {
      highInput = vset_lane_s32(*leftHighInput ++, highInput, 0);
      highInput = vset_lane_s32(*rightHighInput ++, highInput, 1);
      lowInput = vset_lane_s32(*leftLowInput ++, lowInput, 0);
      lowInput = vset_lane_s32(*rightLowInput ++, lowInput, 1);

      IN = vsubq_s32(vcombine_s32(vsub_s32(lowInput, highInput),
                                  vadd_s32(lowInput, highInput)),
                     vqdmulhq_s32(K2, MEM2));
      OUT = vaddq_s32(vqdmulhq_s32(K2, IN), MEM2);

      vst1q_lane_s32(leftOutput ++, OUT, 0);
      vst1q_lane_s32(rightOutput ++, OUT, 1);
      vst1q_lane_s32(leftOutput ++, OUT, 2);
      vst1q_lane_s32(rightOutput ++, OUT, 3);

      IN = vsubq_s32(IN, vqdmulhq_s32(K1, tmp));
      MEM2 = vaddq_s32(vqdmulhq_s32(K1, IN), tmp);
      tmp = IN;

      i += 2;
    }

    if (instance->m_prevInputSampleCount & 1)
    {
      highInput = vset_lane_s32(*leftHighInput, highInput, 0);
      highInput = vset_lane_s32(*rightHighInput, highInput, 1);
      lowInput = vset_lane_s32(*leftLowInput, lowInput, 0);
      lowInput = vset_lane_s32(*rightLowInput, lowInput, 1);

      OUT = vsubq_s32(vcombine_s32(vsub_s32(lowInput, highInput),
                                   vadd_s32(lowInput, highInput)),
                     vqdmulhq_s32(K2, MEM2));
      K2 = vaddq_s32(vqdmulhq_s32(K2, OUT), MEM2);
      OUT = vsubq_s32(OUT, vqdmulhq_s32(K1, tmp));
      K1 = vaddq_s32(vqdmulhq_s32(K1, OUT), tmp);

      vst1q_lane_s32(leftOutput ++, K2, 0);
      vst1q_lane_s32(rightOutput ++, K2, 1);
      vst1q_lane_s32(leftOutput, K2, 2);
      vst1q_lane_s32(rightOutput, K2, 3);

      instance->m_leftSynthesisFilter1.m_mem1 = vgetq_lane_s32(OUT, 2);
      instance->m_rightSynthesisFilter1.m_mem1 = vgetq_lane_s32(OUT, 3);
      instance->m_leftSynthesisFilter1.m_mem2 = vgetq_lane_s32(K1, 2);
      instance->m_rightSynthesisFilter1.m_mem2 = vgetq_lane_s32(K1, 3);
    }
    else
    {
      instance->m_leftSynthesisFilter1.m_mem1 = vgetq_lane_s32(tmp, 2);
      instance->m_rightSynthesisFilter1.m_mem1 = vgetq_lane_s32(tmp, 3);
      instance->m_leftSynthesisFilter1.m_mem2 = vgetq_lane_s32(MEM2, 2);
      instance->m_rightSynthesisFilter1.m_mem2 = vgetq_lane_s32(MEM2, 3);

      highInput = vset_lane_s32(*leftHighInput, highInput, 0);
      highInput = vset_lane_s32(*rightHighInput, highInput, 1);
      lowInput = vset_lane_s32(*leftLowInput, lowInput, 0);
      lowInput = vset_lane_s32(*rightLowInput, lowInput, 1);

      OUT = vsubq_s32(vsubq_s32(vcombine_s32(vsub_s32(lowInput, highInput),
                                             highInput),
                                vqdmulhq_s32(K2, MEM2)), vqdmulhq_s32(K1, tmp));
      K2 = vaddq_s32(vqdmulhq_s32(K2, OUT), MEM2);
      K1 = vaddq_s32(vqdmulhq_s32(K1, OUT), tmp);

      vst1q_lane_s32(leftOutput, K2, 0);
      vst1q_lane_s32(rightOutput, K2, 1);
    }

    instance->m_leftSynthesisFilter0.m_mem1 = vgetq_lane_s32(OUT, 0);
    instance->m_rightSynthesisFilter0.m_mem1 = vgetq_lane_s32(OUT, 1);
    instance->m_leftSynthesisFilter0.m_mem2 = vgetq_lane_s32(K1, 0);
    instance->m_rightSynthesisFilter0.m_mem2 = vgetq_lane_s32(K1, 1);
  }
  else
  {
    MEM1 = vsetq_lane_s32(instance->m_leftSynthesisFilter0.m_mem1, MEM1, 0);
    MEM1 = vsetq_lane_s32(instance->m_rightSynthesisFilter0.m_mem1, MEM1, 1);
    MEM2 = vsetq_lane_s32(instance->m_leftSynthesisFilter0.m_mem2, MEM2, 0);
    MEM2 = vsetq_lane_s32(instance->m_rightSynthesisFilter0.m_mem2, MEM2, 1);

    while (instance->m_prevInputSampleCount - 1 > i)
    {
      highInput = vset_lane_s32(*leftHighInput ++, highInput, 0);
      highInput = vset_lane_s32(*rightHighInput ++, highInput, 1);
      lowInput = vset_lane_s32(*leftLowInput ++, lowInput, 0);
      lowInput = vset_lane_s32(*rightLowInput ++, lowInput, 1);

      OUT = vsubq_s32(vcombine_s32(vsub_s32(lowInput, highInput),
                                   vadd_s32(lowInput, highInput)),
                      vqdmulhq_s32(K2, MEM2));
      tmp = vsubq_s32(OUT, vqdmulhq_s32(K1, MEM1));
      OUT = vaddq_s32(vqdmulhq_s32(K2, OUT), MEM2);

      vst1q_lane_s32(leftOutput ++, OUT, 0);
      MEM2 = vqdmulhq_s32(K1, tmp);
      vst1q_lane_s32(rightOutput ++, OUT, 1);
      MEM2 = vaddq_s32(MEM2, MEM1);
      vst1q_lane_s32(leftOutput ++, OUT, 2);
      MEM1 = tmp;
      vst1q_lane_s32(rightOutput ++, OUT, 3);


      i += 2;
    }

    instance->m_leftSynthesisFilter1.m_mem1 = vgetq_lane_s32(MEM1, 2);
    instance->m_rightSynthesisFilter1.m_mem1 = vgetq_lane_s32(MEM1, 3);
    instance->m_leftSynthesisFilter1.m_mem2 = vgetq_lane_s32(MEM2, 2);
    instance->m_rightSynthesisFilter1.m_mem2 = vgetq_lane_s32(MEM2, 3);

    if (instance->m_prevInputSampleCount & 1)
    {
      highInput = vset_lane_s32(*leftHighInput, highInput, 0);
      highInput = vset_lane_s32(*rightHighInput, highInput, 1);
      lowInput = vset_lane_s32(*leftLowInput, lowInput, 0);
      lowInput = vset_lane_s32(*rightLowInput, lowInput, 1);

      OUT = vsubq_s32(vcombine_s32(vsub_s32(lowInput, highInput),
                                   vadd_s32(lowInput, highInput)),
                      vqdmulhq_s32(K2, MEM2));
      tmp = vsubq_s32(OUT, vqdmulhq_s32(K1, MEM1));
      OUT = vaddq_s32(vqdmulhq_s32(K2, OUT), MEM2);
      MEM2 = vqdmulhq_s32(K1, tmp);

      vst1q_lane_s32(leftOutput, OUT, 0);
      vst1q_lane_s32(rightOutput, OUT, 1);

      MEM2 = vaddq_s32(MEM2, MEM1);
      MEM1 = tmp;
    }

    instance->m_leftSynthesisFilter0.m_mem1 = vgetq_lane_s32(MEM1, 0);
    instance->m_rightSynthesisFilter0.m_mem1 = vgetq_lane_s32(MEM1, 1);
    instance->m_leftSynthesisFilter0.m_mem2 = vgetq_lane_s32(MEM2, 0);
    instance->m_rightSynthesisFilter0.m_mem2 = vgetq_lane_s32(MEM2, 1);
  }

  if (instance->m_prevOutputSampleCount > 0)
  {
    instance->m_leftSynthesisMem = *leftLowInput + *leftHighInput;
    instance->m_rightSynthesisMem = *rightLowInput + *rightHighInput;;
  }
#else
  int32 leftMem1 = instance->m_leftSynthesisFilter0.m_mem1;
  int32 leftMem2 = instance->m_leftSynthesisFilter0.m_mem2;
  int32 rightMem1 = instance->m_rightSynthesisFilter0.m_mem1;
  int32 rightMem2 = instance->m_rightSynthesisFilter0.m_mem2;
  int16 K1 = instance->m_coeff01;
  int16 K2 = instance->m_coeff02;
  int i = 0;
  int o = 0;

  for (o = instance->m_synthesisOdd; instance->m_prevInputSampleCount > o;
       o += 2)
  {
    int32 leftInput = leftLowInput[i] - leftHighInput[i];
    int32 rightInput = rightLowInput[i] - rightHighInput[i];

    leftOutput[o] = AllpassUpdate(K1, K2, &leftMem1, &leftMem2, leftInput);
    rightOutput[o] = AllpassUpdate(K1, K2, &rightMem1, &rightMem2, rightInput);

    i ++;
  }

  instance->m_leftSynthesisFilter0.m_mem1 = leftMem1;
  instance->m_leftSynthesisFilter0.m_mem2 = leftMem2;
  instance->m_rightSynthesisFilter0.m_mem1 = rightMem1;
  instance->m_rightSynthesisFilter0.m_mem2 = rightMem2;


  leftMem1 = instance->m_leftSynthesisFilter1.m_mem1;
  leftMem2 = instance->m_leftSynthesisFilter1.m_mem2;
  rightMem1 = instance->m_rightSynthesisFilter1.m_mem1;
  rightMem2 = instance->m_rightSynthesisFilter1.m_mem2;

  K1 = instance->m_coeff11;
  K2 = instance->m_coeff12;

  if (instance->m_synthesisOdd && instance->m_prevInputSampleCount)
  {
    *leftOutput = AllpassUpdate(K1, K2, &leftMem1, &leftMem2,
                                instance->m_leftSynthesisMem);
    *rightOutput = AllpassUpdate(K1, K2, &rightMem1, &rightMem2,
                                 instance->m_rightSynthesisMem);
  }

  i = 0;

  for (o = instance->m_synthesisOdd + 1; instance->m_prevInputSampleCount > o;
       o += 2)
  {
    int32 leftInput = leftLowInput[i] + leftHighInput[i];
    int32 rightInput = rightLowInput[i] + rightHighInput[i];

    leftOutput[o] = AllpassUpdate(K1, K2, &leftMem1, &leftMem2, leftInput);
    rightOutput[o] = AllpassUpdate(K1, K2, &rightMem1, &rightMem2, rightInput);
    i ++;
  }

  instance->m_leftSynthesisFilter1.m_mem1 = leftMem1;
  instance->m_leftSynthesisFilter1.m_mem2 = leftMem2;
  instance->m_rightSynthesisFilter1.m_mem1 = rightMem1;
  instance->m_rightSynthesisFilter1.m_mem2 = rightMem2;

  if (instance->m_prevOutputSampleCount > 0)
  {
    instance->m_leftSynthesisMem =
        leftHighInput[instance->m_prevOutputSampleCount - 1] +
        leftLowInput[instance->m_prevOutputSampleCount - 1];
    instance->m_rightSynthesisMem =
        rightHighInput[instance->m_prevOutputSampleCount - 1] +
        rightLowInput[instance->m_prevOutputSampleCount - 1];
  }
#endif

  instance->m_synthesisOdd =
      (instance->m_synthesisOdd + instance->m_prevInputSampleCount) & 1;
}

#ifdef EAP_QMF_STEREO_INT32_TEST
/*
CPU@500MHz, gcc 4.7.2

Verifying that EAP_QmfStereoInt32_XXX output matches to stock
DONE.
Timing EAP_QmfStereoInt32_XXX with 3 x 30000 runs, 1024 samples
Stock Nokia:
msecs: 9330
msecs: 9300
msecs: 9290
FOSS replacement:
msecs: 8850
msecs: 8910
msecs: 8860

*/
#include <assert.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef int (*EAP_QmfStereoInt32_AnalyzeFptf)(EAP_QmfStereoInt32 *instance,
                           int32 *leftLowOutput,
                           int32 *rightLowOutput,
                           int32 *leftHighOutput,
                           int32 *rightHighOutput,
                           const int32 *leftInput,
                           const int32 *rightInput,
                           int inputSampleCount);

typedef void (*EAP_QmfStereoInt32_ResynthesizeFptf)(EAP_QmfStereoInt32 *instance,
                                int32 *leftOutput,
                                int32 *rightOutput,
                                const int32 *leftLowInput,
                                const int32 *rightLowInput,
                                const int32 *leftHighInput,
                                const int32 *rightHighInput);

#define timing(a) \
{ \
  clock_t start=clock(), diff; \
  int msec; \
  do { \
    a; \
  } \
  while(0); \
  diff = clock() - start; \
  msec = diff * 1000 / CLOCKS_PER_SEC; \
  printf("msecs: %d\n",msec); \
}

#define SAMPLES 1024
#define BANDS 5
#define BENCHCNT 30000

int32 a[BANDS][SAMPLES] __attribute__((aligned(4096)));
int32 b[BANDS][SAMPLES] __attribute__((aligned(4096)));

int32 an[BANDS][SAMPLES] __attribute__((aligned(4096)));
int32 bn[BANDS][SAMPLES] __attribute__((aligned(4096)));

int32 *leftLowOutputBuffers[BANDS] __attribute__((aligned(4096)));
int32 *rightLowOutputBuffers[BANDS] __attribute__((aligned(4096)));
int32 *leftLowOutputBuffersn[BANDS] __attribute__((aligned(4096)));
int32 *rightLowOutputBuffersn[BANDS] __attribute__((aligned(4096)));
int32 leftHighOutput[SAMPLES] __attribute__((aligned(4096)));
int32 rightHighOutput[SAMPLES] __attribute__((aligned(4096)));
int32 leftHighOutputn[SAMPLES] __attribute__((aligned(4096)));
int32 rightHighOutputn[SAMPLES] __attribute__((aligned(4096)));
int32 lowInput[SAMPLES] __attribute__((aligned(4096)));

void initOutput()
{
  int i, j;

  for (i = 0; i < BANDS; i++)
  {
    for (j = 0; j < SAMPLES; j ++)
    {
      a[i][j] = an[i][j] = (j % 3) ? j : -j;
      b[i][j] = bn[i][j] = (j % 3) ? j : -j;
    }

    leftLowOutputBuffers[i] = a[i];
    rightLowOutputBuffers[i] = b[i];

    leftLowOutputBuffersn[i] = an[i];;
    rightLowOutputBuffersn[i] = bn[i];;
  }

  for (i = 0; i < SAMPLES; i++)
  {
    leftHighOutput[i] = rightHighOutput[i] =
        leftHighOutputn[i] = rightHighOutputn[i] = 1;
    lowInput[i] = i;
  }
}

int main()
{
  int i, j;

  printf("Verifying that EAP_QmfStereoInt32_XXX output matches to stock\n");

  initOutput();

  void *h = dlopen("/usr/lib/pulse-0.9.15/modules/module-nokia-record.so", RTLD_LAZY);

  EAP_QmfStereoInt32 qmf, qmfn;
  EAP_QmfStereoInt32_Init(&qmf, 112, 22, 11, 12);
  EAP_QmfStereoInt32_Init(&qmfn, 112, 22, 11, 12);

  EAP_QmfStereoInt32_AnalyzeFptf analyze =
      (EAP_QmfStereoInt32_AnalyzeFptf)dlsym(h,"EAP_QmfStereoInt32_Analyze");

  EAP_QmfStereoInt32_ResynthesizeFptf resyntesize =
      (EAP_QmfStereoInt32_ResynthesizeFptf)dlsym(h,"EAP_QmfStereoInt32_Resynthesize");

  analyze(&qmfn,
          leftLowOutputBuffersn[0],
          rightLowOutputBuffersn[0],
          leftHighOutputn,
          rightHighOutputn,
          lowInput,
          lowInput,
          SAMPLES - 1);

  resyntesize(&qmfn,
              leftLowOutputBuffersn[0],
              rightLowOutputBuffersn[0],
              leftHighOutputn,
              rightHighOutputn,
              lowInput,
              lowInput);

  analyze(&qmfn,
          leftLowOutputBuffersn[0],
          rightLowOutputBuffersn[0],
          leftHighOutputn,
          rightHighOutputn,
          lowInput,
          lowInput,
          SAMPLES);

  resyntesize(&qmfn,
              leftLowOutputBuffersn[0],
              rightLowOutputBuffersn[0],
              leftHighOutputn,
              rightHighOutputn,
              lowInput,
              lowInput);

  analyze(&qmfn,
          leftLowOutputBuffersn[0],
          rightLowOutputBuffersn[0],
          leftHighOutputn,
          rightHighOutputn,
          lowInput,
          lowInput,
          SAMPLES - 1);

  resyntesize(&qmfn,
              leftLowOutputBuffersn[0],
              rightLowOutputBuffersn[0],
              leftHighOutputn,
              rightHighOutputn,
              lowInput,
              lowInput);

  EAP_QmfStereoInt32_Analyze(&qmf,
                             leftLowOutputBuffers[0],
                             rightLowOutputBuffers[0],
                             leftHighOutput,
                             rightHighOutput,
                             lowInput,
                             lowInput,
                             SAMPLES-1);

  EAP_QmfStereoInt32_Resynthesize(&qmf,
                                  leftLowOutputBuffers[0],
                                  rightLowOutputBuffers[0],
                                  leftHighOutput,
                                  rightHighOutput,
                                  lowInput,
                                  lowInput);

  EAP_QmfStereoInt32_Analyze(&qmf,
                             leftLowOutputBuffers[0],
                             rightLowOutputBuffers[0],
                             leftHighOutput,
                             rightHighOutput,
                             lowInput,
                             lowInput,
                             SAMPLES);

  EAP_QmfStereoInt32_Resynthesize(&qmf,
                                  leftLowOutputBuffers[0],
                                  rightLowOutputBuffers[0],
                                  leftHighOutput,
                                  rightHighOutput,
                                  lowInput,
                                  lowInput);

  EAP_QmfStereoInt32_Analyze(&qmf,
                             leftLowOutputBuffers[0],
                             rightLowOutputBuffers[0],
                             leftHighOutput,
                             rightHighOutput,
                             lowInput,
                             lowInput,
                             SAMPLES - 1);

  EAP_QmfStereoInt32_Resynthesize(&qmf,
                                  leftLowOutputBuffers[0],
                                  rightLowOutputBuffers[0],
                                  leftHighOutput,
                                  rightHighOutput,
                                  lowInput,
                                  lowInput);


  if(memcmp(&qmf, &qmfn, sizeof(qmf)))
  {
    printf("qmf differ after EAP_QmfStereoInt32_Resynthesize");
    abort();
  }

  for(j = 0; j < SAMPLES; j++)
  {
    if(leftLowOutputBuffers[0][j] != leftLowOutputBuffersn[0][j])
    {
      printf("leftLowOutputBuffers[%d][%d] differ [%x] - [%x]\n", 0, j,
             leftLowOutputBuffers[0][j], leftLowOutputBuffersn[0][j]);
      abort();
    }
    if(rightLowOutputBuffers[0][j] != rightLowOutputBuffersn[0][j])
    {
      printf("rightLowOutputBuffers[%d][%d] differ [%x] - [%x]\n", 0, j,
             rightLowOutputBuffers[0][j], rightLowOutputBuffersn[0][j]);
      abort();
    }
  }

  for (i = 0; i < SAMPLES; i++)
  {
    if(leftHighOutput[i] != leftHighOutputn[i])
    {
      printf("leftHighOutput[%d] differ [%x] - [%x]\n", i,
             leftHighOutput[i], leftHighOutputn[i]);
      abort();
    }
    if(rightHighOutput[i] != rightHighOutputn[i])
    {
      printf("rightHighOutput[%d] differ [%x] - [%x]\n", i,
             rightHighOutput[i], rightHighOutputn[i]);
      abort();
    }
  }

  printf("DONE.\nTiming EAP_QmfStereoInt32_XXX with 3 x %d runs, %d samples\n", BENCHCNT, SAMPLES);

  EAP_QmfStereoInt32_Init(&qmf, 112, 22, 11, 12);
  EAP_QmfStereoInt32_Init(&qmfn, 112, 22, 11, 12);

  printf("Stock Nokia:\n");
  for (i = 0; i < 3; i++)
  {
      timing(
      for (j = 0; j < BENCHCNT; j++)
      {
        analyze(&qmfn,
                leftLowOutputBuffersn[0],
                rightLowOutputBuffersn[0],
                leftHighOutputn,
                rightHighOutputn,
                lowInput,
                lowInput,
                SAMPLES);

        resyntesize(&qmfn,
                    leftLowOutputBuffersn[0],
                    rightLowOutputBuffersn[0],
                    leftHighOutputn,
                    rightHighOutputn,
                    lowInput,
                    lowInput);

        analyze(&qmfn,
                leftLowOutputBuffersn[0],
                rightLowOutputBuffersn[0],
                leftHighOutputn,
                rightHighOutputn,
                lowInput,
                lowInput,
                SAMPLES);

        resyntesize(&qmfn,
                    leftLowOutputBuffersn[0],
                    rightLowOutputBuffersn[0],
                    leftHighOutputn,
                    rightHighOutputn,
                    lowInput,
                    lowInput);

        analyze(&qmfn,
                leftLowOutputBuffersn[0],
                rightLowOutputBuffersn[0],
                leftHighOutputn,
                rightHighOutputn,
                lowInput,
                lowInput,
                SAMPLES - 1);

        resyntesize(&qmfn,
                    leftLowOutputBuffersn[0],
                    rightLowOutputBuffersn[0],
                    leftHighOutputn,
                    rightHighOutputn,
                    lowInput,
                    lowInput);
      })
  }

  printf("FOSS replacement:\n");

  for (i = 0; i < 3; i++)
  {
      timing(
        for (j = 0; j < BENCHCNT; j++)
      {
        EAP_QmfStereoInt32_Analyze(&qmf,
                                   leftLowOutputBuffersn[0],
                                   rightLowOutputBuffersn[0],
                                   leftHighOutputn,
                                   rightHighOutputn,
                                   lowInput,
                                   lowInput,
                                   SAMPLES);

        EAP_QmfStereoInt32_Resynthesize(&qmf,
                                        leftLowOutputBuffersn[0],
                                        rightLowOutputBuffersn[0],
                                        leftHighOutputn,
                                        rightHighOutputn,
                                        lowInput,
                                        lowInput);

        EAP_QmfStereoInt32_Analyze(&qmf,
                                   leftLowOutputBuffersn[0],
                                   rightLowOutputBuffersn[0],
                                   leftHighOutputn,
                                   rightHighOutputn,
                                   lowInput,
                                   lowInput,
                                   SAMPLES);

        EAP_QmfStereoInt32_Resynthesize(&qmf,
                                        leftLowOutputBuffersn[0],
                                        rightLowOutputBuffersn[0],
                                        leftHighOutputn,
                                        rightHighOutputn,
                                        lowInput,
                                        lowInput);

        EAP_QmfStereoInt32_Analyze(&qmf,
                                   leftLowOutputBuffersn[0],
                                   rightLowOutputBuffersn[0],
                                   leftHighOutputn,
                                   rightHighOutputn,
                                   lowInput,
                                   lowInput,
                                   SAMPLES - 1);

        EAP_QmfStereoInt32_Resynthesize(&qmf,
                                        leftLowOutputBuffersn[0],
                                        rightLowOutputBuffersn[0],
                                        leftHighOutputn,
                                        rightHighOutputn,
                                        lowInput,
                                        lowInput);
      })
  }
}
#endif
