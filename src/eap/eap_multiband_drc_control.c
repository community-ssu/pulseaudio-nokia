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

void
EAP_MultibandDrcControl_InterpolateCompression(
    EAP_MdrcCompressionCurve *outCurve,
    const EAP_MdrcCompressionCurve *inCurve1,
    const EAP_MdrcCompressionCurve *inCurve2,
    float currVolume)
{
  float range;
  float offset;
  float weight1;
  float weight2;
  int i;

  range = inCurve2->volume - inCurve1->volume;
  offset = currVolume - inCurve1->volume;

  if (offset > 0.0)
  {
    if (offset < range && range > 0.0)
      weight2 = offset / range;
    else
      weight2 = 1.0;
  }
  else
    weight2 = 0.0;

  weight1 = 1.0 - weight2;

  for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
  {
    outCurve->inputLevels[i] =
        inCurve1->inputLevels[i] * weight1 +
        inCurve2->inputLevels[i] * weight2;
    outCurve->outputLevels[i] =
        inCurve1->outputLevels[i] * weight1 +
        inCurve2->outputLevels[i] * weight2;
  }

  outCurve->limitLevel =
      inCurve1->limitLevel * weight1 + inCurve2->limitLevel * weight2;
  outCurve->volume = currVolume;
}

void
EAP_MultibandDrcControl_ApplyVolumeSetting(
    EAP_MdrcCompressionCurve *outCurve,
    const EAP_MdrcCompressionCurveSet *inCurves,
    float currVolume)
{
  const EAP_MdrcCompressionCurve *curve1;
  const EAP_MdrcCompressionCurve *curve2;
  int i;

  curve1 = inCurves->curves;
  curve2 = &inCurves->curves[inCurves->curveCount - 1];

  for (i = 0; i < inCurves->curveCount; i ++)
  {
    if (currVolume >= inCurves->curves[i].volume)
      curve1 = &inCurves->curves[i];
  }

  for (i = inCurves->curveCount; i > 0; i --)
  {
    if (inCurves->curves[i - 1].volume >= currVolume )
      curve2 = &inCurves->curves[i - 1];
  }

  EAP_MultibandDrcControl_InterpolateCompression(outCurve,
                                                 curve1,
                                                 curve2,
                                                 currVolume);
}

void
EAP_MultibandDrcControl_ApplyLevel(EAP_MdrcCompressionCurve *outCurve,
                                   const EAP_MdrcCompressionCurve *inCurve,
                                   float level)
{
  int i;

  for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
    outCurve->outputLevels[i] = inCurve->outputLevels[i] + level;
}

void
EAP_MultibandDrcControl_ApplySaturation(EAP_MdrcCompressionCurve *outCurve,
                                        const EAP_MdrcCompressionCurve *inCurve)
{
  int i;
  float limit = inCurve->limitLevel;
  int lastBelowLimit = -1;

  outCurve->limitLevel = inCurve->limitLevel;
  outCurve->volume = inCurve->volume;

  for (i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
  {
    outCurve->inputLevels[i] = inCurve->inputLevels[i];
    outCurve->outputLevels[i] = inCurve->outputLevels[i];

    if (limit > inCurve->outputLevels[i])
      lastBelowLimit = i;
  }

  if ( lastBelowLimit >= 0 && lastBelowLimit <= 3 &&
       inCurve->outputLevels[lastBelowLimit + 1] > limit )
  {
    float x0 = inCurve->outputLevels[lastBelowLimit + 1];
    float x1 = inCurve->outputLevels[lastBelowLimit];
    float y0 = inCurve->inputLevels[lastBelowLimit + 1];
    float y1 = inCurve->inputLevels[lastBelowLimit];

    outCurve->inputLevels[lastBelowLimit + 1] =
        ((y0 - y1) * limit - x1 * y0 + x0 * y1) / (x0 - x1);
  }

  for ( i = lastBelowLimit + 1; i < EAP_MDRC_MAX_BAND_COUNT; i ++)
    outCurve->outputLevels[i] = limit;
}

int
EAP_MultibandDrcControl_CalcCurve(const EAP_MultibandDrcControl *instance,
                                  EAP_MdrcCompressionCurve *outCurve,
                                  int band)
{
  int i;
  float eqLevel = 0;

  for (i = 0; instance->m_eqCount > i; i ++)
    eqLevel = instance->m_eqCurves[i][band] + eqLevel;

  EAP_MultibandDrcControl_ApplyVolumeSetting(outCurve,
                                             &instance->m_curveSet[band],
                                             instance->m_volume[band]);
  EAP_MultibandDrcControl_ApplyLevel(outCurve, outCurve, eqLevel);
  EAP_MultibandDrcControl_ApplySaturation(outCurve, outCurve);

  return 0;
}

int
EAP_MultibandDrcControl_UpdateCompressionCurveSet(
    EAP_MultibandDrcControl *instance,
    EAP_MdrcCompressionCurve *outCurve,
    const EAP_MdrcCompressionCurveSet *curveSet,
    int band)
{
  int rv = -1;
  int i, j;

  if (band >= 0 && band < instance->m_bandCount)
  {
    if (curveSet->curveCount != instance->m_curveSet[band].curveCount)
    {
      free(instance->m_curveSet[band].curves);
      if (curveSet->curveCount)
      {
        instance->m_curveSet[band].curves = (EAP_MdrcCompressionCurve *)
            malloc(sizeof(EAP_MdrcCompressionCurve) * curveSet->curveCount);

        if (!instance->m_curveSet[band].curves)
        {
          instance->m_curveSet[band].curveCount = 0;
          return -2; /* E_NOMEM ? */
        }
      }
      else
        instance->m_curveSet[band].curves = 0;

      instance->m_curveSet[band].curveCount = curveSet->curveCount;
    }

    for (i = 0; curveSet->curveCount != i; i ++)
    {
      EAP_MdrcCompressionCurve *src = &curveSet->curves[i];
      EAP_MdrcCompressionCurve *dst = &instance->m_curveSet[band].curves[i];

      for (j = 0; j < EAP_MDRC_MAX_BAND_COUNT; j ++)
      {
        dst->inputLevels[j] = src->inputLevels[j];
        dst->outputLevels[j] = src->outputLevels[j];
      }

      dst->limitLevel = src->limitLevel;
      dst->volume = src->volume;
    }

    rv = EAP_MultibandDrcControl_CalcCurve(instance, outCurve, band);
  }

  return rv;
}

int
EAP_MultibandDrcControl_UpdateEQLevel(const EAP_MultibandDrcControl *instance,
                                      EAP_MdrcCompressionCurve *outCurve,
                                      float eqLevel, int eqIndex, int band)
{
  if (band < 0 || band >= instance->m_bandCount)
    return -1;

  if (eqIndex < 0 || eqIndex >= instance->m_eqCount)
    return -2;

  if ( eqLevel < -15.0 || eqLevel > 15.0 )
    return -3;

  instance->m_eqCurves[eqIndex][band] = eqLevel;

  return EAP_MultibandDrcControl_CalcCurve(instance, outCurve, band);
}

void
EAP_MultibandDrcControl_DeInit(
  EAP_MultibandDrcControl *instance)
{
  if (instance->m_eqCurves)
  {
    for (int i = 0; instance->m_eqCount != i; i++)
    {
      free(instance->m_eqCurves[i]);
      instance->m_eqCurves[i] = NULL;
    }
    free(instance->m_eqCurves);
    instance->m_eqCurves = NULL;
  }
  for (int i = 0; i != 5; i++)
  {
    if (instance->m_curveSet[i].curveCount > 0)
    {
      free(instance->m_curveSet[i].curves);
      instance->m_curveSet[i].curves = NULL;
      instance->m_curveSet[i].curveCount = 0;
    }
  }
}

int
EAP_MultibandDrcControl_UpdateVolumeSetting(EAP_MultibandDrcControl *instance, EAP_MdrcCompressionCurve *outCurve, float volume, int band)
{
	if (band >= 0 && instance->m_bandCount > band)
	{
		instance->m_volume[band] = volume;
		return EAP_MultibandDrcControl_CalcCurve(instance, outCurve, band);
	}
	return -1;
}
