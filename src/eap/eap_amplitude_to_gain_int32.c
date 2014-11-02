#include "eap_amplitude_to_gain_int32.h"
#include "eap_long_multiplications.h"
#include "eap_normalize.h"

void
EAP_AmplitudeToGainInt32(const EAP_CompressionCurveImplDataInt32 *curveData,
                         int32 *gainOutput,
                         const int32 *amplitudeInput,
                         int frames)
{
  int f;

  for (f = 0; f < frames; f ++)
  {
    int i;
    int32 K;
    int16 AExp;
    int16 AFrac;
    int32 ampl = amplitudeInput[f];

    for (i = 0; i <= 4 && curveData->levelLimits[i] < ampl; i ++);

    K = curveData->K[i];
    AExp = curveData->AExp[i];
    AFrac = curveData->AFrac[i];

    if (K)
    {
      int32 gainMant;
      int32 arg;
      int16 fracPol;
      int16 gainExp;
      int16 fracArg;
      int16 fracArg035;
      int16 fracArg065;
      int32 absArg;
      int16 intArg;
      int16 amplExp = __builtin_clz(abs(ampl)) - 1;
      int16 frac16 = (ampl << amplExp >> 15) + EAP_INT16_MIN;
      int16 frac035 = 11469 * (frac16) >> 15;
      int32 log2Ampl =
          ((7 - amplExp) << 15) + frac16 + frac035 - (frac16 * frac035 >> 15);

      arg = EAP_LongMult32Q15x32(log2Ampl, K);

      absArg = abs(arg);
      intArg = absArg >> 15;
      fracArg = absArg & EAP_INT16_MAX;
      fracArg035 = 11469 * fracArg >> 15;
      fracArg065 = 21299 * fracArg >> 15;
      fracPol = (fracArg035 * fracArg >> 15) + fracArg065;

      if (arg < 0)
        Inverse(&fracPol, &intArg);

      gainExp = intArg + AExp;
      gainMant = fracPol - EAP_INT16_MIN + AFrac + (fracPol * AFrac >> 15);

      gainOutput[f] =
          ((intArg + AExp <= 0) ? gainMant >> -gainExp : gainMant << gainExp);
    }
    else
    {
      int result = AFrac - EAP_INT16_MIN;
      gainOutput[f] = ((AExp <= 0) ? (result >> -AExp) : (result << AExp));
    }
  }
}
