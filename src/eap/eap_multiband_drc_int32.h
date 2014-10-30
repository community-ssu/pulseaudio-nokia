#ifndef EAP_MULTIBAND_DRC_INT32_H
#define EAP_MULTIBAND_DRC_INT32_H

#include "eap_att_rel_filter_int32.h"
#include "eap_average_amplitude_int32.h"
#include "eap_limiter_int32.h"
#include "eap_memory.h"
#include "eap_mdrc_constants.h"
#include "eap_mdrc_delays_and_gains_int32.h"
#include "eap_mdrc_internal_events.h"
#include "eap_qmf_stereo_int32.h"
#include "eap_wfir_int32.h"
#include "eap_wfir_dummy_int32.h"
#include "eap_wfir_two_bands_int32.h"
#include "eap_wfir_three_bands_int32.h"
#include "eap_wfir_four_bands_int32.h"
#include "eap_wfir_five_bands_int32.h"

typedef void * EAP_MultibandDrcInt32Handle;

enum EAP_MultibandDrcInt32MemRecordsBase
{
  MEM_INSTANCE = 0,
  MEM_FILTERBANK = 1,
  MEM_AVG_FILTERS = 2,
  MEM_ATT_REL_FILTERS = 3,
  MEM_COMPRESSION_CURVES = 4,
  MEM_LIMITER_LOOKAHEAD1 = 5,
  MEM_LIMITER_LOOKAHEAD2 = 6,
  MEM_COMPANDER_LOOKAHEAD1 = 7,
  MEM_COMPANDER_LOOKAHEAD2 = 8,
  MEM_LEVEL_DATA = 9,
  MEM_SCRATCH1 = 10,
  MEM_SCRATCH2 = 11,
  MEM_SCRATCH3 = 12,
  MEM_SCRATCH4 = 13,
  MEM_SCRATCH5 = 14,
  MEM_SCRATCH6 = 15,
  MEM_RECORD_BASE_COUNT = 16,
};

enum _EAP_MultibandDrcInt32MemRecordsBand
{
  MEM_LOOKAHEAD_LEFT = 0,
  MEM_LOOKAHEAD_RIGHT = 1,
  MEM_FB_OUTPUT_LEFT = 2,
  MEM_FB_OUTPUT_RIGHT = 3,
  MEM_RECORD_BAND_COUNT = 4
};


typedef enum _EAP_MultibandDrcInt32MemRecordsBand
    EAP_MultibandDrcInt32MemRecordsBand;

struct _EAP_MultibandDrcInt32_InitInfo
{
  int sampleRate;
  int bandCount;
  int companderLookahead;
  int limiterLookahead;
  int downSamplingFactor;
  int avgShift;
  int maxBlockSize;
};

typedef struct _EAP_MultibandDrcInt32_InitInfo EAP_MultibandDrcInt32_InitInfo;

struct _EAP_CompressionCurveImplDataInt32
{
  int32 levelLimits[EAP_MDRC_MAX_BAND_COUNT];
  int32 K[EAP_MDRC_MAX_BAND_COUNT + 1];
  int16 AExp[EAP_MDRC_MAX_BAND_COUNT + 1];
  int16 AFrac[EAP_MDRC_MAX_BAND_COUNT + 1];
};
typedef struct _EAP_CompressionCurveImplDataInt32
    EAP_CompressionCurveImplDataInt32;

struct _EAP_MultibandDrcInt32
{
  EAP_QmfStereoInt32 qmf;
  EAP_WfirInt32_ProcessFptr filterbankFunc;
  EAP_WfirInt32 *filterbank;
  EAP_AverageAmplitudeInt32 *avgFilters;
  EAP_CompressionCurveImplDataInt32 *compressionCurves;
  EAP_AttRelFilterInt32 *attRelFilters;
  EAP_MdrcDelaysAndGainsInt32 gains;
  EAP_LimiterInt32 limiter;
  int32 bandCount;
  int16 m_xBandLinkSelf;
  int16 m_xBandLinkSum;
  int32 *m_companderLookahead[EAP_MDRC_MAX_BAND_COUNT];
  int32 *m_limiterLookahead1;
  int32 *m_limiterLookahead2;
  int32 *m_leftFilterbankOutputs[EAP_MDRC_MAX_BAND_COUNT];
  int32 *m_rightFilterbankOutputs[EAP_MDRC_MAX_BAND_COUNT];
  int32 *m_levelData[EAP_MDRC_MAX_BAND_COUNT];
  int32 *m_scratchMem1;
  int32 *m_scratchMem2;
  int32 *m_scratchMem3;
  int32 *m_scratchMem4;
  int32 *m_scratchMem5;
  int32 *m_scratchMem6;
};
typedef struct _EAP_MultibandDrcInt32 EAP_MultibandDrcInt32;

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

struct _EAP_MdrcInternalEventCoeffInt32
{
  EAP_MdrcInternalEvent common;
  int16 coeff;
  int16 dummy;
};

typedef struct _EAP_MdrcInternalEventCoeffInt32
    EAP_MdrcInternalEventCoeffInt32;

typedef struct _EAP_MdrcInternalEventCoeffInt32
    EAP_MdrcInternalEventLimiterAttackCoeffInt32;

typedef struct _EAP_MdrcInternalEventCoeffInt32
    EAP_MdrcInternalEventLimiterReleaseCoeffInt32;

struct _EAP_MdrcInternalEventLimiterThresholdInt32
{
  EAP_MdrcInternalEvent common;
  int32 threshold;
};

typedef struct _EAP_MdrcInternalEventLimiterThresholdInt32
    EAP_MdrcInternalEventLimiterThresholdInt32;

int
EAP_MultibandDrcInt32_MemoryRecordCount(
    EAP_MultibandDrcInt32_InitInfo *initInfo);

EAP_MultibandDrcInt32Handle EAP_MultibandDrcInt32_Init(
    EAP_MemoryRecord *memRec, EAP_MultibandDrcInt32_InitInfo *initInfo);

void
EAP_MultibandDrcInt32_Process(EAP_MultibandDrcInt32Handle handle,
                              int32 *output1, int32 *output2,
                              const int32 *input1, const int32 *input2,
                              int frames);

void EAP_MultibandDrcInt32_MemoryNeed(
    EAP_MemoryRecord *memRec, const EAP_MultibandDrcInt32_InitInfo *initInfo);

int
EAP_MultibandDrcInt32_Update(EAP_MultibandDrcInt32Handle handle,
                             const EAP_MdrcInternalEvent *event);

void EAP_MemsetBuff_filterbank_Int32(int32 *ptr_left, int32 *ptr_right);

#endif // EAP_MULTIBAND_DRC_INT32_H
