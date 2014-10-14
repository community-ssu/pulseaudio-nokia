#ifndef EAP_QMF_STEREO_INT32_H
#define EAP_QMF_STEREO_INT32_H

#include "eap_data_types.h"

struct _EAP_QmfStereoInt32FilterState
{
  int32 m_mem1;
  int32 m_mem2;
};
typedef struct _EAP_QmfStereoInt32FilterState EAP_QmfStereoInt32FilterState;

struct _EAP_QmfStereoInt32
{
  int16 coeff01;
  int16 coeff02;
  int16 coeff11;
  int16 coeff12;
  EAP_QmfStereoInt32FilterState m_leftAnalysisFilter0;
  EAP_QmfStereoInt32FilterState m_leftAnalysisFilter1;
  EAP_QmfStereoInt32FilterState m_rightAnalysisFilter0;
  EAP_QmfStereoInt32FilterState m_rightAnalysisFilter1;
  EAP_QmfStereoInt32FilterState m_leftSynthesisFilter0;
  EAP_QmfStereoInt32FilterState m_leftSynthesisFilter1;
  EAP_QmfStereoInt32FilterState m_rightSynthesisFilter0;
  EAP_QmfStereoInt32FilterState m_rightSynthesisFilter1;
  int m_prevInputSampleCount;
  int m_analysisOdd;
  int m_prevOutputSampleCount;
  int m_synthesisOdd;
  int32 m_leftAnalysisMem;
  int32 m_rightAnalysisMem;
  int32 m_leftSynthesisMem;
  int32 m_rightSynthesisMem;
};
typedef struct _EAP_QmfStereoInt32 EAP_QmfStereoInt32;

void
EAP_QmfStereoInt32_Init(EAP_QmfStereoInt32 *qmf, int16 coeff01, int16 coeff02,
                        int16 coeff11, int16 coeff12);

#endif // EAP_QMF_STEREO_INT32_H
