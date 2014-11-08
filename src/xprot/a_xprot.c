#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>
#include "xprot.h"
#include "dsp.h"


static int
fexp(int32_t a, int32_t b) __attribute__ ( ( naked ) );

static int
fexp(int32_t a, int32_t b)
{
  int32_t tmp1, tmp2;

  __asm__ __volatile (
        "MOV             %2, #0\n"
        "MOV             %1, %1,LSL#6\n"
        "QSUB16          %0, %2, %1\n"
        "SXTH            %1, %1\n"
        "SXTH            %0, %0\n"
        "QADD16          %0, %0, %1\n"
        "SXTH            %1, %0\n"
        "ANDS            %0, %1, #0x3F\n"
        "MOVEQ           %3, %2\n"

        /*
          Stock code was like that :

        "BEQ             l1%=\n"
        "SSAT            %3, #0x10, %0,LSL#9\n"
        "l1%=:\n"

          I guess we can save an instruction and a branch by using SSATNE
        */

        "SSATNE          %3, #0x10, %0,LSL#9\n"
        "QSUB16          %0, %2, %3\n"
        "MOV             %1, %1,ASR#6\n"
        "MOVW            %3, #0x7FFF\n"
        "MOVW            %2, #0x556C\n"
        "MOV             %3, %3,LSR %1\n"
        "MOVW            %1, #0x15EE\n"
        "SMULBB          %1, %0, %1\n"
        "SBFX            %1, %1, #0xF, #0x10\n"
        "QADD16          %1, %1, %2\n"
        "SMULBB          %0, %1, %0\n"
        "MOVW            %1, #0x7FBC\n"
        "QADD            %0, %0, %0\n"
        "MOV             %0, %0,ASR#16\n"
        "QADD16          %0, %0, %1\n"
        "SMULBB          %0, %3, %0\n"
        "QADD            %0, %0, %0\n"
        "MOV             %0, %0,ASR#16\n"
        "BX              LR"
        : "=r"(a), "=r"(b), "=r"(tmp1), "=r"(tmp2));
  return a;
}

static int32_t
dB100toLin(int32_t a, int16_t b)
{
  return fexp(__qdadd(32768, __smulbb(a, 3483)) >> 16, b);
}

void
a_xprot_func(XPROT_Variable *var, XPROT_Fixed *fix, int16 *in,
             int16 temp_limit, int16 displ_limit)
{
  pa_assert(var);
  pa_assert(fix);
  pa_assert(in);

  if (temp_limit > 0)
    a_xprot_temp_limiter(var, fix, in);

  if (displ_limit == 1)
  {
    int32 tmp;
    int16 x_peak = a_xprot_dp_filter(in, var->DP_IIR_w, var->DP_IIR_d,
                                     var->lin_vol, fix->frame_length);

    if ( var->x_peak < x_peak )
      var->x_peak = x_peak;

    tmp = __smulbb(fix->t_rav[0], var->x_peak);
    var->x_peak = __qadd(tmp, tmp) >> 16;

    a_xprot_coeff_calc(var, fix);

    a_xprot_lfsn_mono(in, var, fix->t_rav[2], fix->frame_length);

    if (fix->compute_nltm == 1)
    {
      a_xprot_dpvl_mono(in, var, fix->frame_length);

      var->u_d_sum = __qdadd(var->u_d_sum * fix->frame_average >> 15,
                             __smulbt(var->u_d_sum, fix->frame_average));

      var->x_d_sum = __qdadd(var->x_d_sum * fix->frame_average >> 15,
                             __smulbt(var->x_d_sum, fix->frame_average));
    }
  }

  if (temp_limit > 0)
    a_xprot_temp_predictor(var, fix, in);
}

void
a_xprot_func_s(XPROT_Variable *var_left, XPROT_Fixed *fix_left,
               XPROT_Variable *var_right, XPROT_Fixed *fix_right,
               int16 *in_left, int16 *in_right,
               int16 temp_limit, int16 displ_limit)
{
  pa_assert(var_left);
  pa_assert(var_right);
  pa_assert(fix_left);
  pa_assert(fix_right);
  pa_assert(in_left);
  pa_assert(in_right);
}

void
a_xprot_init(XPROT_Variable *var, XPROT_Fixed *fix, XPROT_Constant *cns)
{
  int i;

  pa_assert(var);
  pa_assert(fix);
  pa_assert(cns);

  fix->x_lm = cns->x_lm;
  fix->t_mp = cns->t_mp;
  fix->sigma_T_amb = cns->sigma_T_amb;
  fix->t_lm = __qadd(__sat_mul_add_16(cns->t_lm, cns->t_mp), 32768u) >> 16;
  var->t_amb = 20;
  var->u_d_sum = 0;
  var->x_d_sum = 0;
  var->volume = 0;
  var->t_gain_dB_l = 0;
  var->T_coil_est = 0;
  var->T_coil_est_old = 0;
  var->T_v_l_old = 0;
  var->T_m_l_old = 0;
  var->DP_IIR_d[0] = 0;
  var->DP_IIR_d[1] = 0;
  var->LFSN_IIR_d[0] = 0;
  var->LFSN_IIR_d[1] = 0;
  var->DPVL_IIR_d[0] = 0;
  var->DPVL_IIR_d[1] = 0;
  var->x_peak = 0;

  fix->frame_length = cns->frame_length;
  fix->stereo_frame_length = 2 * cns->frame_length;
  fix->frame_average = 32768.0 / (float) cns->frame_length;

  var->LFSN_IIR_w[0] = cns->sigma_c_0;
  var->LFSN_IIR_w[1] = cns->a_1_r;
  var->LFSN_IIR_w[2] = cns->a_2_r;
  var->LFSN_IIR_w[3] = cns->b_1_c;
  var->LFSN_IIR_w[4] = cns->b_2_c;
  var->coef_raw[0] = cns->sigma_c_0;
  var->coef_raw[1] = cns->a_1_r;
  var->coef_raw[2] = cns->a_2_r;
  var->DP_IIR_w[0] = cns->sigma_dp;
  var->DP_IIR_w[1] = cns->a_2_t;
  var->DP_IIR_w[2] = cns->a_1_t;
  fix->t_rav[0] = cns->t_r;
  fix->t_rav[1] = cns->t_av1;
  fix->t_rav[2] = cns->t_av2;

  for (i = 0; i < 5; i++)
  {
    fix->pa1n_asnd[i] = cns->pa1n_asnd[i];
    fix->pa2n_asnd[i] = cns->pa2n_asnd[i];
  }

  fix->s_pa1n = __sat_unk_32(cns->s_pa1n, 0xc0000000);
  fix->s_pa2n = __sat_unk_32(cns->s_pa2n, 0xe0000000);
  fix->b_d = cns->b_d;
  fix->b_tv = cns->b_tv;
  fix->a_tv = cns->a_tv;
  fix->b_tm = cns->b_tm;
  fix->a_tm = cns->a_tm;
  fix->alfa = cns->alfa;
  fix->beta = cns->beta;
  var->DPVL_IIR_w[0] = cns->a_1_x_d;
  var->DPVL_IIR_w[1] = cns->a_2_x_d;
  var->DPVL_IIR_w[2] = cns->b_2_x_d;
  var->DPVL_IIR_w[3] = cns->b_2_u_d;
  var->DPVL_IIR_w[4] = cns->b_1_u_d;
  var->prod_raw_rav[0] = __sat_mul_add_16(var->coef_raw[0], fix->t_rav[1]);
  var->prod_raw_rav[1] = __sat_mul_add_16(var->coef_raw[1], fix->t_rav[1]);
  var->prod_raw_rav[2] = __sat_mul_add_16(var->coef_raw[2], fix->t_rav[1]);

  fix->compute_nltm = 0;

  if (__sat_unk_32(fix->beta, fix->alfa) >> 16)
    fix->compute_nltm = 1;
}
