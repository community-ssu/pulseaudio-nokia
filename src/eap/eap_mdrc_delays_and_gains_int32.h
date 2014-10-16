#ifndef EAP_MDRC_DELAYS_AND_GAINS_INT32_H
#define EAP_MDRC_DELAYS_AND_GAINS_INT32_H

#include "eap_data_types.h"
#include "eap_mdrc_constants.h"

struct _EAP_MdrcDelaysAndGainsInt32
{
  int m_bandCount;
  int m_delay;
  int m_downSamplingFactor;
  int m_downSamplingCounter;
  int16 m_oneOverFactorQ15;
  int32 m_currGainQ15[EAP_MDRC_MAX_BAND_COUNT];
  int32 m_currDeltaQ15[EAP_MDRC_MAX_BAND_COUNT];
  int32 *m_memBuffers[12];
};
typedef struct _EAP_MdrcDelaysAndGainsInt32 EAP_MdrcDelaysAndGainsInt32;

void
EAP_MdrcDelaysAndGainsInt32_Init(EAP_MdrcDelaysAndGainsInt32 *instance,
                                 int bandCount, int delay,
                                 int downSamplingFactor,
                                 int32 *const *memoryBuffers);

#endif // EAP_MDRC_DELAYS_AND_GAINS_INT32_H
