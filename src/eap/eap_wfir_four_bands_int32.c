#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_four_bands_int32.h"

void
EAP_WfirFourBandsInt32_Init(EAP_WfirInt32 *fir, int sampleRate)
{
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  fir->warpingShift = 3;
  fir[3841].warpingShift = 4;

  for ( i = 0; i < 11; ++i )
  {
    fir[i + 3842].warpingShift = 0;
    fir[i + 3853].warpingShift = 0;
  }

  for ( i = 0; i < 6; ++i )
  {
    fir[i + 3864].warpingShift = 0;
    fir[i + 3870].warpingShift = 0;
  }
}
