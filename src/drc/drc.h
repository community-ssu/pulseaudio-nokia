#ifndef DRC_H
#define DRC_H

struct _mumdrc_userdata_t
{
  void *unk; //unknown type
  unsigned int DRCnMemRecs;
  EAP_MemoryRecord *DRCpMemRecs;
  EAP_MultibandDrcControl control;
  EAP_MultibandDrcInt32Handle drc;
};
typedef struct _mumdrc_userdata_t mumdrc_userdata_t;

struct _IMUMDRC_Limiter_Status
{
	int32 limiterThreshold;
	int32 limGain;
	int16 lim_attCoeff;
	int16 lim_relCoeff;
	int16 use_limiter;
};

typedef struct _IMUMDRC_Limiter_Status IMUMDRC_Limiter_Status;

struct _IMUMDRC_Status
{
	int16 linkCoeffSelf;
	int16 linkCoeffOthers;
	int32 attCoeff[5];
	int32 relCoeff[5];
	int32 levelLimits[5][6];
	int32 K[5][7];
	int16 AExp[5][7];
	int16 AFrac[5][7];
	int16 band_count;
	int16 use_mumdrc;
	int32 Gain[5];
	int32 amplitudes[5];
};

typedef struct _IMUMDRC_Status IMUMDRC_Status;

void
mudrc_init(mumdrc_userdata_t *u, const int blockSize,
           const float sampleRate);
void
mudrc_deinit(mumdrc_userdata_t *u);

void
mudrc_set_params(mumdrc_userdata_t *u);

void
mudrc_process(mumdrc_userdata_t *u, int32 *dst_left, int32 *dst_right, int32 *src_left, int32 *src_right, const int samples);

void
limiter_write_parameters(EAP_MultibandDrcInt32Handle handle, const void *data, size_t size);

void
mumdrc_write_parameters(EAP_MultibandDrcInt32Handle handle, const void *data, size_t size);

int
write_limiter_status(EAP_MultibandDrcInt32 *instance, IMUMDRC_Limiter_Status *status);

int
write_mumdrc_status(EAP_MultibandDrcInt32 *instance, IMUMDRC_Status *status);

#endif // DRC_H
