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
mudrc_process(mumdrc_userdata_t *mudrc, int32 *dst_left, int32 *dst_right,
              int32 *src_left, int32 *src_right, const int samples)
{
  EAP_MultibandDrcInt32_Process(mudrc->drc, dst_left, dst_right,
                                src_left, src_right, samples);
}

int
write_limiter_status(EAP_MultibandDrcInt32 *instance,
                     IMUMDRC_Limiter_Status *status)
{
  if (instance && status)
  {
    instance->limiter.m_attCoeff = status->lim_attCoeff;
    instance->limiter.m_threshold = status->limiterThreshold;
    instance->limiter.m_relCoeff = status->lim_relCoeff;

    return 0;
  }

  return -1;
}

void
limiter_write_parameters(EAP_MultibandDrcInt32Handle handle, const void *data,
                         size_t size)
{
  write_limiter_status((EAP_MultibandDrcInt32 *)handle,
                       (IMUMDRC_Limiter_Status *)data);
}

void
mumdrc_write_parameters(EAP_MultibandDrcInt32Handle handle,
                        const void *data, size_t size)
{
  write_mumdrc_status((EAP_MultibandDrcInt32 *)handle,
                      (IMUMDRC_Status *)data);
}

int
read_limiter_status(EAP_MultibandDrcInt32 *instance,
                    IMUMDRC_Limiter_Status *status)
{
  if (status && instance)
  {
    status->lim_attCoeff = instance->limiter.m_attCoeff;
    status->limiterThreshold = instance->limiter.m_threshold;
    status->lim_relCoeff = instance->limiter.m_relCoeff;

    return 0;
  }

  return -1;
}

int
set_drc_volume(mumdrc_userdata_t *u, float volume)
{
  EAP_MdrcInternalEventCompressionCurveInt32 compressionCurveEvent;

  if (EAP_MultibandDrcControlInt32_UpdateVolumeSetting(&u->control,
                                                       &compressionCurveEvent,
                                                       volume,
                                                       0))
      goto error;

  EAP_MultibandDrcInt32_Update(u->drc,&compressionCurveEvent.common);

  if (EAP_MultibandDrcControlInt32_UpdateVolumeSetting(&u->control,
                                                       &compressionCurveEvent,
                                                       volume,
                                                       1))
    goto error;

  EAP_MultibandDrcInt32_Update(u->drc, &compressionCurveEvent.common);

  if (EAP_MultibandDrcControlInt32_UpdateVolumeSetting(&u->control,
                                                       &compressionCurveEvent,
                                                       volume,
                                                       2))
    goto error;

  EAP_MultibandDrcInt32_Update(u->drc, &compressionCurveEvent.common);

  return 0;

error:
  pa_log_debug("EAP_MultibandDrcControlInt32_UpdateVolumeSetting FAILED");
  return 1;
}

void
write_mumdrc_variable_volume_params(mumdrc_userdata_t *u)
{
  EAP_MultibandDrcControlInt32 *control;
  EAP_MdrcCompressionCurveSet *curveSet;
  EAP_MdrcInternalEventCompressionCurveInt32 compressionCurveEvent;
  EAP_MdrcInternalEventCompanderReleaseCoeffInt32 companderReleaseCoeffEvent;
  EAP_MdrcInternalEventCompanderAttackCoeffInt32 companderAttackCoeffEvent;
  EAP_MdrcInternalEventCrossBandLinkInt32 crossBandLinkEevent;
  EAP_MdrcInternalEventLimiterThresholdInt32 limiterThresholdEvent;
  EAP_MdrcInternalEventLimiterReleaseCoeffInt32 limiterReleaseEvent;
  EAP_MdrcInternalEventLimiterAttackCoeffInt32 limiterAttackEvent;
  EAP_MdrcCompressionCurve curve[3][2];
  float releaseTimeMs[3];
  float attackTimeMs[3];
  EAP_MdrcCompressionCurveSet set[3] =
  {
    {2, curve[0]},
    {2, curve[1]},
    {2, curve[2]},
  };
  int i, j;

  curveSet = set;

  releaseTimeMs[0] = 70.0;
  releaseTimeMs[1] = 50.0;
  releaseTimeMs[2] = 50.0;

  control = &u->control;


  attackTimeMs[0] = 10.0;
  attackTimeMs[1] = 2.0;
  attackTimeMs[2] = 2.0;

  for (i = 0; i < 3; i ++)
  {
    for (j = 0; j < 2; j ++)
    {
      curve[i][j].limitLevel = 0;
      curve[i][j].volume = -10.0;
      curve[i][j].inputLevels[0] = -75.0;
      curve[i][j].inputLevels[1] = -25.0;
      curve[i][j].inputLevels[2] = -15.0;
      curve[i][j].inputLevels[3] = 0;
      curve[i][j].inputLevels[4] = 25.0;
    }
  }

  for (i = 0; i < 3; i ++)
  {
    curve[i][0].outputLevels[0] = -75.0;
    curve[i][0].outputLevels[1] = -25.0;
    curve[i][0].outputLevels[2] = -15.0;
    curve[i][0].outputLevels[3] = 0;
    curve[i][0].outputLevels[4] = 25.0;
  }

  curve[0][1].outputLevels[0] = -85.0;
  curve[0][1].outputLevels[1] = -35.0;
  curve[0][1].outputLevels[2] = -25.0;
  curve[0][1].outputLevels[3] = -10.0;
  curve[0][1].outputLevels[4] = -10.0;

  curve[1][1].outputLevels[0] = -60.0;
  curve[1][1].outputLevels[1] = -12.0;
  curve[1][1].outputLevels[2] = -7.0;
  curve[1][1].outputLevels[3] = -6.0;
  curve[1][1].outputLevels[4] = -5.0;

  curve[2][1].outputLevels[0] = -60.0;
  curve[2][1].outputLevels[1] = -12.0;
  curve[2][1].outputLevels[2] = -7.0;
  curve[2][1].outputLevels[3] = -6.0;
  curve[2][1].outputLevels[4] = -5.0;

  for (i = 0; i < 3; i ++, curveSet ++)
  {
    if (EAP_MultibandDrcControlInt32_UpdateCompressionCurveSet(
          (EAP_MultibandDrcControl *)control,
          &compressionCurveEvent,
          curveSet,
          i))
      pa_log_debug(
            "EAP_MultibandDrcControlInt32_UpdateCompressionCurveSet FAILED");
    else
      EAP_MultibandDrcInt32_Update(u->drc, &compressionCurveEvent.common);

    if (EAP_MultibandDrcControlInt32_UpdateVolumeSetting(
          control, &compressionCurveEvent, 7.0, i))
      pa_log_debug("EAP_MultibandDrcControlInt32_UpdateVolumeSetting FAILED");
    else
      EAP_MultibandDrcInt32_Update(u->drc, &compressionCurveEvent.common);

    if (EAP_MultibandDrcControlInt32_UpdateEQLevel(control,
                                                   &compressionCurveEvent,
                                                   0.0, 0, i))
      pa_log_debug("EAP_MultibandDrcControlInt32_UpdateEQLevel FAILED");
    else
      EAP_MultibandDrcInt32_Update(u->drc, &compressionCurveEvent.common);

    if (EAP_MultibandDrcControlInt32_UpdateCompanderAttack(
          control,&companderAttackCoeffEvent, attackTimeMs[i], i))
      pa_log_debug(
            "EAP_MultibandDrcControlInt32_UpdateCompanderAttack FAILED");
    else
      EAP_MultibandDrcInt32_Update(u->drc, &companderAttackCoeffEvent.common);

    if (EAP_MultibandDrcControlInt32_UpdateCompanderRelease(
          control,&companderReleaseCoeffEvent, releaseTimeMs[i], i))
      pa_log_debug(
            "EAP_MultibandDrcControlInt32_UpdateCompanderRelease FAILED");
    else
      EAP_MultibandDrcInt32_Update(u->drc, &companderReleaseCoeffEvent.common);
  }

  if (EAP_MultibandDrcControlInt32_UpdateLimiterAttack(
        control, &limiterAttackEvent, 0.25))
    pa_log_debug("EAP_MultibandDrcControlInt32_UpdateLimiterAttack FAILED");
  else
    EAP_MultibandDrcInt32_Update(u->drc, &limiterAttackEvent.common);

  if (EAP_MultibandDrcControlInt32_UpdateLimiterRelease(
        control,&limiterReleaseEvent, 20.0))
    pa_log_debug("EAP_MultibandDrcControlInt32_UpdateLimiterRelease FAILED");
  else
    EAP_MultibandDrcInt32_Update(u->drc, &limiterReleaseEvent.common);

  if (EAP_MultibandDrcControlInt32_UpdateLimiterThreshold(
        control, &limiterThresholdEvent, 0.8414))
    pa_log_debug(
          "EAP_MultibandDrcControlInt32_UpdateLimiterThreshold FAILED");
  else
    EAP_MultibandDrcInt32_Update(u->drc, &limiterThresholdEvent.common);

  if (EAP_MultibandDrcControlInt32_UpdateCrossBandLink(
        control, &crossBandLinkEevent, 0.0))
    pa_log_debug("EAP_MultibandDrcControlInt32_UpdateCrossBandLink FAILED");
  else
    EAP_MultibandDrcInt32_Update(u->drc, &crossBandLinkEevent.common);
}

int
read_mumdrc_status(EAP_MultibandDrcInt32 *instance, IMUMDRC_Status *status)
{
  //todo
  return 0;
}

int
write_mumdrc_status(EAP_MultibandDrcInt32 *instance, IMUMDRC_Status *status)
{
  //todo
  return 0;
}
