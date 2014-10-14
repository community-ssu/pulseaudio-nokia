#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_five_bands_int32.h"

void
EAP_WfirFiveBandsInt32_Init(EAP_WfirInt32 *fir, signed int sampleRate)
{
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  fir->warpingShift = 2;

  for ( i = 0; i < 15; ++i )
  {
    fir[i + 3841].warpingShift = 0;
    fir[i + 3856].warpingShift = 0;
  }

  for ( i = 0; i < 8; ++i )
  {
    fir[i + 3871].warpingShift = 0;
    fir[i + 3879].warpingShift = 0;
  }
}
