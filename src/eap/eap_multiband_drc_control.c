#include <stdlib.h>

#include "eap_multiband_drc_control.h"

int
EAP_MultibandDrcControl_Init(EAP_MultibandDrcControl *instance,
                             float sampleRate, int bandCount, int eqCount,
                             float companderLookahead, float limiterLookahead,
                             float controlRate, int maxBlockSize)
{
  int i, j, k;
  int downSamplingFactor;

  if (bandCount <= 0 || bandCount > EAP_MDRC_MAX_BAND_COUNT)
    return -1;

  if (sampleRate < 44000.0 || sampleRate > 50000.0)
    return -2;

  downSamplingFactor = (int)(sampleRate * 0.5 / controlRate);

  if ( downSamplingFactor <= 0 )
    return -3;

  instance->m_downSamplingFactor = downSamplingFactor;
  instance->m_oneOverFactor = 1.0 / (float)downSamplingFactor;
  instance->m_sampleRate = sampleRate;
  instance->m_bandCount = bandCount;
  instance->m_companderLookahead = companderLookahead;
  instance->m_limiterLookahead = limiterLookahead;
  instance->m_maxBlockSize = maxBlockSize;

  for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
  {
    instance->m_volume[i] = 0;
    instance->m_curveSet[i].curveCount = 1;
    instance->m_curveSet[i].curves =
        (EAP_MdrcCompressionCurve *)malloc(sizeof(EAP_MdrcCompressionCurve));

    if (!instance->m_curveSet[i].curves)
    {
      for ( j = 0; j != i; j ++)
        free(instance->m_curveSet[j].curves);

      return -4;
    }

    instance->m_curveSet[i].curves->limitLevel = 10.0;
    instance->m_curveSet[i].curves->volume = 0;

    for (k = 0; k < EAP_MDRC_MAX_BAND_COUNT; k ++)
    {
      float level = (float)k * 20.0 - 60.0;
      instance->m_curveSet[i].curves->inputLevels[k] = level;
      instance->m_curveSet[i].curves->outputLevels[k] = level;
    }
  }

  instance->m_eqCount = eqCount;
  instance->m_eqCurves =
      (float **)malloc(sizeof(instance->m_eqCurves[0]) * eqCount);

  if (!instance->m_eqCurves)
  {
    for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
      free(instance->m_curveSet[i].curves);

    return -5;
  }

  for (i = 0; ; i ++)
  {
    if (i == eqCount)
      return 0;

    instance->m_eqCurves[i] =
        (float *)calloc(bandCount, sizeof(instance->m_eqCurves[0]));

    if (!instance->m_eqCurves[i])
      break;
  }

  for (j = 0; j != i; j ++)
    free(instance->m_eqCurves[j]);

  for (i = 0; i != EAP_MDRC_MAX_BAND_COUNT; i ++ )
    free(instance->m_curveSet[i].curves);

  return -6;
}
