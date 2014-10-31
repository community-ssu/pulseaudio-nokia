#ifndef EAP_MDRC_DELAYS_AND_GAINS_INT32_H
#define EAP_MDRC_DELAYS_AND_GAINS_INT32_H

#include "eap_data_types.h"
#include "eap_mdrc_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _EAP_MdrcDelaysAndGainsInt32
{
  int m_bandCount;
  int m_delay;
  int m_downSamplingFactor;
  int m_downSamplingCounter;
  int16 m_oneOverFactorQ15;
  int32 m_currGainQ15[EAP_MDRC_MAX_BAND_COUNT];
  int32 m_currDeltaQ15[EAP_MDRC_MAX_BAND_COUNT];
  int32 *m_memBuffers[2 * (EAP_MDRC_MAX_BAND_COUNT + 1)];
};
typedef struct _EAP_MdrcDelaysAndGainsInt32 EAP_MdrcDelaysAndGainsInt32;

void
EAP_MdrcDelaysAndGainsInt32_Init(EAP_MdrcDelaysAndGainsInt32 *instance,
                                 int bandCount, int delay,
                                 int downSamplingFactor,
                                 int32 *const *memoryBuffers);

void
EAP_MdrcDelaysAndGainsInt32_Gain_Scal(int32 const *in1, int32 const *in2,
                                      int32 const *gainVector,
                                      int32 *out1, int32 *out2,
                                      int32 loop_count);

void
EAP_MdrcDelaysAndGainsInt32_Gain_Scal1(int32 const *in1, int32 const *in2,
                                       int32 const *gainVector,
                                       int32 *out1, int32 *out2,
                                       int32 loop_count);
void
CalcGainVector(const EAP_MdrcDelaysAndGainsInt32 *instance,
               int32 *output,
               const int32 *input,
               int *currCounter,
               int32 *currGain,
               int32 *currDelta,
               int outputFrames);

void
EAP_MdrcDelaysAndGainsInt32_Process(EAP_MdrcDelaysAndGainsInt32 *instance,
                                    int32 *leftLowOutput,
                                    int32 *rightLowOutput,
                                    int32 *leftHighOutput,
                                    int32 *rightHighOutput,
                                    int32 *const *leftLowInputs,
                                    int32 *const *rightLowInputs,
                                    int32 *leftHighInput,
                                    int32 *rightHighInput,
                                    int32 *const *gainInputs,
                                    int frames);

#ifdef __cplusplus
}
#endif

#endif // EAP_MDRC_DELAYS_AND_GAINS_INT32_H
