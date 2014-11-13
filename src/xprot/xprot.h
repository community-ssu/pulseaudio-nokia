#ifndef XPROT_H
#define XPROT_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int16_t int16;
typedef int32_t int32;

struct _XPROT_Constant
{
  int16 x_lm;
  int16 sigma_dp;
  int16 a_1_t;
  int16 a_2_t;
  int16 t_r;
  int16 t_av1;
  int16 t_av2;
  int16 pa1n_asnd[5];
  int16 pa2n_asnd[5];
  int16 s_pa1n;
  int16 s_pa2n;
  int16 b_d;
  int16 a_1_r;
  int16 a_2_r;
  int16 b_1_c;
  int16 b_2_c;
  int16 sigma_c_0;
  int16 t_lm;
  int16 sigma_T_amb;
  int16 b_tv;
  int16 a_tv;
  int16 b_tm;
  int16 a_tm;
  int16 frame_length;
  int16 t_mp;
  int16 a_1_x_d;
  int16 a_2_x_d;
  int16 b_2_x_d;
  int16 b_1_u_d;
  int16 b_2_u_d;
  int32 alfa;
  int32 beta;
};

typedef struct _XPROT_Constant XPROT_Constant;

struct _XPROT_Variable
{
  int16 x_peak;
  int16 LFSN_IIR_w[5];
  int32 LFSN_IIR_d[2];
  int16 DPVL_IIR_w[5];
  int32 DPVL_IIR_d[2];
  int16 DP_IIR_w[3];
  int16 DP_IIR_d[2];
  int32 t_gain_dB_l;
  int16 T_coil_est;
  int16 T_coil_est_old;
  int32 T_v_l_old;
  int32 T_m_l_old;
  int16 lin_vol;
  int16 coef_raw[3];
  int16 t_amb;
  int16 volume;
  int16 u_d;
  int16 x_d;
  int32 u_d_sum;
  int32 x_d_sum;
  int32 prod_raw_rav[3];
};

typedef struct _XPROT_Variable XPROT_Variable;

struct _XPROT_Fixed
{
  int16 x_lm;
  int16 t_rav[3];
  int16 pa1n_asnd[5];
  int16 pa2n_asnd[5];
  int32 s_pa1n;
  int32 s_pa2n;
  int16 b_d;
  int16 t_lm;
  int16 sigma_T_amb;
  int16 t_mp;
  int16 b_tv;
  int16 a_tv;
  int16 b_tm;
  int16 a_tm;
  int16 frame_length;
  int16 stereo_frame_length;
  int16 frame_average;
  int32 alfa;
  int32 beta;
  int32 compute_nltm;
};

typedef struct _XPROT_Fixed XPROT_Fixed;

struct _xprot_channel
{
  XPROT_Variable *variable;
  XPROT_Fixed *fixed;
  XPROT_Constant *constant;
};

typedef struct _xprot_channel xprot_channel;

struct _xprot
{
  XPROT_Constant *xprot_left_const;
  XPROT_Variable *xprot_left_variable;
  XPROT_Fixed *xprot_left_fixed;
  XPROT_Constant *xprot_right_const;
  XPROT_Variable *xprot_right_variable;
  XPROT_Fixed *xprot_right_fixed;
  xprot_channel *xprot_left_channel;
  xprot_channel *xprot_right_channel;
  int16 displ_limit;
  int16 temp_limit;
  int16 stereo;
  int16 apssas;
  int16 volume_level;
};
typedef struct _xprot xprot;

#if 0
struct _nokia_xprot_temperature
{
  pa_core *core;
  xprot *xprot_handle;
  pa_time_event *time_event;
  __int8 input_task_active : 1;
  __int8 xprot_watchdog : 1;
  int ambient_temperature;
};

typedef struct _nokia_xprot_temperature nokia_xprot_temperature;
#endif

void xprot_change_ambient_temperature(xprot *xp, int temperature);
void xprot_change_mode(xprot *xp, int mode);
void xprot_temp_enable(xprot *xp, int enable);
void xprot_displ_enable(xprot *xp, int enable);
void xprot_change_volume(xprot *xp, int16_t q15_vol_L, int16_t q15_vol_R);
void xprot_process_stereo_srcdst(xprot *xp, int16 *src_dst_left, int16 *src_dst_right, int16 length);
void xprot_process_stereo(xprot *xp, int16 *src_left, int16 *src_right, int16 *dst_left, int16 *dst_right, int16 length);
void xprot_free(xprot *xp);
void xprot_change_params(xprot *xp, const void *parameters, size_t length, int channel);
xprot *xprot_new(void);
void a_xprot_func(XPROT_Variable *var, XPROT_Fixed *fix, int16 *in, int16 temp_limit, int16 displ_limit);
void a_xprot_func_s(XPROT_Variable *var_left, XPROT_Fixed *fix_left, XPROT_Variable *var_right, XPROT_Fixed *fix_right, int16 *in_left, int16 *in_right, int16 temp_limit, int16 displ_limit);
void a_xprot_init(XPROT_Variable *var, XPROT_Fixed *fix, XPROT_Constant *cns);
#if 0
void nokia_xprot_temperature_cancel_read_timer(nokia_xprot_temperature *u);
void nokia_xprot_temperature_init(nokia_xprot_temperature *t, pa_core *c, xprot *xprot_handle);
void nokia_xprot_temperature_update(nokia_xprot_temperature *u);
#endif

#ifdef __cplusplus
}
#endif

#endif
