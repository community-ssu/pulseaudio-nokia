#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>

#include "../eap/eap_memory.h"
#include "../eap/eap_multiband_drc_int32.h"
#include "drc.h"

void
mudrc_init(mumdrc_userdata_t *mumdrc, const int blockSize,
           const float sampleRate)
{
  EAP_MultibandDrcInt32_InitInfo initInfo;

  if (EAP_MultibandDrcControlInt32_Init(&mumdrc->control, sampleRate, 3, 1, 4.0,
                                        1.0, 1000.0, blockSize))
    pa_log_debug("DRC Control Init Failed");

  EAP_MultibandDrcControlInt32_GetProcessingInitInfo(&mumdrc->control,
                                                     &initInfo);
  mumdrc->DRCnMemRecs = EAP_MultibandDrcInt32_MemoryRecordCount(&initInfo);

  mumdrc->DRCpMemRecs = (EAP_MemoryRecord *)
      malloc(mumdrc->DRCnMemRecs * sizeof(EAP_MemoryRecord));

  if (!mumdrc->DRCpMemRecs)
    pa_log_debug("DRCpMemRecs Allocation Failed");

  EAP_MultibandDrcInt32_MemoryNeed(mumdrc->DRCpMemRecs, &initInfo);

  if (EAP_Memory_Alloc(mumdrc->DRCpMemRecs, mumdrc->DRCnMemRecs, NULL, 0))
    pa_log_debug("EAP Memory Allocation Failed");

  mumdrc->drc = EAP_MultibandDrcInt32_Init(mumdrc->DRCpMemRecs, &initInfo);
}
