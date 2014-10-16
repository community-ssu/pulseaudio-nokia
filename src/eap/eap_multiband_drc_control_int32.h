#ifndef EAP_MULTIBAND_DRC_CONTROL_INT32_H
#define EAP_MULTIBAND_DRC_CONTROL_INT32_H

#include "eap_multiband_drc_int32.h"
#include "eap_mdrc_constants.h"
#include "eap_mdrc_internal_events.h"

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

struct _EAP_MultibandDrcControlInt32
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

typedef struct _EAP_MultibandDrcControlInt32 EAP_MultibandDrcControlInt32;

struct _EAP_MdrcInternalEventBandCoeffInt32
{
  EAP_MdrcInternalEvent common;
  int16 coeff;
  int16 dummy;
  int32 band;
};

typedef struct _EAP_MdrcInternalEventBandCoeffInt32
    EAP_MdrcInternalEventBandCoeffInt32;

typedef EAP_MdrcInternalEventBandCoeffInt32
    EAP_MdrcInternalEventCompanderAttackCoeffInt32;

typedef EAP_MdrcInternalEventBandCoeffInt32
    EAP_MdrcInternalEventCompanderReleaseCoeffInt32;

int
EAP_MultibandDrcControlInt32_Init(EAP_MultibandDrcControlInt32 *control,
                                  float sampleRate,
                                  int bandCount, int eqCount,
                                  float companderLookahead,
                                  float limiterLookahead,
                                  float unk, int blockSize);
void
EAP_MultibandDrcControlInt32_GetProcessingInitInfo(
    EAP_MultibandDrcControlInt32 *control,
    EAP_MultibandDrcInt32_InitInfo *initInfo);

int
EAP_MultibandDrcControlInt32_UpdateCompanderAttack(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompanderAttackCoeffInt32 *event, float attackTimeMs,
    int band);

#endif // EAP_MULTIBAND_DRC_CONTROL_INT32_H
