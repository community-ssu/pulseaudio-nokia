#ifndef EAP_ATT_REL_FILTER_INT32_H
#define EAP_ATT_REL_FILTER_INT32_H

#include "eap_data_types.h"

struct _EAP_AttRelFilterInt32
{
  int16 m_attCoeff;
  int16 m_relCoeff;
  int32 m_prevOutput;
};
typedef struct _EAP_AttRelFilterInt32 EAP_AttRelFilterInt32;

void EAP_AttRelFilterInt32_Init(EAP_AttRelFilterInt32 *instance);

void
EAP_AttRelFilterInt32_UpdateAttackCoeff(EAP_AttRelFilterInt32 *instance,
                                        int16 attack);

void
EAP_AttRelFilterInt32_UpdateReleaseCoeff(EAP_AttRelFilterInt32 *instance,
                                         int16 release);

void EAP_AttRelFilterInt32_Process(EAP_AttRelFilterInt32 *instance,
                                   int32 *output, const int32 *input,
                                   int frames);

#endif // EAP_ATT_REL_FILTER_INT32_H
