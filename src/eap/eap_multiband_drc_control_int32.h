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

void
EAP_MultibandDrcControlInt32_DeInit(
  EAP_MultibandDrcControlInt32 *instance);

#ifdef __cplusplus
}
#endif

#endif // EAP_MULTIBAND_DRC_CONTROL_INT32_H
