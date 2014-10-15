#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_three_bands_int32.h"

struct _EAP_WfirThreeBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_leftMemory[7];
  int32 m_rightMemory[7];
  int32 m_leftCompMem[4];
  int32 m_rightCompMem[4];
};

typedef struct _EAP_WfirThreeBandsInt32 EAP_WfirThreeBandsInt32;

void
EAP_WfirThreeBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirThreeBandsInt32 *inst = (EAP_WfirThreeBandsInt32 *)instance;
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 3;

  for (i = 0; i < 7; ++i)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  inst->m_leftCompMem[0] = 0;
  inst->m_leftCompMem[1] = 0;
  inst->m_leftCompMem[2] = 0;
  inst->m_leftCompMem[3] = 0;
  inst->m_rightCompMem[0] = 0;
  inst->m_rightCompMem[1] = 0;
  inst->m_rightCompMem[2] = 0;
  inst->m_rightCompMem[3] = 0;
}
