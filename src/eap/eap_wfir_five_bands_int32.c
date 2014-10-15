#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_five_bands_int32.h"

struct _EAP_WfirFiveBandsInt32
{
  EAP_WfirInt32 common;
  int32 m_leftMemory[15];
  int32 m_rightMemory[15];
  int32 m_leftCompMem[8];
  int32 m_rightCompMem[8];
};

typedef struct _EAP_WfirFiveBandsInt32 EAP_WfirFiveBandsInt32;

void
EAP_WfirFiveBandsInt32_Init(EAP_WfirInt32 *instance, int32 sampleRate)
{
  EAP_WfirFiveBandsInt32 *inst = (EAP_WfirFiveBandsInt32 *)instance;
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  inst->common.warpingShift = 2;

  for (i = 0; i < 15; i ++)
  {
    inst->m_leftMemory[i] = 0;
    inst->m_rightMemory[i] = 0;
  }

  for (i = 0; i < 8; ++i)
  {
    inst->m_leftCompMem[i] = 0;
    inst->m_rightCompMem[i] = 0;
  }
}
