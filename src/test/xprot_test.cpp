#include <assert.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <QByteArray>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#define PACKAGE pulseaudio

#include "../xprot/xprot.h"
#include "../xprot/dsp.h"

#define timing(a) \
{ \
  clock_t start=clock(), diff; \
  int msec; \
  do { \
    a; \
  } \
  while(0); \
  diff = clock() - start; \
  msec = diff * 1000 / CLOCKS_PER_SEC; \
  printf("msecs: %d\n",msec); \
}

int32_t SAMPLES;
int16_t *src;
int16_t *dst;
int16_t *dstn;

#define check_xp(var) assert(a->var == b->var)

#define check_xp_(s, var) \
{ \
  assert(a->xprot_left_##s->var == b->xprot_left_##s->var); \
  assert(a->xprot_right_##s->var == b->xprot_right_##s->var); \
}
#define check_xp_array_(s, var) \
{ \
  assert(!memcmp(a->xprot_left_##s->var, b->xprot_left_##s->var, sizeof(a->xprot_left_##s->var))); \
  assert(!memcmp(a->xprot_right_##s->var, b->xprot_right_##s->var, sizeof(a->xprot_right_##s->var))); \
}

#define check_xp_var(var) check_xp_(variable, var)
#define check_xp_var_array(var) check_xp_array_(variable, var)

#define check_xp_const(var) check_xp_(const, var)
#define check_xp_const_array(var) check_xp_array_(const, var)

#define check_xp_fixed(var) check_xp_(fixed, var)
#define check_xp_fixed_array(var) check_xp_array_(fixed, var)

void compare_xp(xprot *a, xprot *b)
{
  check_xp(displ_limit);
  check_xp(temp_limit);
  check_xp(stereo);
  check_xp(apssas);
  check_xp(volume_level);

  check_xp_var(x_peak);
  check_xp_var_array(LFSN_IIR_w);
  check_xp_var_array(LFSN_IIR_d);
  check_xp_var_array(DPVL_IIR_w);
  check_xp_var_array(DPVL_IIR_d);
  check_xp_var_array(DP_IIR_w);
  check_xp_var_array(DP_IIR_d);
  check_xp_var(t_gain_dB_l);
  check_xp_var(T_coil_est);
  check_xp_var(T_coil_est_old);
  check_xp_var(T_v_l_old);
  check_xp_var(T_m_l_old);
  check_xp_var(lin_vol);
  check_xp_var_array(coef_raw);
  check_xp_var(t_amb);
  check_xp_var(volume);
  check_xp_var(u_d);
  check_xp_var(x_d);
  check_xp_var(u_d_sum);
  check_xp_var(x_d_sum);
  check_xp_var_array(prod_raw_rav);

  check_xp_const(x_lm);
  check_xp_const(sigma_dp);
  check_xp_const(a_1_t);
  check_xp_const(a_2_t);
  check_xp_const(t_r);
  check_xp_const(t_av1);
  check_xp_const(t_av2);
  check_xp_const_array(pa1n_asnd);
  check_xp_const_array(pa2n_asnd);
  check_xp_const(s_pa1n);
  check_xp_const(s_pa2n);
  check_xp_const(b_d);
  check_xp_const(a_1_r);
  check_xp_const(a_2_r);
  check_xp_const(b_1_c);
  check_xp_const(b_2_c);
  check_xp_const(sigma_c_0);
  check_xp_const(t_lm);
  check_xp_const(sigma_T_amb);
  check_xp_const(b_tv);
  check_xp_const(a_tv);
  check_xp_const(b_tm);
  check_xp_const(a_tm);
  check_xp_const(frame_length);
  check_xp_const(t_mp);
  check_xp_const(a_1_x_d);
  check_xp_const(a_2_x_d);
  check_xp_const(b_2_x_d);
  check_xp_const(b_1_u_d);
  check_xp_const(b_2_u_d);
  check_xp_const(alfa);
  check_xp_const(beta);

  check_xp_fixed(x_lm);
  check_xp_fixed(t_rav[3]);
  check_xp_fixed_array(pa1n_asnd);
  check_xp_fixed_array(pa2n_asnd);
  check_xp_fixed(s_pa1n);
  check_xp_fixed(s_pa2n);
  check_xp_fixed(b_d);
  check_xp_fixed(t_lm);
  check_xp_fixed(sigma_T_amb);
  check_xp_fixed(t_mp);
  check_xp_fixed(b_tv);
  check_xp_fixed(a_tv);
  check_xp_fixed(b_tm);
  check_xp_fixed(a_tm);
  check_xp_fixed(frame_length);
  check_xp_fixed(stereo_frame_length);
  check_xp_fixed(frame_average);
  check_xp_fixed(alfa);
  check_xp_fixed(beta);
  check_xp_fixed(compute_nltm);

  assert(!memcmp(dst, dstn, SAMPLES));
}

typedef void
(*xprot_change_params_f)(xprot *xp, const void *parameters, size_t length,
                    int channel);

extern "C" void
a_xprot_temp_limiter(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in);
typedef void
(*a_xprot_temp_limiter_f)(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in);

extern "C" int16_t
a_xprot_dp_filter(int16_t *in, int16_t *w, int16_t *d, int volume, int length);

typedef int16_t
(*a_xprot_dp_filter_f)(int16_t *in, int16_t *w, int16_t *d, int volume, int length);

extern "C" void
a_xprot_dpvl_mono(const int16_t *in, XPROT_Variable *var, int lenght);

typedef void
(*a_xprot_dpvl_mono_f)(const int16_t *in, XPROT_Variable *var, int lenght);

extern "C" void
a_xprot_coeff_calc(XPROT_Variable *var, XPROT_Fixed *fix);

typedef void
(*a_xprot_coeff_calc_f)(XPROT_Variable *var, XPROT_Fixed *fix);

extern "C" void
a_xprot_temp_predictor(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in);

typedef void
(*a_xprot_temp_predictor_f)(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in);

extern "C" void
a_xprot_lfsn_mono(int16_t *in, XPROT_Variable *var, int16_t t, int length);

typedef void
(*a_xprot_lfsn_mono_f)(int16_t *in, XPROT_Variable *var, int16_t t, int length);

extern "C" void
a_xprot_func_s(XPROT_Variable *var_left, XPROT_Fixed *fix_left,
               XPROT_Variable *var_right, XPROT_Fixed *fix_right,
               int16_t *in_left, int16_t *in_right,
               int16_t temp_limit, int16_t displ_limit);

typedef void
(*a_xprot_func_s_f)(XPROT_Variable *var_left, XPROT_Fixed *fix_left,
               XPROT_Variable *var_right, XPROT_Fixed *fix_right,
               int16_t *in_left, int16_t *in_right,
               int16_t temp_limit, int16_t displ_limit);

extern "C" void
a_xprot_func(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in,
             int16_t temp_limit, int16_t displ_limit);

typedef void
(*a_xprot_func_f)(XPROT_Variable *var, XPROT_Fixed *fix, int16_t *in,
             int16_t temp_limit, int16_t displ_limit);

extern "C" void
a_xprot_lfsn_stereo(int16_t *in_left, int16_t *in_right,
                    XPROT_Variable *var_left, XPROT_Variable *var_right,
                    int16_t t, int length);

typedef void
(*a_xprot_lfsn_stereo_f)(int16_t *in_left, int16_t *in_right,
                    XPROT_Variable *var_left, XPROT_Variable *var_right,
                    int16_t t, int length);

typedef void
(*a_xprot_func_s_f)(XPROT_Variable *var_left, XPROT_Fixed *fix_left,
               XPROT_Variable *var_right, XPROT_Fixed *fix_right,
               int16_t *in_left, int16_t *in_right,
               int16_t temp_limit, int16_t displ_limit);

int main(int argc, char *argv[])
{
  int i;

  FILE* fp = fopen("/opt/xprot_test/bin/xprot.raw", "r"), *fpout;
  assert(fp);
  fseek (fp, 0, SEEK_END);
  SAMPLES = ftell(fp);
  fseek (fp, 0, SEEK_SET);

  src = (int16_t *)malloc(sizeof(int16_t) * SAMPLES);
  dst = (int16_t *)calloc(sizeof(int16_t), SAMPLES);
  dstn = (int16_t *)calloc(sizeof(int16_t), SAMPLES);

  SAMPLES /= 2 * sizeof(int32_t);

  printf("Reading %d samples\n", SAMPLES);

  for (i = 0; i < SAMPLES; i ++)
  {
    int32_t sample;
    fread(&sample, 4, 1, fp);

    dst[i] = dstn[i] = src[i] = sample >> 16;
    fread(&sample, 4, 1, fp);

    dst[i + SAMPLES] = dstn[i + SAMPLES] = src[i + SAMPLES] = sample >> 16;
  }

  void *h = dlopen("/usr/lib/pulse-0.9.15/modules/module-nokia-voice.so", RTLD_LAZY);

  xprot* (*xp_new)() = ( xprot*(*)())dlsym(h, "xprot_new");
  xprot_change_params_f xp_change_params = (xprot_change_params_f)dlsym(h, "xprot_change_params");

  Dl_info dlinfo;
  dladdr((void*)xp_new, &dlinfo);

  a_xprot_temp_limiter_f a_xp_temp_limiter = (a_xprot_temp_limiter_f)(((char*)dlinfo.dli_fbase) + 0x28458);
  a_xprot_dp_filter_f a_xp_dp_filter = (a_xprot_dp_filter_f)(((char*)dlinfo.dli_fbase) + 0x28B2C);
  a_xprot_dpvl_mono_f a_xp_dpvl_mono = (a_xprot_dpvl_mono_f)(((char*)dlinfo.dli_fbase) + 0x28C30);
  a_xprot_coeff_calc_f a_xp_coeff_calc = (a_xprot_coeff_calc_f)(((char*)dlinfo.dli_fbase) + 0x282C4);
  a_xprot_temp_predictor_f a_xp_temp_predictor = (a_xprot_temp_predictor_f)(((char*)dlinfo.dli_fbase) + 0x27F30);
  a_xprot_lfsn_mono_f a_xp_lfsn_mono = (a_xprot_lfsn_mono_f)(((char*)dlinfo.dli_fbase) + 0x28DC8);
  a_xprot_lfsn_stereo_f a_xp_lfsn_stereo = (a_xprot_lfsn_stereo_f)(((char*)dlinfo.dli_fbase) + 0x29018);
  a_xprot_func_s_f a_xp_func_s = (a_xprot_func_s_f)(((char*)dlinfo.dli_fbase) + 0x2855C);

  a_xprot_func_f a_xp_func = (a_xprot_func_f)(((char*)dlinfo.dli_fbase) + 0x28754);

  QByteArray param = QByteArray::fromHex("8c09c7004277ce8fc77f4404bc7bfa85014964a1615368e28751efa8df7fb68db6280040922de722ee8acd6b1f8bae6bff7f0020f6288f04598264002d80f00000001b89be6fc2012a040efd0000000000000000");

  XPROT_Constant *c = (XPROT_Constant *)param.constData();
  /*
  c->alfa = 1000;
  c->beta = 10;
  */
  xprot* xpn = xp_new();
  xprot* xp = xprot_new();

  xp_change_params(xpn, c, param.length(), 0);
  xprot_change_params(xp, c, param.length(), 0);
  xp_change_params(xpn, c, param.length(), 1);
  xprot_change_params(xp, c, param.length(), 1);

  /*xpn->xprot_left_fixed->x_lm = xp->xprot_left_fixed->x_lm = 100;*/
  compare_xp(xpn, xp);

  /*xpn->xprot_left_variable->T_coil_est =
      xp->xprot_left_variable->T_coil_est = 10;

  xpn->xprot_left_fixed->t_lm =
      xp->xprot_left_fixed->t_lm = 80;

  xpn->xprot_left_fixed->t_mp =
      xp->xprot_left_fixed->t_mp = 180;*/

  xprot_change_ambient_temperature(xpn, 30);
  xprot_change_ambient_temperature(xp, 30);

  printf("Comparing stock vs foss results for first %d frames \n", 64);
  for(i = 0; i < 64; i++)
  {
    a_xp_temp_limiter(xpn->xprot_left_variable,
                      xpn->xprot_left_fixed,
                      dstn + xpn->xprot_left_fixed->frame_length * i);
    a_xprot_temp_limiter(xp->xprot_left_variable,
                         xp->xprot_left_fixed,
                         dst + xp->xprot_left_fixed->frame_length * i);

    compare_xp(xpn, xp);

    int16_t x_peakn = a_xp_dp_filter(dstn+ xpn->xprot_left_fixed->frame_length * i,
                                     xpn->xprot_left_variable->DP_IIR_w,
                                     xpn->xprot_left_variable->DP_IIR_d,
                                     xpn->xprot_left_variable->lin_vol,
                                     xpn->xprot_left_fixed->frame_length);

    int16_t x_peak = a_xprot_dp_filter(dst + xp->xprot_left_fixed->frame_length * i,
                                       xp->xprot_left_variable->DP_IIR_w,
                                       xp->xprot_left_variable->DP_IIR_d,
                                       xp->xprot_left_variable->lin_vol,
                                       xp->xprot_left_fixed->frame_length);
    assert(x_peakn == x_peak);
    compare_xp(xpn, xp);

    if (xpn->xprot_left_variable->x_peak < x_peakn)
      xpn->xprot_left_variable->x_peak = x_peakn;
    xpn->xprot_left_variable->x_peak =
        __sat_mul_add_16(xpn->xprot_left_fixed->t_rav[0],
                         xpn->xprot_left_variable->x_peak) >> 16;

    if (xp->xprot_left_variable->x_peak < x_peak)
      xp->xprot_left_variable->x_peak = x_peak;
    xp->xprot_left_variable->x_peak =
        __sat_mul_add_16(xp->xprot_left_fixed->t_rav[0],
                         xp->xprot_left_variable->x_peak) >> 16;

    compare_xp(xpn, xp);

    a_xp_coeff_calc(xpn->xprot_left_variable, xpn->xprot_left_fixed);
    a_xprot_coeff_calc(xp->xprot_left_variable, xp->xprot_left_fixed);

    compare_xp(xpn, xp);

    a_xp_lfsn_mono(dstn + xpn->xprot_left_fixed->frame_length * i,
                   xpn->xprot_left_variable,
                   xpn->xprot_left_fixed->t_rav[2],
                   xpn->xprot_left_fixed->frame_length);
    a_xprot_lfsn_mono(dst + xp->xprot_left_fixed->frame_length * i,
                      xp->xprot_left_variable,
                      xp->xprot_left_fixed->t_rav[2],
                      xp->xprot_left_fixed->frame_length);

    compare_xp(xpn, xp);

    a_xp_lfsn_stereo(dstn + xpn->xprot_left_fixed->frame_length * i,
                     dstn + xpn->xprot_left_fixed->frame_length * i + SAMPLES,
                     xpn->xprot_left_variable,
                     xpn->xprot_right_variable,
                     xpn->xprot_left_fixed->t_rav[2],
                     xpn->xprot_left_fixed->frame_length);
    a_xprot_lfsn_stereo(dst + xp->xprot_left_fixed->frame_length * i,
                        dst + xp->xprot_left_fixed->frame_length * i + SAMPLES,
                        xp->xprot_left_variable,
                        xp->xprot_right_variable,
                        xp->xprot_left_fixed->t_rav[2],
                        xp->xprot_left_fixed->frame_length);

    compare_xp(xpn, xp);

    a_xp_dpvl_mono(dstn + xpn->xprot_left_fixed->frame_length * i,
                   xpn->xprot_left_variable,
                   xpn->xprot_left_fixed->frame_length);

    a_xprot_dpvl_mono(dst + xp->xprot_left_fixed->frame_length * i,
                      xp->xprot_left_variable,
                      xp->xprot_left_fixed->frame_length);

    compare_xp(xpn, xp);

    a_xp_temp_predictor(xpn->xprot_left_variable,
                        xpn->xprot_left_fixed,
                        dstn + xpn->xprot_left_fixed->frame_length * i);
    a_xprot_temp_predictor(xp->xprot_left_variable,
                           xp->xprot_left_fixed,
                           dst + xp->xprot_left_fixed->frame_length * i);

    compare_xp(xpn, xp);

    /* put them together */
    a_xp_func(xpn->xprot_left_variable,
              xpn->xprot_left_fixed, dstn + xpn->xprot_left_fixed->frame_length * i, 1, 1);
    a_xprot_func(xp->xprot_left_variable,
              xp->xprot_left_fixed, dst + xp->xprot_left_fixed->frame_length * i, 1, 1);

    compare_xp(xpn, xp);

    a_xp_func_s(xpn->xprot_left_variable,
                xpn->xprot_left_fixed,
                xpn->xprot_right_variable,
                xpn->xprot_right_fixed,
                dstn + xpn->xprot_left_fixed->frame_length * i,
                dstn + xpn->xprot_left_fixed->frame_length * i + SAMPLES,
                1, 1);
    a_xprot_func_s(xp->xprot_left_variable,
                   xp->xprot_left_fixed,
                   xp->xprot_right_variable,
                   xp->xprot_right_fixed,
                   dst + xp->xprot_left_fixed->frame_length * i,
                   dst + xp->xprot_left_fixed->frame_length * i + SAMPLES,
                   1, 1);

    compare_xp(xpn, xp);
  }

  printf("Timing a_xprot_func() over %d frames %d bytes each\n",
         SAMPLES / xp->xprot_left_fixed->frame_length,
         xp->xprot_left_fixed->frame_length);
  printf("stock\n");
  for(int k = 0; k < 3; k++)
  timing(
  for(int j = 0; j < 50; j++)
  for(i = 0; i< SAMPLES / xp->xprot_left_fixed->frame_length; i++)
  a_xp_func(xp->xprot_left_variable,
            xp->xprot_left_fixed, dst + xp->xprot_left_fixed->frame_length * i, 1, 1);)

  printf("foss\n");
  for(int k = 0; k < 3; k++)
  timing(
  for(int j = 0; j< 50; j++)
  for(i = 0; i< SAMPLES / xp->xprot_left_fixed->frame_length; i++)
  a_xprot_func(xp->xprot_left_variable,
            xp->xprot_left_fixed, dst + xp->xprot_left_fixed->frame_length * i, 1, 1);)

  memcpy(dst, dstn, SAMPLES * 2);
  printf("Timing a_xprot_func_s() over %d frames %d bytes each\n",
         (SAMPLES) / xp->xprot_left_fixed->frame_length,
         xp->xprot_left_fixed->frame_length);
  printf("stock\n");
  for(int k = 0; k < 3; k++)
  timing(
  for(int j = 0; j< 50; j++)
  for(i = 0; i < (SAMPLES) / xp->xprot_left_fixed->frame_length; i++)
  a_xp_func_s(xpn->xprot_left_variable,
              xpn->xprot_left_fixed,
              xpn->xprot_right_variable,
              xpn->xprot_right_fixed,
              dst + xpn->xprot_left_fixed->frame_length * i,
              dst + xpn->xprot_left_fixed->frame_length * i + SAMPLES,
              1, 1));
  memcpy(dst, dstn, SAMPLES * 2);
  printf("foss\n");
  for(int k = 0; k < 3; k++)
  timing(
  for(int j = 0; j< 50; j++)
  for(i = 0; i < (SAMPLES) / xp->xprot_left_fixed->frame_length; i++)
  a_xprot_func_s(xp->xprot_left_variable,
                 xp->xprot_left_fixed,
                 xp->xprot_right_variable,
                 xp->xprot_right_fixed,
                 dst + xp->xprot_left_fixed->frame_length * i,
                 dst + xp->xprot_left_fixed->frame_length * i + SAMPLES,
                 1, 1));

  printf("Generating processed file\n");

  fpout = fopen("/opt/xprot_test/bin/xprotout.raw", "w+");

  assert(fpout);
  for(i = 0; i < SAMPLES / xp->xprot_left_fixed->frame_length; i++)
  a_xprot_func_s(xp->xprot_left_variable,
                 xp->xprot_left_fixed,
                 xp->xprot_right_variable,
                 xp->xprot_right_fixed,
                 src + xp->xprot_left_fixed->frame_length * i,
                 src + xp->xprot_left_fixed->frame_length * i + SAMPLES,
                 1, 1);

  for (i = 0; i < SAMPLES; i ++)
  {
    fwrite(&src[i], 2, 1, fpout);
    fwrite(&src[i + SAMPLES], 2, 1, fpout);
  }

  return 0;
}
