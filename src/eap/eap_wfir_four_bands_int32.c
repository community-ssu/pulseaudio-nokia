#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_four_bands_int32.h"

struct _EAP_WfirFourBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_warpingShift2;
  int32 m_leftMemory[11];
  int32 m_rightMemory[11];
  int32 m_leftCompMem[6];
  int32 m_rightCompMem[6];
};

typedef struct _EAP_WfirFourBandsInt32 EAP_WfirFourBandsInt32;

void
EAP_WfirFourBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirFourBandsInt32 *inst = (EAP_WfirFourBandsInt32 *)instance;
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 3;
  inst->m_warpingShift2 = 4;

  for (i = 0; i < 11; i ++)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  for (i = 0; i < 6; i ++)
  {
    inst->m_leftCompMem[i] = 0;
    inst->m_rightCompMem[i] = 0;
  }
}
