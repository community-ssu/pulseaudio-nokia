#ifndef EAP_WFIR_TWO_BANDS_INT32_H
#define EAP_WFIR_TWO_BANDS_INT32_H

#include "eap_wfir_int32.h"

void
EAP_WfirTwoBandsInt32_Init(EAP_WfirInt32 *fir, int sampleRate);

void
EAP_WfirTwoBandsInt32_filter(int *fl, int *fr, int **l, int **r,
                             int samplesCount);

#endif // EAP_WFIR_TWO_BANDS_INT32_H
