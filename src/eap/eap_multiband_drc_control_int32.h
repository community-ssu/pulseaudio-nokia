#ifndef EAP_MULTIBAND_DRC_CONTROL_INT32_H
#define EAP_MULTIBAND_DRC_CONTROL_INT32_H

#include "eap_multiband_drc_control.h"
#include "eap_multiband_drc_int32.h"
#include "eap_mdrc_internal_events.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef EAP_MultibandDrcControl  EAP_MultibandDrcControlInt32;

enum _EAP_MdrcControlInternalEventType
{
  UpdateCompressionCurve = 0,
  UpdateCompanderAttack = 1,
  UpdateCompanderRelease = 2,
  UpdateLimiterAttack = 3,
  UpdateLimiterRelease = 4,
  UpdateLimiterThreshold = 5,
  UpdateCrossBandLink = 6,
};

typedef enum _EAP_MdrcControlInternalEventType EAP_MdrcControlInternalEventType;

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

struct _EAP_MdrcInternalEventCrossBandLinkInt32
{
  EAP_MdrcInternalEvent common;
  int16 selfMult;
  int16 sumMult;
};

typedef struct _EAP_MdrcInternalEventCrossBandLinkInt32
    EAP_MdrcInternalEventCrossBandLinkInt32;

struct EAP_MdrcInternalEventCompressionCurveInt32
{
  EAP_MdrcInternalEvent common;
  EAP_CompressionCurveImplDataInt32 curve;
  int32 band;
};

typedef struct EAP_MdrcInternalEventCompressionCurveInt32
    EAP_MdrcInternalEventCompressionCurveInt32;

int
EAP_MultibandDrcControlInt32_Init(EAP_MultibandDrcControlInt32 *instance,
                                  float sampleRate, int bandCount, int eqCount,
                                  float companderLookahead,
                                  float limiterLookahead,
                                  float controlRate, int maxBlockSize);

void
EAP_MultibandDrcControlInt32_GetProcessingInitInfo(
    EAP_MultibandDrcControlInt32 *instance,
    EAP_MultibandDrcInt32_InitInfo *initInfo);

int
EAP_MultibandDrcControlInt32_UpdateCompanderAttack(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompanderAttackCoeffInt32 *event, float attackTimeMs,
    int band);

int
EAP_MultibandDrcControlInt32_UpdateCompanderRelease(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompanderReleaseCoeffInt32 *event,
    float releaseTimeMs, int band);

int
EAP_MultibandDrcControlInt32_UpdateCrossBandLink(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCrossBandLinkInt32 *event, float link);

int
EAP_MultibandDrcControlInt32_UpdateCompressionCurve(
    const EAP_MultibandDrcControlInt32 *instance,
    EAP_MdrcInternalEventCompressionCurveInt32 *event,
    const float *inputLevels, const float *outputLevels, int band);

int
CalcCurve(EAP_CompressionCurveImplDataInt32 *curve, const float *inLevels,
              const float *outLevels);

void
ConvertA(int16 *aFrac, int16 *aExp, float A);

int
EAP_MultibandDrcControlInt32_UpdateCompressionCurveSet(
    EAP_MultibandDrcControl *instance,
    EAP_MdrcInternalEventCompressionCurveInt32 *event,
    const EAP_MdrcCompressionCurveSet *curveSet, int band);

int
EAP_MultibandDrcControlInt32_UpdateEQLevel(
    const EAP_MultibandDrcControl *instance,
    EAP_MdrcInternalEventCompressionCurveInt32 *event, float eqLevel,
    int eqIndex, int band);

void
EAP_MultibandDrcControlInt32_DeInit(
  EAP_MultibandDrcControlInt32 *instance);

struct _EAP_MdrcInternalEventCoeffInt32
{
	EAP_MdrcInternalEvent common;
	int16 coeff;
	int16 dummy;
};

typedef struct _EAP_MdrcInternalEventCoeffInt32 EAP_MdrcInternalEventCoeffInt32;
typedef struct _EAP_MdrcInternalEventCoeffInt32 EAP_MdrcInternalEventLimiterAttackCoeffInt32;
typedef struct _EAP_MdrcInternalEventCoeffInt32 EAP_MdrcInternalEventLimiterReleaseCoeffInt32;

int
EAP_MultibandDrcControlInt32_UpdateLimiterAttack(const EAP_MultibandDrcControlInt32 *instance, EAP_MdrcInternalEventLimiterAttackCoeffInt32 *event, float attackTimeMs);

int
EAP_MultibandDrcControlInt32_UpdateLimiterRelease(const EAP_MultibandDrcControlInt32 *instance, EAP_MdrcInternalEventLimiterReleaseCoeffInt32 *event, float releaseTimeMs);

struct _EAP_MdrcInternalEventLimiterThresholdInt32
{
	EAP_MdrcInternalEvent common;
	int32 threshold;
};

typedef struct _EAP_MdrcInternalEventLimiterThresholdInt32 EAP_MdrcInternalEventLimiterThresholdInt32;

int
EAP_MultibandDrcControlInt32_UpdateLimiterThreshold(EAP_MultibandDrcControlInt32 *instance, EAP_MdrcInternalEventLimiterThresholdInt32 *event, float threshold);

int
EAP_MultibandDrcControlInt32_UpdateVolumeSetting(EAP_MultibandDrcControlInt32 *instance, EAP_MdrcInternalEventCompressionCurveInt32 *event, float volume, int band);

#ifdef __cplusplus
}
#endif

#endif // EAP_MULTIBAND_DRC_CONTROL_INT32_H
