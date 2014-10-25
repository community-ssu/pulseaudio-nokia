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
    int gainMant;
    unsigned int arg;
    int16 fracPol;
    int16 AExp;
    int16 AFrac;
    int16 gainExp;
    int16 fracArg;
    int16 fracArg035;
    int16 fracArg065;
    int32 ampl = amplitudeInput[f];

    for (i = 0; i <= 4 && curveData->levelLimits[i] < ampl; i ++);

    K = curveData->K[i];
    AExp = curveData->AExp[i];
    AFrac = curveData->AFrac[i];

    if (K)
    {
      int16 intArg;
      int16 amplExp = EAP_Norm32(ampl);
      int16 frac16 = (ampl << amplExp >> 15) + -32768;
      int16 frac035 = 11469 * (int16)((ampl << amplExp >> 15) + -32768) >> 15;
      int32 absArg;
      int32 log2Ampl = ((7 - amplExp) << 15) + frac16 + 11469 * (frac16 >> 15) -
          (frac16 * (frac035) >> 15);

      arg = EAP_LongMult32Q15x32(log2Ampl, K);

      if (arg >> 31 )
        absArg = -arg;
      else
        absArg = arg;

      intArg = absArg >> 15;
      fracArg = absArg & 0x7FFF;
      fracArg035 = 11469 * fracArg >> 15;
      fracArg065 = 21299 * fracArg >> 15;
      fracPol = fracArg035 * intArg + fracArg065;

      if (arg >> 31)
        Inverse(&fracPol, &intArg);

      gainExp = intArg + AExp;
      gainMant = fracPol + 32768 + AFrac + (fracPol * AFrac >> 15);

      gainOutput[f] = (((int16)(intArg + AExp) <= 0) ? gainMant >> -gainExp :
                                                       gainMant << gainExp);
    }
    else
    {
      int result = AFrac + 32768;
      gainOutput[f] = ((AExp <= 0) ? (result >> -AExp) : (result << AExp));
    }
  }
}
