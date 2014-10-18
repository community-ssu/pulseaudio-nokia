#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>

#include "../eap/eap_memory.h"
#include "../eap/eap_multiband_drc_int32.h"
#include "../eap/eap_multiband_drc_control_int32.h"

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

void
mudrc_set_params(mumdrc_userdata_t *mumdrc)
{
}

void
mudrc_deinit(mumdrc_userdata_t *mumdrc)
{
	pa_log_debug("DRC DeInit Called");
	EAP_MultibandDrcControlInt32_DeInit(&mumdrc->control);
	EAP_Memory_Free(mumdrc->DRCpMemRecs, mumdrc->DRCnMemRecs);
	free(mumdrc->DRCpMemRecs);
	free(mumdrc->unk);
}

void
mudrc_process(mumdrc_userdata_t *mudrc, int32 *dst_left, int32 *dst_right, int32 *src_left, int32 *src_right, const int samples)
{
	EAP_MultibandDrcInt32_Process(mudrc->drc, dst_left, dst_right, src_left, src_right, samples);
}

int
write_limiter_status(EAP_MultibandDrcInt32 *instance, IMUMDRC_Limiter_Status *status)
{
	if (instance && status)
	{
		instance->limiter.m_attCoeff = status->lim_attCoeff;
		instance->limiter.m_threshold = status->limiterThreshold;
		instance->limiter.m_relCoeff = status->lim_relCoeff;
		return 0;
	}
	else
	{
		return -1;
	}
}

void
limiter_write_parameters(EAP_MultibandDrcInt32Handle handle, const void *data, size_t size)
{
	write_limiter_status((EAP_MultibandDrcInt32 *) handle, (IMUMDRC_Limiter_Status *) data);
}

void
mumdrc_write_parameters(EAP_MultibandDrcInt32Handle handle, const void *data, size_t size)
{
	write_mumdrc_status((EAP_MultibandDrcInt32 *) handle, (IMUMDRC_Status *) data);
}

int
write_mumdrc_status(EAP_MultibandDrcInt32 *instance, IMUMDRC_Status *status)
{
	//todo
}
