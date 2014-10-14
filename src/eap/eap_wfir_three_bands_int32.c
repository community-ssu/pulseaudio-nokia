#include <assert.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "eap_wfir_three_bands_int32.h"

void
EAP_WfirThreeBandsInt32_Init(EAP_WfirInt32 *fir, int sampleRate)
{
  int i;

  assert((sampleRate >= 22000) && (sampleRate <= 25000));

  fir->warpingShift = 3;

  for (i = 0; i < 7; ++i)
  {
    fir[i + 3841].warpingShift = 0;
    fir[i + 3848].warpingShift = 0;
  }

  fir[3855].warpingShift = 0;
  fir[3856].warpingShift = 0;
  fir[3857].warpingShift = 0;
  fir[3858].warpingShift = 0;
  fir[3859].warpingShift = 0;
  fir[3860].warpingShift = 0;
  fir[3861].warpingShift = 0;
  fir[3862].warpingShift = 0;
}
