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
  event->common.type = CompanderAttackCoeff;

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
  event->common.type = CompanderReleaseCoeff;

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
    event->common.type = CrossBandLink;
    event->selfMult = ((1.0 - link) * 16384.0);
    event->sumMult = (16384.0 / (long double)instance->m_bandCount * link);
    return 0;
  }

  return -1;
}
