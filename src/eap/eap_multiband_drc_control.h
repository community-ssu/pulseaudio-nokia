#ifndef EAP_MULTIBAND_DRC_CONTROL_H
#define EAP_MULTIBAND_DRC_CONTROL_H

#include "eap_data_types.h"
#include "eap_mdrc_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

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

int
EAP_MultibandDrcControl_UpdateCompressionCurveSet(
    EAP_MultibandDrcControl *instance,
    EAP_MdrcCompressionCurve *outCurve,
    const EAP_MdrcCompressionCurveSet *curveSet,
    int band);

int EAP_MultibandDrcControl_CalcCurve(const EAP_MultibandDrcControl *instance,
                                      EAP_MdrcCompressionCurve *outCurve,
                                      int band);

void
EAP_MultibandDrcControl_ApplyVolumeSetting(
    EAP_MdrcCompressionCurve *outCurve,
    const EAP_MdrcCompressionCurveSet *inCurves,
    float currVolume);

void
EAP_MultibandDrcControl_InterpolateCompression(
    EAP_MdrcCompressionCurve *outCurve,
    const EAP_MdrcCompressionCurve *inCurve1,
    const EAP_MdrcCompressionCurve *inCurve2,
    float currVolume);

void
EAP_MultibandDrcControl_ApplyLevel(EAP_MdrcCompressionCurve *outCurve,
                                   const EAP_MdrcCompressionCurve *inCurve,
                                   float level);

void
EAP_MultibandDrcControl_ApplySaturation(
    EAP_MdrcCompressionCurve *outCurve,const EAP_MdrcCompressionCurve *inCurve);

int
EAP_MultibandDrcControl_UpdateEQLevel(const EAP_MultibandDrcControl *instance,
                                      EAP_MdrcCompressionCurve *outCurve,
                                      float eqLevel, int eqIndex, int band);

void
EAP_MultibandDrcControl_DeInit(EAP_MultibandDrcControl *instance);

int
EAP_MultibandDrcControl_UpdateVolumeSetting(EAP_MultibandDrcControl *instance,
                                            EAP_MdrcCompressionCurve *outCurve,
                                            float volume, int band);

#ifdef __cplusplus
}
#endif

#endif // EAP_MULTIBAND_DRC_CONTROL_H
