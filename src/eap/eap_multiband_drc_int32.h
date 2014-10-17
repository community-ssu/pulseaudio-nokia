#ifndef EAP_MULTIBAND_DRC_INT32_H
#define EAP_MULTIBAND_DRC_INT32_H

#include "eap_att_rel_filter_int32.h"
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

void
EAP_MultibandDrcInt32_Process(EAP_MultibandDrcInt32Handle handle,
                              int32 *output1, int32 *output2,
                              const int32 *input1, const int32 *input2,
                              int frames);

#endif // EAP_MULTIBAND_DRC_INT32_H
