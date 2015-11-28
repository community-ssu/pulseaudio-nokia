#ifndef XPROT_H
#define XPROT_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _XPROT_Constant
{
  int16_t x_lm;
  int16_t sigma_dp;
  int16_t a_1_t;
  int16_t a_2_t;
  int16_t t_r;
  int16_t t_av1;
  int16_t t_av2;
  int16_t pa1n_asnd[5];
  int16_t pa2n_asnd[5];
  int16_t s_pa1n;
  int16_t s_pa2n;
  int16_t b_d;
  int16_t a_1_r;
  int16_t a_2_r;
  int16_t b_1_c;
  int16_t b_2_c;
  int16_t sigma_c_0;
  int16_t t_lm;
  int16_t sigma_T_amb;
  int16_t b_tv;
  int16_t a_tv;
  int16_t b_tm;
  int16_t a_tm;
  int16_t frame_length;
  int16_t t_mp;
  int16_t a_1_x_d;
  int16_t a_2_x_d;
  int16_t b_2_x_d;
  int16_t b_1_u_d;
  int16_t b_2_u_d;
  int32_t alfa;
  int32_t beta;
};

typedef struct _XPROT_Constant XPROT_Constant;

struct _XPROT_Variable
{
  int16_t x_peak;
  int16_t LFSN_IIR_w[5];
  int32_t LFSN_IIR_d[2];
  int16_t DPVL_IIR_w[5];
  int32_t DPVL_IIR_d[2];
  int16_t DP_IIR_w[3];
  int16_t DP_IIR_d[2];
  int32_t t_gain_dB_l;
  int16_t T_coil_est;
  int16_t T_coil_est_old;
  int32_t T_v_l_old;
  int32_t T_m_l_old;
  int16_t lin_vol;
  int16_t coef_raw[3];
  int16_t t_amb;
  int16_t volume;
  int16_t u_d;
  int16_t x_d;
  int32_t u_d_sum;
  int32_t x_d_sum;
  int32_t prod_raw_rav[3];
};

typedef struct _XPROT_Variable XPROT_Variable;

struct _XPROT_Fixed
{
  int16_t x_lm;
  int16_t t_rav[3];
  int16_t pa1n_asnd[5];
  int16_t pa2n_asnd[5];
  int32_t s_pa1n;
  int32_t s_pa2n;
  int16_t b_d;
  int16_t t_lm;
  int16_t sigma_T_amb;
  int16_t t_mp;
  int16_t b_tv;
  int16_t a_tv;
  int16_t b_tm;
  int16_t a_tm;
  int16_t frame_length;
  int16_t stereo_frame_length;
  int16_t frame_average;
  int32_t alfa;
  int32_t beta;
  int32_t compute_nltm;
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
  int16_t displ_limit;
  int16_t temp_limit;
  int16_t stereo;
  int16_t apssas;
  int16_t volume_level;
};
typedef struct _xprot xprot;

void xprot_change_ambient_temperature(xprot *xp, int temperature);
void xprot_change_mode(xprot *xp, int mode);
void xprot_temp_enable(xprot *xp, int enable);
void xprot_displ_enable(xprot *xp, int enable);
void xprot_change_volume(xprot *xp, int16_t q15_vol_L, int16_t q15_vol_R);
void xprot_process_stereo_srcdst(xprot *xp, int16_t *src_dst_left,
                                 int16_t *src_dst_right, int16_t length);
void xprot_process_stereo(xprot *xp, int16_t *src_left, int16_t *src_right,
                          int16_t *dst_left, int16_t *dst_right,
                          int16_t length);
void xprot_free(xprot *xp);
void xprot_change_params(xprot *xp, const void *parameters, size_t length,
                         int channel);
xprot *xprot_new(void);
void a_xprot_func(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in,
                  int16_t temp_limit, int16_t displ_limit);
void a_xprot_func_s(XPROT_Variable *var_left, XPROT_Fixed *fix_left,
                    XPROT_Variable *var_right, XPROT_Fixed *fix_right,
                    int16_t *in_left, int16_t *in_right,
                    int16_t temp_limit, int16_t displ_limit);
void a_xprot_init(XPROT_Variable *var, XPROT_Fixed *fix, XPROT_Constant *cns);
#ifdef __cplusplus
}
#endif

#endif
