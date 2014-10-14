#include "eap_qmf_stereo_int32.h"

void
EAP_QmfStereoInt32_Init(EAP_QmfStereoInt32 *qmf, int16 coeff01, int16 coeff02,
                        int16 coeff11, int16 coeff12)
{
  qmf->m_leftAnalysisFilter0.m_mem1 = 0;
  qmf->m_leftAnalysisFilter0.m_mem2 = 0;
  qmf->m_leftAnalysisFilter1.m_mem1 = 0;
  qmf->m_leftAnalysisFilter1.m_mem2 = 0;
  qmf->m_rightAnalysisFilter0.m_mem1 = 0;
  qmf->m_rightAnalysisFilter0.m_mem2 = 0;
  qmf->m_rightAnalysisFilter1.m_mem1 = 0;
  qmf->m_rightAnalysisFilter1.m_mem2 = 0;
  qmf->m_leftSynthesisFilter0.m_mem1 = 0;
  qmf->m_leftSynthesisFilter0.m_mem2 = 0;
  qmf->m_leftSynthesisFilter1.m_mem1 = 0;
  qmf->m_leftSynthesisFilter1.m_mem2 = 0;
  qmf->m_rightSynthesisFilter0.m_mem1 = 0;
  qmf->m_rightSynthesisFilter0.m_mem2 = 0;
  qmf->m_rightSynthesisFilter1.m_mem1 = 0;
  qmf->m_rightSynthesisFilter1.m_mem2 = 0;
  qmf->m_prevInputSampleCount = 0;
  qmf->m_analysisOdd = 0;
  qmf->m_prevOutputSampleCount = 0;
  qmf->m_synthesisOdd = 0;
  qmf->m_leftAnalysisMem = 0;
  qmf->m_rightAnalysisMem = 0;
  qmf->m_leftSynthesisMem = 0;
  qmf->m_rightSynthesisMem = 0;
  qmf->coeff01 = coeff01;
  qmf->coeff02 = coeff02;
  qmf->coeff11 = coeff11;
  qmf->coeff12 = coeff12;
}
