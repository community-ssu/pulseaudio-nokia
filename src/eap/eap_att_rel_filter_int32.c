#include <assert.h>

#include "eap_att_rel_filter_int32.h"
#include "eap_long_multiplications.h"

void EAP_AttRelFilterInt32_Init(EAP_AttRelFilterInt32 *instance)
{
  instance->m_attCoeff = 0;
  instance->m_relCoeff = 0;
  instance->m_prevOutput = 0;
}

void
EAP_AttRelFilterInt32_UpdateAttackCoeff(EAP_AttRelFilterInt32 *instance,
                                        int16 attack)
{
  assert(attack >= 0);

  instance->m_attCoeff = attack;
}

void
EAP_AttRelFilterInt32_UpdateReleaseCoeff(EAP_AttRelFilterInt32 *instance,
                                         int16 release)
{
  assert(release >= 0);

  instance->m_relCoeff = release;
}

void EAP_AttRelFilterInt32_Process(EAP_AttRelFilterInt32 *instance,
                                   int32 *output, const int32 *input,
                                   int frames)
{
  int16 attCoeff = instance->m_attCoeff;
  int16 relCoeff = instance->m_relCoeff;
  int32 currOutput = instance->m_prevOutput;
  int i;

  for (i = 0; i < frames; i ++, output ++)
  {
    int32 diff = *input - currOutput;
    int16 coeff;

    input ++;

    if (diff <= 0)
      coeff = relCoeff;
    else
      coeff = attCoeff;

    currOutput += EAP_LongMultPosQ15x32(coeff, diff);

    *output = currOutput;

  }

  instance->m_prevOutput = currOutput;
}
