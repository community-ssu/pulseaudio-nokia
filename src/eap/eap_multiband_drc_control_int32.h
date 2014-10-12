#ifndef EAP_MULTIBAND_DRC_CONTROL_INT32_H
#define EAP_MULTIBAND_DRC_CONTROL_INT32_H

#include "eap_multiband_drc_int32.h"
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
EAP_MultibandDrcControlInt32_Init(EAP_MultibandDrcControl *control,
                                  float sampleRate,
                                  int bandCount, int eqCount,
                                  float companderLookahead,
                                  float limiterLookahead,
                                  float unk, int blockSize);
void
EAP_MultibandDrcControlInt32_GetProcessingInitInfo(
    EAP_MultibandDrcControl *control, EAP_MultibandDrcInt32_InitInfo *initInfo);


#endif // EAP_MULTIBAND_DRC_CONTROL_INT32_H
