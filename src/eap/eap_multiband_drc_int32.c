#include <assert.h>

#include "eap_multiband_drc_int32.h"
#include "eap_average_amplitude_int32.h"
#include "eap_wfir_dummy_int32.h"
#include "eap_wfir_two_bands_int32.h"
#include "eap_wfir_three_bands_int32.h"
#include "eap_wfir_four_bands_int32.h"
#include "eap_wfir_five_bands_int32.h"

int
EAP_MultibandDrcInt32_MemoryRecordCount(
    EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  return 4 * (initInfo->bandCount + 4);
}

EAP_MultibandDrcInt32Handle EAP_MultibandDrcInt32_Init(
    EAP_MemoryRecord *memRec, EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  EAP_WfirInt32_InitFptr filterbankInitFunc;
  EAP_MultibandDrcInt32 *instance;
  int i;
  int32 *memBuffers[12];

  filterbankInitFunc = 0;
  instance = (EAP_MultibandDrcInt32 *)memRec[MEM_INSTANCE].base;

  assert(instance);
  assert(memRec);
  assert((initInfo->sampleRate >= 44000) && (initInfo->sampleRate <= 50000));
  assert((initInfo->bandCount >= 1) && (initInfo->bandCount <= EAP_MDRC_MAX_BAND_COUNT));

  instance->filterbank = (EAP_WfirInt32 *)memRec[MEM_FILTERBANK].base;
  instance->avgFilters =
      (EAP_AverageAmplitudeInt32 *)memRec[MEM_AVG_FILTERS].base;
  instance->attRelFilters =
      (EAP_AttRelFilterInt32 *)memRec[MEM_ATT_REL_FILTERS].base;
  instance->compressionCurves =
      (EAP_CompressionCurveImplDataInt32 *)memRec[MEM_COMPRESSION_CURVES].base;
  instance->m_limiterLookahead1 = (int32 *)memRec[MEM_LIMITER_LOOKAHEAD1].base;
  instance->m_limiterLookahead2 = (int32 *)memRec[MEM_LIMITER_LOOKAHEAD2].base;
  instance->m_scratchMem1 = (int32 *)memRec[MEM_SCRATCH1].base;
  instance->m_scratchMem2 = (int32 *)memRec[MEM_SCRATCH2].base;
  instance->m_scratchMem3 = (int32 *)memRec[MEM_SCRATCH3].base;
  instance->m_scratchMem4 = (int32 *)memRec[MEM_SCRATCH4].base;
  instance->m_scratchMem5 = (int32 *)memRec[MEM_SCRATCH5].base;
  instance->m_scratchMem6 = (int32 *)memRec[MEM_SCRATCH6].base;

  for (i = 0; initInfo->bandCount > i; i ++)
  {
    memBuffers[2 * i] = (int32 *)memRec[4 * i + 16].base;
    memBuffers[2 * i + 1] = (int32 *)memRec[4 * i + 17].base;
    instance->m_leftFilterbankOutputs[i] = (int32 *)memRec[4 * i + 18].base;
    instance->m_rightFilterbankOutputs[i] = (int32 *)memRec[4 * i + 19].base;
  }

  memBuffers[2 * initInfo->bandCount] =
      (int32 *)memRec[MEM_COMPANDER_LOOKAHEAD1].base;
  memBuffers[2 * initInfo->bandCount + 1] =
      (int32 *)memRec[MEM_COMPANDER_LOOKAHEAD2].base;

  EAP_MdrcDelaysAndGainsInt32_Init(&instance->gains, initInfo->bandCount,
                                   initInfo->companderLookahead,
                                   initInfo->downSamplingFactor,
                                   memBuffers);

  for (i = 0; initInfo->bandCount > i; i ++)
  {
    instance->m_levelData[i] =
        (int32*)(((char *)memRec[MEM_LEVEL_DATA].base) +
                 memRec[9].size / initInfo->bandCount * i);
  }

  instance->bandCount = initInfo->bandCount;

  switch (instance->bandCount)
  {
    case 1:
      instance->filterbankFunc = EAP_WfirDummyInt32_Process;
      filterbankInitFunc = EAP_WfirDummyInt32_Init;
      break;
    case 2:
      instance->filterbankFunc = EAP_WfirTwoBandsInt32_Process;
      filterbankInitFunc = EAP_WfirTwoBandsInt32_Init;
      break;
    case 3:
      instance->filterbankFunc = EAP_WfirThreeBandsInt32_Process;
      filterbankInitFunc = EAP_WfirThreeBandsInt32_Init;
      break;
    case 4:
      instance->filterbankFunc = EAP_WfirFourBandsInt32_Process;
      filterbankInitFunc = EAP_WfirFourBandsInt32_Init;
      break;
    case 5:
      instance->filterbankFunc = EAP_WfirFiveBandsInt32_Process;
      filterbankInitFunc = EAP_WfirFiveBandsInt32_Init;
      break;
  default:
    break;
  }

  filterbankInitFunc(instance->filterbank, initInfo->sampleRate * 0.5);

  EAP_QmfStereoInt32_Init(&instance->qmf, 16482, 802, 28168, 5492);
  EAP_LimiterInt32_Init( &instance->limiter, instance->m_limiterLookahead1,
                         instance->m_limiterLookahead2,
                         initInfo->limiterLookahead,
                         (int16 *)instance->m_scratchMem1);

  for (i = 0; i< instance->bandCount + 1; i++)
  {
    EAP_AverageAmplitudeInt32_Init(&instance->avgFilters[i],
                                   initInfo->downSamplingFactor,
                                   initInfo->avgShift);
  }

  instance->m_xBandLinkSelf = 16384;
  instance->m_xBandLinkSum = 0;

  EAP_MemsetBuff_filterbank_Int32(&instance->filterbank[1],
                                  &instance->filterbank[1921]);

  return instance;
}
