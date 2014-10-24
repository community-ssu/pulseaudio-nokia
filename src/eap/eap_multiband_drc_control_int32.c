#include <stdlib.h>
#include <math.h>

#include "eap_multiband_drc_control_int32.h"
#include "eap_clip.h"

int
EAP_MultibandDrcControlInt32_Init(EAP_MultibandDrcControlInt32 *instance,
                                  float sampleRate, int bandCount, int eqCount,
                                  float companderLookahead,
                                  float limiterLookahead,
                                  float controlRate, int maxBlockSize)
{
  return EAP_MultibandDrcControl_Init(
           (EAP_MultibandDrcControl *)instance,
           sampleRate,
           bandCount,
           eqCount,
           companderLookahead,
           limiterLookahead,
           controlRate,
           maxBlockSize);
}

void
EAP_MultibandDrcControlInt32_GetProcessingInitInfo(
    EAP_MultibandDrcControlInt32 *instance,
    EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  initInfo->sampleRate = instance->m_sampleRate;
  initInfo->bandCount = instance->m_bandCount;
  initInfo->companderLookahead = instance->m_companderLookahead * 0.001 * 0.5 *
      instance->m_sampleRate + 0.5;
  initInfo->limiterLookahead =
      instance->m_limiterLookahead * 0.001 * instance->m_sampleRate + 0.5;
  initInfo->downSamplingFactor = instance->m_downSamplingFactor;
  initInfo->maxBlockSize = instance->m_maxBlockSize;

  initInfo->avgShift =
      0.5 - log(1.0 - exp(-1.0 / instance->m_downSamplingFactor)) / log(2.0);
}

static int16
CalcCoeff(float timeConstant, float sampleRate)
{
  return EAP_Clip16(
        (1.0 - exp(-1000.0 / (timeConstant * 0.5 * sampleRate))) * 32768.0);
}

int
EAP_MultibandDrcControlInt32_UpdateCompanderAttack(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompanderAttackCoeffInt32 *event, float attackTimeMs,
    int band)
{
  event->common.type = UpdateCompanderAttack;

  if (band >= 0 && instance->m_bandCount > band)
  {
    event->band = band;
    event->coeff =
        CalcCoeff(attackTimeMs,
                  instance->m_sampleRate * 0.5 * instance->m_oneOverFactor);

    return 0;
  }

  return -1;
}

int
EAP_MultibandDrcControlInt32_UpdateCompanderRelease(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompanderReleaseCoeffInt32 *event,
    float releaseTimeMs, int band)
{
  event->common.type = UpdateCompanderRelease;

  if (band >= 0 && instance->m_bandCount > band)
  {
    event->band = band;
    event->coeff =
        CalcCoeff(releaseTimeMs,
                  instance->m_sampleRate * 0.5 * instance->m_oneOverFactor);

    return 0;
  }

  return -1;
}

int
EAP_MultibandDrcControlInt32_UpdateCrossBandLink(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCrossBandLinkInt32 *event, float link)
{
  if ( link >= 0.0 && link <= 1.0 )
  {
    event->common.type = UpdateCrossBandLink;
    event->selfMult = ((1.0 - link) * 16384.0);
    event->sumMult = (16384.0 / (long double)instance->m_bandCount * link);
    return 0;
  }

  return -1;
}

void
ConvertA(int16 *aFrac, int16 *aExp, float A)
{
  int exponent;
  float mant;

  exponent = 0;
  mant = frexp(A, &exponent);
  mant = mant + mant;
  exponent --;
  *aFrac = EAP_Clip16((signed int)((mant - 1.0) * 32768.0 + 0.5));
  *aExp = exponent;
}

int
CalcCurve(EAP_CompressionCurveImplDataInt32 *curve, const float *inLevels,
              const float *outLevels)
{
  float k;
  float a;
  int i;
  int32 *K = curve->K;
  int16 *AExp = curve->AExp;
  int16 *AFrac = curve->AFrac;

  for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
    curve->levelLimits[i] = pow(10.0, inLevels[i] * 0.05) * 8388608.0;

  K[0] = 0;
  ConvertA(AFrac, AExp, pow(10.0, (outLevels[0] - inLevels[0]) * 0.05));

  for (i = 1; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
  {
    k = (outLevels[i] - outLevels[i - 1]) / (inLevels[i] - inLevels[i - 1]);
    a = outLevels[i - 1] - inLevels[i - 1] * k;
    K[i] = (k - 1.0) * 32768.0 + 0.5;
    ConvertA(&AFrac[i], &AExp[i], pow(10.0, a * 0.05));
  }

  curve->K[5] = -32768;
  ConvertA(&curve->AFrac[EAP_MDRC_MAX_BAND_COUNT],
           &curve->AExp[EAP_MDRC_MAX_BAND_COUNT],
           pow(10.0, outLevels[EAP_MDRC_MAX_BAND_COUNT - 1] * 0.05));

  return 0;
}

int
EAP_MultibandDrcControlInt32_UpdateCompressionCurve(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompressionCurveInt32 *event,
    const float *inputLevels, const float *outputLevels, int band)
{
  event->common.type = UpdateCompressionCurve;

  if (band >= 0 && band < instance->m_bandCount)
  {
    event->band = band;

    if (!CalcCurve(&event->curve, inputLevels, outputLevels))
      return 0;
  }

  return -1;
}
void
EAP_MultibandDrcControlInt32_DeInit(
  EAP_MultibandDrcControlInt32 *instance)
{
  EAP_MultibandDrcControl_DeInit(instance);
}
