#include <assert.h>

#include "eap_multiband_drc_int32.h"
#include "eap_average_amplitude_int32.h"
#include "eap_wfir_dummy_int32.h"
#include "eap_wfir_two_bands_int32.h"
#include "eap_wfir_three_bands_int32.h"
#include "eap_wfir_four_bands_int32.h"
#include "eap_wfir_five_bands_int32.h"
#include "eap_mdrc_delays_and_gains_int32.h"
#include "eap_long_multiplications.h"
#include "eap_amplitude_to_gain_int32.h"

int
EAP_MultibandDrcInt32_MemoryRecordCount(
    EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  return 4 * (initInfo->bandCount + 4);
}

EAP_MultibandDrcInt32Handle EAP_MultibandDrcInt32_Init(
    EAP_MemoryRecord *memRec, EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  EAP_WfirInt32_InitFptr filterInitFunc;
  EAP_MultibandDrcInt32 *instance;
  int i;
  int32 *memBuffers[12];

  filterInitFunc = 0;
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
      filterInitFunc = EAP_WfirDummyInt32_Init;
      break;
    case 2:
      instance->filterbankFunc =
          (EAP_WfirInt32_ProcessFptr)EAP_WfirTwoBandsInt32_Process;
      filterInitFunc = (EAP_WfirInt32_InitFptr)EAP_WfirTwoBandsInt32_Init;
      break;
    case 3:
      instance->filterbankFunc = EAP_WfirThreeBandsInt32_Process;
      filterInitFunc = EAP_WfirThreeBandsInt32_Init;
      break;
    case 4:
      instance->filterbankFunc = EAP_WfirFourBandsInt32_Process;
      filterInitFunc = EAP_WfirFourBandsInt32_Init;
      break;
    case 5:
      instance->filterbankFunc = EAP_WfirFiveBandsInt32_Process;
      filterInitFunc = EAP_WfirFiveBandsInt32_Init;
      break;
  default:
    break;
  }

  filterInitFunc(instance->filterbank, initInfo->sampleRate * 0.5);

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

  EAP_MemsetBuff_filterbank_Int32(instance->filterbank->w_left,
                                  instance->filterbank->w_right);

  return instance;
}

static void
EAP_MultibandDrcInt32_CrossBandLink(EAP_MultibandDrcInt32 *instance, int frames)
{
  int32 bandCount = instance->bandCount;
  int16 self = instance->m_xBandLinkSelf;
  int32 **levels = instance->m_levelData;
  int32 i;
  int16 sum = instance->m_xBandLinkSum;

  for (i = 0; i < frames; i ++)
  {
    int32 levelSum = 0;
    int32 j;

    for (j = 0; j < bandCount; j ++)
      levelSum += levels[j][i];

    for (j = 0; j < bandCount; j ++)
    {
      levels[j][i] = EAP_LongMultPosQ14x32(
            self,
            levels[j][i]) + EAP_LongMultPosQ14x32(sum, levelSum);
    }
  }
}

void
EAP_MultibandDrcInt32_Process(EAP_MultibandDrcInt32Handle handle,
                              int32 *output1, int32 *output2,
                              const int32 *input1, const int32 *input2,
                              int frames)
{
  EAP_MultibandDrcInt32 *instance = handle;
  int lFramesLeft = frames;
  int lFramesDone = 0;
  int32 i;

  assert(output1 != output2);

  frames = 240;

  while (lFramesLeft > 0)
  {
    int levelCount;
    int downSampledFrames;

    if (lFramesLeft < 240)
      frames = lFramesLeft;

    downSampledFrames =  EAP_QmfStereoInt32_Analyze(&instance->qmf,
                                                    instance->m_scratchMem1,
                                                    instance->m_scratchMem2,
                                                    instance->m_scratchMem3,
                                                    instance->m_scratchMem4,
                                                    &input1[lFramesDone],
                                                    &input2[lFramesDone],
                                                    frames);
    instance->filterbankFunc(instance->filterbank,
                             instance->m_leftFilterbankOutputs,
                             instance->m_rightFilterbankOutputs,
                             instance->m_scratchMem5,
                             instance->m_scratchMem6,
                             instance->m_scratchMem1,
                             instance->m_scratchMem2,
                             instance->m_scratchMem3,
                             instance->m_scratchMem4,
                             downSampledFrames);

    for (i = 0; instance->bandCount > i; i ++)
    {
      EAP_AverageAmplitudeInt32_Process(&instance->avgFilters[i],
                                        instance->m_levelData[i],
                                        instance->m_leftFilterbankOutputs[i],
                                        instance->m_rightFilterbankOutputs[i],
                                        downSampledFrames,
                                        0);
    }

    levelCount =  EAP_AverageAmplitudeInt32_Process(
          &instance->avgFilters[instance->bandCount],
          instance->m_levelData[instance->bandCount - 1],
          instance->m_scratchMem5,
          instance->m_scratchMem6,
          downSampledFrames,
          1);
    EAP_MultibandDrcInt32_CrossBandLink(instance, levelCount);

    for (i = 0; instance->bandCount > i; i ++)
    {
      EAP_AttRelFilterInt32_Process(&instance->attRelFilters[i],
                                    instance->m_levelData[i],
                                    instance->m_levelData[i],
                                    levelCount);
      EAP_AmplitudeToGainInt32(&instance->compressionCurves[i],
                               instance->m_levelData[i],
                               instance->m_levelData[i],
                               levelCount);
    }

    EAP_MdrcDelaysAndGainsInt32_Process(&instance->gains,
                                        instance->m_scratchMem1,
                                        instance->m_scratchMem2,
                                        instance->m_scratchMem3,
                                        instance->m_scratchMem4,
                                        instance->m_leftFilterbankOutputs,
                                        instance->m_rightFilterbankOutputs,
                                        instance->m_scratchMem5,
                                        instance->m_scratchMem6,
                                        instance->m_levelData,
                                        downSampledFrames);
	EAP_QmfStereoInt32_Resynthesize(&instance->qmf,
                                    instance->m_scratchMem5,
                                    instance->m_scratchMem6,
                                    instance->m_scratchMem1,
                                    instance->m_scratchMem2,
                                    instance->m_scratchMem3,
                                    instance->m_scratchMem4);
    EAP_LimiterInt32_Process(&instance->limiter, output1, output2,
                             instance->m_scratchMem5, instance->m_scratchMem6,
                             frames);
    lFramesLeft -= frames;
    lFramesDone += frames;
    output1 += frames;
    output2 += frames;
  }
}

void EAP_MultibandDrcInt32_MemoryNeed(EAP_MemoryRecord *memRec, const EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  size_t companderLookaheadSize = 4 * initInfo->companderLookahead;
  memRec->type = 0;
  memRec->alignment = 0;
  memRec->size = 380;
  memRec[1].type = 0;
  memRec[1].alignment = 0;
  if (initInfo->bandCount <= 5)
  {
	  switch (initInfo->bandCount)
	  {
	  case 1:
		  memRec[1].size = 4;
		  break;
	  case 2:
		  memRec[1].size = 44;
		  break;
	  case 3:
		  memRec[1].size = 92;
		  break;
	  case 4:
		  memRec[1].size = 144;
		  break;
	  case 5:
		  memRec[1].size = 188;
		  break;
	  case 0:
		  break;
	  }
  }
  memRec[2].type = 0;
  memRec[2].alignment = 0;
  memRec[2].size = 16 * initInfo->bandCount + 16;
  memRec[3].type = 0;
  memRec[3].alignment = 0;
  memRec[3].size = 8 * initInfo->bandCount;
  memRec[4].type = 0;
  memRec[4].alignment = 0;
  memRec[4].size = 68 * initInfo->bandCount;
  memRec[5].type = 0;
  memRec[5].alignment = 0;
  memRec[5].size = 4 * initInfo->limiterLookahead;
  memRec[6].type = 0;
  memRec[6].alignment = 0;
  memRec[6].size = 4 * initInfo->limiterLookahead;
  memRec[7].type = 0;
  memRec[7].alignment = 0;
  memRec[7].size = companderLookaheadSize;
  memRec[8].type = 0;
  memRec[8].alignment = 0;
  memRec[8].size = companderLookaheadSize;
  int outputsamplecount = EAP_QmfStereoInt32_MaxOutputSampleCount(initInfo->maxBlockSize);
  size_t blocksize = 4 * initInfo->maxBlockSize;
  size_t scratchSize2 = 4 * outputsamplecount;
  size_t maxoutputcount = 4 * EAP_AverageAmplitudeInt32_MaxOutputCount(initInfo->downSamplingFactor, outputsamplecount) * initInfo->bandCount;
  memRec[10].type = 1;
  memRec[10].alignment = 4;
  memRec[10].size = 4 * outputsamplecount;
  memRec[11].type = 1;
  memRec[11].alignment = 4;
  memRec[11].size = 4 * outputsamplecount;
  memRec[12].type = 1;
  memRec[12].alignment = 4;
  memRec[12].size = 4 * outputsamplecount;
  memRec[13].type = 1;
  memRec[13].alignment = 4;
  memRec[13].size = 4 * outputsamplecount;
  memRec[14].type = 1;
  memRec[14].alignment = 4;
  memRec[14].size = blocksize;
  memRec[15].type = 1;
  memRec[15].alignment = 4;
  memRec[15].size = blocksize;
  memRec[9].type = 1;
  memRec[9].alignment = 4;
  memRec[9].size = maxoutputcount;
  for (int i = 0;initInfo->bandCount > i;i++)
  {
	  memRec[4 * i + 16].type = 0;
	  memRec[4 * i + 16].alignment = 4;
	  memRec[4 * i + 16].size = companderLookaheadSize;
	  memRec[4 * i + 17].type = 0;
	  memRec[4 * i + 17].alignment = 4;
	  memRec[4 * i + 17].size = companderLookaheadSize;
	  memRec[4 * i + 18].type = 1;
	  memRec[4 * i + 18].alignment = 4;
	  memRec[4 * i + 18].size = scratchSize2;
	  memRec[4 * i + 19].type = 1;
	  memRec[4 * i + 19].alignment = 4;
	  memRec[4 * i + 19].size = scratchSize2;
  }
}

void
EAP_MemsetBuff_filterbank_Int32(int32 *ptr_left, int32 *ptr_right)
{
#ifdef __ARM_NEON__
	int i = 240;
	int32x4_t zero = { 0, };

	for (i = 0; i < 240; i++, ptr_left += 8, ptr_right += 8)
	{
		vst1q_s32(ptr_left, zero);
		vst1q_s32(ptr_right, zero);
		vst1q_s32(ptr_left + 4, zero);
		vst1q_s32(ptr_right + 4, zero);
	}
#else
	memset(ptr_left, 0, 240 * 8 * sizeof(int32));
	memset(ptr_right, 0, 240 * 8 * sizeof(int32))
#endif
}

int
EAP_MultibandDrcInt32_Update(EAP_MultibandDrcInt32 *handle, const EAP_MdrcInternalEvent *event)
{
	//todo
	return 0;
}
