#ifndef EAP_AMPLITUDE_TO_GAIN_INT32_H
#define EAP_AMPLITUDE_TO_GAIN_INT32_H

#include "eap_multiband_drc_int32.h"

#ifdef __cplusplus
extern "C" {
#endif

void
EAP_AmplitudeToGainInt32(const EAP_CompressionCurveImplDataInt32 *curveData,
                         int32 *gainOutput,
                         const int32 *amplitudeInput,
                         int frames);

#ifdef __cplusplus
}
#endif

#endif // EAP_AMPLITUDE_TO_GAIN_INT32_H
