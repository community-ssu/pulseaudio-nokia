typedef void *EAP_MultibandDrcInt32Handle;

struct EAP_MdrcCompressionCurve
{
	float inputLevels[5];
	float outputLevels[5];
	float limitLevel;
	float volume;
};

struct EAP_MdrcCompressionCurveSet
{
	int curveCount;
	EAP_MdrcCompressionCurve *curves;
};

struct EAP_MultibandDrcControl
{
	float m_sampleRate;
	int m_bandCount;
	int m_downSamplingFactor;
	float m_companderLookahead;
	float m_limiterLookahead;
	int m_maxBlockSize;
	float m_oneOverFactor;
	float m_volume[5];
	EAP_MdrcCompressionCurveSet m_curveSet[5];
	int m_eqCount;
	float **m_eqCurves;
};

enum EAP_MemoryType
{
	EAP_MEMORY_PERSISTENT = 0x0,
	EAP_MEMORY_SCRATCH = 0x1,
};

struct EAP_MemoryRecord
{
	size_t size;
	size_t alignment;
	EAP_MemoryType type;
	int location;
	void *base;
	int freeThisBlock;
};

struct mumdrc_userdata_t
{
	unsigned int DRCnMemRecs;
	EAP_MemoryRecord *DRCpMemRecs;
	EAP_MultibandDrcControl control;
	EAP_MultibandDrcInt32Handle drc;
};

struct userdata
{
	pa_core *core;
	pa_module *module;
	meego_algorithm_base *base;
	meego_algorithm_hook_slot *mumdrc_hook_slot;
	float mumdrc_volume;
	pa_bool_t mumdrc_enabled;
	mumdrc_userdata_t *mumdrc_data;
};

struct IMUMDRC_Status
{
	short linkCoeffSelf;
	short linkCoeffOthers;
	int32_t attCoeff[5];
	int32_t relCoeff[5];
	int32_t levelLimits[5][6];
	int32_t K[5][7];
	short AExp[5][7];
	short AFrac[5][7];
	short band_count;
	short use_mumdrc;
	int32_t Gain[5];
	int32_t amplitudes[5];
};

struct IMUMDRC_Limiter_Status
{
	int32_t limiterThreshold;
	int32_t limGain;
	short lim_attCoeff;
	short lim_relCoeff;
	short use_limiter;
};

mumdrc_userdata_t *mumdrc_init(const int blockSize, const float sampleRate);
int set_drc_volume(mumdrc_userdata_t *u, float volume);
void mumdrc_deinit(mumdrc_userdata_t *u);
void mumdrc_process(mumdrc_userdata_t *u, int32_t *dst_left, int32_t *dst_right, int32_t *src_left, int32_t *src_right, const int samples);
int set_drc_volume(mumdrc_userdata_t *u, float volume);
int write_mumdrc_params(mumdrc_userdata_t *u, IMUMDRC_Status *params, unsigned int length);
int write_limiter_params(mumdrc_userdata_t *u, IMUMDRC_Limiter_Status *params, unsigned int length);
