#include "eap_multiband_drc_int32.h"

void
EAP_AmplitudeToGainInt32(const EAP_CompressionCurveImplDataInt32 *curveData, int32 *gainOutput, const int32 *amplitudeInput, int frames);
