#include <stdlib.h>
#include <math.h>

#include "eap_multiband_drc_control_int32.h"

int
EAP_MultibandDrcControlInt32_Init(EAP_MultibandDrcControl *control,
                                  float sampleRate,
                                  int bandCount, int eqCount,
                                  float companderLookahead,
                                  float limiterLookahead,
                                  float unk, int blockSize)
{
  int i, j, k;
  int downSamplingFactor;

  if (bandCount <= 0 || bandCount > EAP_MDRC_MAX_BAND_COUNT)
    return -1;

  if (sampleRate < 44000.0 || sampleRate > 50000.0)
    return -2;

  downSamplingFactor = (int)(sampleRate * 0.5 / unk);

  if ( downSamplingFactor <= 0 )
    return -3;

  control->m_downSamplingFactor = downSamplingFactor;
  control->m_oneOverFactor = 1.0 / (float)downSamplingFactor;
  control->m_sampleRate = sampleRate;
  control->m_bandCount = bandCount;
  control->m_companderLookahead = companderLookahead;
  control->m_limiterLookahead = limiterLookahead;
  control->m_maxBlockSize = blockSize;

  for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
  {
    control->m_volume[i] = 0;
    control->m_curveSet[i].curveCount = 1;
    control->m_curveSet[i].curves =
        (EAP_MdrcCompressionCurve *)malloc(sizeof(EAP_MdrcCompressionCurve));

    if (!control->m_curveSet[i].curves)
    {
      for ( j = 0; j != i; j ++)
        free(control->m_curveSet[j].curves);

      return -4;
    }

    control->m_curveSet[i].curves->limitLevel = 10.0;
    control->m_curveSet[i].curves->volume = 0;

    for (k = 0; k < EAP_MDRC_MAX_BAND_COUNT; k ++)
    {
      float level = (float)k * 20.0 - 60.0;
      control->m_curveSet[i].curves->inputLevels[k] = level;
      control->m_curveSet[i].curves->outputLevels[k] = level;
    }
  }

  control->m_eqCount = eqCount;
  control->m_eqCurves =
      (float **)malloc(sizeof(control->m_eqCurves[0]) * eqCount);

  if (!control->m_eqCurves)
  {
    for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
      free(control->m_curveSet[i].curves);

    return -5;
  }

  for (i = 0; ; i ++)
  {
    if (i == eqCount)
      return 0;

    control->m_eqCurves[i] =
        (float *)calloc(bandCount, sizeof(control->m_eqCurves[0]));

    if (!control->m_eqCurves[i])
      break;
  }

  for (j = 0; j != i; j ++)
    free(control->m_eqCurves[j]);

  for (i = 0; i != EAP_MDRC_MAX_BAND_COUNT; i ++ )
    free(control->m_curveSet[i].curves);

  return -6;
}

void
EAP_MultibandDrcControlInt32_GetProcessingInitInfo(
    EAP_MultibandDrcControl *control, EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  initInfo->sampleRate = control->m_sampleRate;
  initInfo->bandCount = control->m_bandCount;
  initInfo->companderLookahead =
      control->m_companderLookahead * 0.001 * 0.5 * control->m_sampleRate + 0.5;
  initInfo->limiterLookahead =
      control->m_limiterLookahead * 0.001 * control->m_sampleRate + 0.5;
  initInfo->downSamplingFactor = control->m_downSamplingFactor;
  initInfo->maxBlockSize = control->m_maxBlockSize;

  initInfo->avgShift =
      0.5 - log(1.0 - exp(-1.0 / control->m_downSamplingFactor)) / log(2.0);
}
