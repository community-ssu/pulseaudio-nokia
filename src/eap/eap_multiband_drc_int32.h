#ifndef EAP_MULTIBAND_DRC_INT32_H
#define EAP_MULTIBAND_DRC_INT32_H

#include "eap_data_types.h"
#include "eap_mdrc_constants.h"
#include "eap_mdrc_delays_and_gains_int32.h"
#include "eap_limiter_int32.h"
#include "eap_average_amplitude_int32.h"
#include "eap_memory.h"
#include "eap_wfir_int32.h"
#include "eap_qmf_stereo_int32.h"

typedef void * EAP_MultibandDrcInt32Handle;

enum EAP_MultibandDrcInt32MemRecordsBase
{
  MEM_INSTANCE = 0x0,
  MEM_FILTERBANK = 0x1,
  MEM_AVG_FILTERS = 0x2,
  MEM_ATT_REL_FILTERS = 0x3,
  MEM_COMPRESSION_CURVES = 0x4,
  MEM_LIMITER_LOOKAHEAD1 = 0x5,
  MEM_LIMITER_LOOKAHEAD2 = 0x6,
  MEM_COMPANDER_LOOKAHEAD1 = 0x7,
  MEM_COMPANDER_LOOKAHEAD2 = 0x8,
  MEM_LEVEL_DATA = 0x9,
  MEM_SCRATCH1 = 0xA,
  MEM_SCRATCH2 = 0xB,
  MEM_SCRATCH3 = 0xC,
  MEM_SCRATCH4 = 0xD,
  MEM_SCRATCH5 = 0xE,
  MEM_SCRATCH6 = 0xF,
  MEM_RECORD_BASE_COUNT = 0x10,
};

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
  int32 levelLimits[5];
  int32 K[6];
  int16 AExp[6];
  int16 AFrac[6];
};
typedef struct _EAP_CompressionCurveImplDataInt32 EAP_CompressionCurveImplDataInt32;

struct _EAP_AttRelFilterInt32
{
  int16 m_attCoeff;
  int16 m_relCoeff;
  int32 m_prevOutput;
};
typedef struct _EAP_AttRelFilterInt32 EAP_AttRelFilterInt32;

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

int
EAP_MultibandDrcInt32_MemoryRecordCount(
    EAP_MultibandDrcInt32_InitInfo *initInfo);

EAP_MultibandDrcInt32Handle EAP_MultibandDrcInt32_Init(
    EAP_MemoryRecord *memRec, EAP_MultibandDrcInt32_InitInfo *initInfo);

#endif // EAP_MULTIBAND_DRC_INT32_H
