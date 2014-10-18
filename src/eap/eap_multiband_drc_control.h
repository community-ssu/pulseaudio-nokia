#ifndef EAP_MULTIBAND_DRC_CONTROL_H
#define EAP_MULTIBAND_DRC_CONTROL_H

#include "eap_data_types.h"
#include "eap_mdrc_constants.h"

struct _EAP_MdrcCompressionCurve
{
  float inputLevels[EAP_MDRC_MAX_BAND_COUNT];
  float outputLevels[EAP_MDRC_MAX_BAND_COUNT];
  float limitLevel;
  float volume;
};

typedef struct _EAP_MdrcCompressionCurve EAP_MdrcCompressionCurve;

struct _EAP_MdrcCompressionCurveSet
{
  int curveCount;
  EAP_MdrcCompressionCurve *curves;
};

typedef struct _EAP_MdrcCompressionCurveSet EAP_MdrcCompressionCurveSet;

struct _EAP_MultibandDrcControl
{
  float m_sampleRate;
  int m_bandCount;
  int m_downSamplingFactor;
  float m_companderLookahead;
  float m_limiterLookahead;
  int m_maxBlockSize;
  float m_oneOverFactor;
  float m_volume[EAP_MDRC_MAX_BAND_COUNT];
  EAP_MdrcCompressionCurveSet m_curveSet[EAP_MDRC_MAX_BAND_COUNT];
  int m_eqCount;
  float **m_eqCurves;
};

typedef struct _EAP_MultibandDrcControl EAP_MultibandDrcControl;

int
EAP_MultibandDrcControl_Init(EAP_MultibandDrcControl *instance,
                             float sampleRate, int bandCount, int eqCount,
                             float companderLookahead, float limiterLookahead,
                             float controlRate, int maxBlockSize);

void
EAP_MultibandDrcControl_DeInit(
							EAP_MultibandDrcControl *instance);


#endif // EAP_MULTIBAND_DRC_CONTROL_H
