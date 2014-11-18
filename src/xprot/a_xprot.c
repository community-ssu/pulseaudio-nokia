#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#include "a_xprot_neon.h"
#endif

#include <pulsecore/log.h>
#include "xprot.h"
#include "dsp.h"

void
a_xprot_dpvl_mono(const int16 *in, XPROT_Variable *var, int lenght)
{
  int32x2_t DPVL_IIR_d0, DPVL_IIR_d1;
  int32x2_t DPVL_IIR_w03, DPVL_IIR_w24, DPVL_IIR_w1;
  int32x2_t d_sum;
  int32x2_t Z;
  int i;

  lenght >>= 2;

  DPVL_IIR_d0 = vdup_n_s32(var->DPVL_IIR_d[0]);
  DPVL_IIR_d1 = vdup_n_s32(var->DPVL_IIR_d[1]);

  DPVL_IIR_w03 = vset_lane_s32(var->DPVL_IIR_w[0] << 16, DPVL_IIR_w03, 0);
  DPVL_IIR_w03 = vset_lane_s32(var->DPVL_IIR_w[3] << 16, DPVL_IIR_w03, 1);

  DPVL_IIR_w24 = vset_lane_s32(var->DPVL_IIR_w[2] << 16, DPVL_IIR_w24, 1);
  DPVL_IIR_w24 = vset_lane_s32(var->DPVL_IIR_w[4] << 16, DPVL_IIR_w24, 0);

  DPVL_IIR_w1 = vset_lane_s32(var->DPVL_IIR_w[1] << 16, DPVL_IIR_w1, 0);
  DPVL_IIR_w1 = vset_lane_s32(0, DPVL_IIR_w1, 1);

  d_sum = vset_lane_s32(var->u_d_sum, d_sum, 0);
  d_sum = vset_lane_s32(var->x_d_sum, d_sum, 1);

  Z = vdup_n_s32(0);

  for (i = 0; i < lenght; i ++)
  {
    int32x2_t tmp_32x2_1,tmp_32x2_2,tmp_32x2_3;
    int32x4_t IN;
    int32x2x2_t tmp_32x2x2;

    IN = vmovl_s16(vld1_s16(in));

    in += 4;

    tmp_32x2x2 = vtrn_s32(vqdmulh_s32(DPVL_IIR_d0, DPVL_IIR_w03), Z);
    tmp_32x2_2 = vqsub_s32(vget_low_s32(IN),
                   vqadd_s32(vqshl_n_s32(tmp_32x2x2.val[0], 1),
                             vqdmulh_s32(DPVL_IIR_d1, DPVL_IIR_w1)));
    DPVL_IIR_d1 = vdup_lane_s32(tmp_32x2_2, 0);
    tmp_32x2_1 = vqshl_n_s32(vqadd_s32(vqdmulh_s32(DPVL_IIR_w24, DPVL_IIR_d1),
                               tmp_32x2x2.val[1]), 16);
    tmp_32x2_3 = vqadd_s32(vqdmulh_s32(tmp_32x2_1, tmp_32x2_1), d_sum);
    tmp_32x2x2 = vtrn_s32(vqdmulh_s32(DPVL_IIR_d1, DPVL_IIR_w03), Z);
    DPVL_IIR_d0 =
        vdup_lane_s32(vqsub_s32(vdup_lane_s32(tmp_32x2_2, 1),
                                vqadd_s32(vqshl_n_s32(tmp_32x2x2.val[0], 1),
                                          vqdmulh_s32(DPVL_IIR_d0,
                                                      DPVL_IIR_w1))) ,0);
    d_sum = vqshl_n_s32(vqadd_s32(vqdmulh_s32(DPVL_IIR_w24, DPVL_IIR_d0),
                                  tmp_32x2x2.val[1]), 16);
    tmp_32x2_2 = vqadd_s32(vqdmulh_s32(d_sum, d_sum), tmp_32x2_3);
    tmp_32x2x2 = vtrn_s32(vqdmulh_s32(DPVL_IIR_d0, DPVL_IIR_w03), Z);
    tmp_32x2_1 =
        vqsub_s32(vget_high_s32(IN),
                  vqadd_s32(vqshl_n_s32(tmp_32x2x2.val[0], 1),
                            vqdmulh_s32(DPVL_IIR_d1,
                                        DPVL_IIR_w1)));
    DPVL_IIR_d1 = vdup_lane_s32(tmp_32x2_1, 0);
    d_sum = vqshl_n_s32(vqadd_s32(vqdmulh_s32(DPVL_IIR_w24, DPVL_IIR_d1),
                                  tmp_32x2x2.val[1]), 16);

    tmp_32x2x2 = vtrn_s32(vqdmulh_s32(DPVL_IIR_d1, DPVL_IIR_w03), Z);
    DPVL_IIR_d0 = vqsub_s32(vdup_lane_s32(tmp_32x2_1, 1),
                            vqadd_s32(vqshl_n_s32(tmp_32x2x2.val[0], 1),
                                      vqdmulh_s32(DPVL_IIR_d0, DPVL_IIR_w1)));
    tmp_32x2_2 = vqadd_s32(vqdmulh_s32(d_sum, d_sum), tmp_32x2_2);
    DPVL_IIR_d0 = vdup_lane_s32(DPVL_IIR_d0, 0);
    d_sum = vqshl_n_s32(vqadd_s32(vqdmulh_s32(DPVL_IIR_w24, DPVL_IIR_d0),
                                  tmp_32x2x2.val[1]), 16);
    d_sum = vqadd_s32(vqdmulh_s32(d_sum, d_sum), tmp_32x2_2);
  }

  var->DPVL_IIR_d[0] = vget_lane_s32(DPVL_IIR_d0, 0);
  var->DPVL_IIR_d[1] = vget_lane_s32(DPVL_IIR_d1, 0);
  var->u_d_sum = vget_lane_s32(d_sum, 0);
  var->x_d_sum = vget_lane_s32(d_sum, 1);
}

int32_t
fexp(int32_t a, int32_t b)
{
#if 0
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
#endif

  int shift;
  int32_t tmp1, tmp2;

  tmp1 = __qadd16(__qsub16(0, a), b << 6);
  tmp2 = tmp1 & 0x3F;
  shift = tmp1 >> 6;

  tmp1 = __qsub16(0, tmp2 ? __ssat_16(tmp2 << 9) : 0);
  tmp2 = __smulbb(__qadd16(__sbfx_16(__smulbb(5614, tmp1)), 21868), tmp1);
  tmp1 = __smulbb(32767 >> shift, __qadd16(__qadd(tmp2, tmp2) >> 16, 32700));

  return __qadd(tmp1, tmp1) >> 16;
}

int32_t
dB100toLin(int32_t a, int16_t b)
{
  return fexp(__qdadd(32768, __smulbb(a, 3483)) >> 16, b);
}

void
a_xprot_temp_limiter(XPROT_Variable *var, XPROT_Fixed *fix, int16 *in)
{
  int i;
  int32_t t_gain_dB_l = var->t_gain_dB_l;
  int16x4_t gain;

  if (fix->t_lm >= var->T_coil_est)
  {
    int32_t k;

    if (__qsub16(fix->t_lm, 25) <= var->T_coil_est)
      k = 32718;
    else
      k = 32738;

    t_gain_dB_l = L_mult32_16(t_gain_dB_l, k);
  }
  else
  {
    int32_t k;
    int16_t diff = __qsub16(var->T_coil_est_old, var->T_coil_est);

    if (diff < 0)
    {
      if (__qadd16(fix->t_lm, 25) < var->T_coil_est)
        k = 6830;
      else
        k = 2830;
    }
    else
      k = 2277;

    t_gain_dB_l = __qsub(t_gain_dB_l, 2 * __smulbb(diff, k));
  }

  var->t_gain_dB_l = t_gain_dB_l;
  t_gain_dB_l = -t_gain_dB_l;
  t_gain_dB_l = L_mult32_16(100, t_gain_dB_l);

  t_gain_dB_l = dB100toLin(__ssat_16(t_gain_dB_l), 0);

  var->T_coil_est_old = var->T_coil_est;
  i = fix->frame_length >> 3;
  gain = vdup_n_s16(t_gain_dB_l);

  while ( i > 0 )
  {
    vst1_s16(in, vqrdmulh_s16(vld1_s16(in), gain));
    vst1_s16(in + 4, vqrdmulh_s16(vld1_s16(in + 4), gain));

    in += 8;
    i --;
  }
}

int16_t
a_xprot_dp_filter(int16 *in, int16 *w, int16 *d, int volume, int length)
{
  int16x4_t W, W12, D, tmp_16x4, RV, VOL;
  int32x4_t Z, TMP1, TMP2;
  int i = length >> 2;

  W = vdup_n_s16(w[0]);
  W12 = vdup_n_s16(0);
  RV = W12;
  Z = vdupq_n_s32(0);
  TMP1 = Z;
  D = vdup_n_s16(d[0]);
  TMP1 = vsetq_lane_s32(__sat_mul_add_16(w[1], d[1]), TMP1, 0);
  W12 = vset_lane_s16(w[2], W12, 0);
  W12 = vset_lane_s16(w[1], W12, 1);
  VOL = vdup_n_s16(volume);

  while (i > 0)
  {
    TMP1 = vqdmlal_s16(
          vqdmlal_s16(TMP1, vqrdmulh_s16(vld1_s16(in), VOL), W),
          D, W12);

    in += 4;
    i --;

    TMP2 = vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(TMP1),
                                       vreinterpretq_s8_s32(Z),
                                       4));
    TMP1 = vqdmlal_s16(TMP1, D, W12);
    tmp_16x4 = vqrshrn_n_s32(TMP1, 16);
    D = vdup_lane_s16(tmp_16x4, 0);
    TMP2 = vqdmlal_s16(TMP2, D, W12);
    TMP1 = vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(TMP2),
                                       vreinterpretq_s8_s32(Z),
                                       4));
    TMP2 = vqdmlal_s16(TMP2, D, W12);
    D = vqrshrn_n_s32(TMP2, 16);
    D = vdup_lane_s16(D, 0);
    TMP1 = vqdmlal_s16(TMP1, D, W12);
    tmp_16x4 = vreinterpret_s16_s8(vext_s8(vreinterpret_s8_s16(D),
                                     vreinterpret_s8_s16(tmp_16x4),
                                     2));
    TMP2 = vreinterpretq_s32_s8(
          vextq_s8(vreinterpretq_s8_s32(TMP1), vreinterpretq_s8_s32(Z), 4));
    TMP1 = vqdmlal_s16(TMP1, D, W12);
    TMP1 = vcombine_s32(vreinterpret_s32_s16(vqrshrn_n_s32(TMP1, 16)),
                      vget_high_s32(TMP1));
    D = vdup_lane_s16(vreinterpret_s16_s32(vget_low_s32(TMP1)), 0);
    TMP2 = vqdmlal_s16(TMP2, D, W12);
    tmp_16x4 = vreinterpret_s16_s8(vext_s8(vreinterpret_s8_s16(tmp_16x4),
                                     vreinterpret_s8_s16(D),
                                     4));
    TMP1 = vreinterpretq_s32_s8(
          vextq_s8(vreinterpretq_s8_s32(TMP2), vreinterpretq_s8_s32(Z), 4));

    TMP2 = vqdmlal_s16(TMP2, D, W12);
    D = vdup_lane_s16(vqrshrn_n_s32(TMP2, 16), 0);
    tmp_16x4 = vreinterpret_s16_s8(vext_s8(vreinterpret_s8_s16(D),
                                     vreinterpret_s8_s16(tmp_16x4),
                                     6));
    RV = vmax_s16(vqabs_s16(tmp_16x4), RV);
  }

  RV = vpmax_s16(vpmax_s16(RV, vreinterpret_s16_s32(vget_low_s32(Z))),
                 vreinterpret_s16_s32(vget_low_s32(Z)));

  d[1] = vget_lane_s16(tmp_16x4, 3);
  d[0] = vget_lane_s16(D, 3);

  return vget_lane_s16(RV, 0);
}

void
a_xprot_coeff_calc(XPROT_Variable *var, XPROT_Fixed *fix)
{
  int32_t x_peak = var->x_peak;

  if (var->x_peak > fix->x_lm)
  {
    int32_t tmp, coef_raw1, k1, k2, k3;

    k1 = __sat_mul_add_16(x_peak, x_peak) >> 16;
    k2 = __sat_mul_add_16(k1, x_peak) >> 16;
    k3 = __sat_mul_add_16(k2, x_peak) >> 16;

    tmp = __sat_mul_dadd_32(x_peak, fix->pa1n_asnd[1], fix->pa1n_asnd[0] << 16);
    tmp = __sat_mul_dadd_32(k1, fix->pa1n_asnd[2], tmp);
    tmp = __sat_mul_dadd_32(k2, fix->pa1n_asnd[3], tmp);
    tmp = __qadd(__sat_mul_dadd_32(k3, fix->pa1n_asnd[4], tmp), 32768) >> 16;;

    coef_raw1 = L_mult32_16(fix->s_pa1n, tmp);
    var->coef_raw[1] = __ssat_16(coef_raw1);

    tmp = __sat_mul_dadd_32(x_peak, fix->pa2n_asnd[1], fix->pa2n_asnd[0] << 16);
    tmp = __sat_mul_dadd_32(k1, fix->pa2n_asnd[2], tmp);
    tmp = __sat_mul_dadd_32(k2, fix->pa2n_asnd[3], tmp);
    tmp = __qadd(__sat_mul_dadd_32(k3, fix->pa2n_asnd[4], tmp), 32768) >> 16;

    tmp = L_mult32_16(fix->s_pa2n, tmp);
    var->coef_raw[2] = __ssat_16(tmp);

    tmp = __qadd(__qadd(32768, -__normalize(coef_raw1, 0xc0000000)), tmp);
    var->coef_raw[0] = __ssat_16(L_mult32_16(tmp, fix->b_d));
    var->prod_raw_rav[0] = __sat_mul_add_16(var->coef_raw[0], fix->t_rav[1]);
    var->prod_raw_rav[1] = __sat_mul_add_16(var->coef_raw[1], fix->t_rav[1]);
    var->prod_raw_rav[2] = __sat_mul_add_16(var->coef_raw[2], fix->t_rav[1]);
  }
}

static int32_t
calc_ntlm_part(int32_t a, int32_t b)
{
  int32_t rv = 32767;
  int32 x, y;

  if (b < a && b > 0)
  {
    x = __normalize(b, a);

    if (!x)
      return rv;

    x >>= 16;
    y = __normalize(a, a) >> 16;

    assert(y > 0);
    assert(x >= 0);
    assert(x <= y);

    rv = (x << 15) / y;
    rv = rv >= 32768 ? 32767 : (int16_t)rv;
  }

  return rv;
}

void
a_xprot_temp_predictor(XPROT_Variable *var, XPROT_Fixed *fix, int16 *in)
{

  int32_t ntlm = 0, out;
  int16x4_t frame_average_16x4;
  int32x4_t out_32x4 = vdupq_n_s32(0);
  int i = fix->frame_length >> 2;

  frame_average_16x4= vdup_n_s16(fix->frame_average);

  while (i > 0)
  {
    int16x4_t in_16x4 = vld1_s16(in);
    in += 4;
    out_32x4 = vqdmlal_s16(out_32x4, vqdmulh_s16(in_16x4, frame_average_16x4),
                           in_16x4);
    i --;
  }

  out = L_mult32_16(__qadd(__qadd(__qadd(vgetq_lane_s32(out_32x4, 0),
                                         vgetq_lane_s32(out_32x4, 1)),
                                  vgetq_lane_s32(out_32x4, 2)),
                           vgetq_lane_s32(out_32x4, 3)),
                    var->lin_vol);

  if (fix->compute_nltm)
  {
    ntlm =
        __qadd(__normalize(calc_ntlm_part(var->x_d_sum, var->u_d_sum >> 2),
                           0xE0000000),
               fix->alfa);
    ntlm =
        L_mult32_16(L_mult32_16(var->x_d_sum, calc_ntlm_part(ntlm, fix->beta)),
                    var->T_coil_est);
  }

  out = __qsub(out, ntlm);
  var->T_v_l_old =
      __qsub(L_mult32_16(out, fix->b_tv),
             L_mult32_16(var->T_v_l_old, fix->a_tv));
  var->T_m_l_old =
      __qsub(L_mult32_16(out, fix->b_tm),
             L_mult32_16(var->T_m_l_old, fix->a_tm));
  var->T_coil_est =
      __qadd(__qadd(var->T_m_l_old, var->T_v_l_old), 0x8000) >> 16;
  var->T_coil_est =
      __qadd16(var->T_coil_est,
               __qadd(
                 __sat_mul_add_16(var->t_amb ? __ssat_16(var->t_amb << 8) : 0,
                                  fix->sigma_T_amb), 0x8000) >> 16);
  var->T_coil_est =
      __qadd(__sat_mul_add_16(var->T_coil_est, fix->t_mp), 0x8000) >> 16;
  var->u_d_sum = 0;
  var->x_d_sum = 0;
}

void
a_xprot_lfsn_stereo(int16_t *in_left, int16_t *in_right,
                    XPROT_Variable *var_left, XPROT_Variable *var_right,
                    int16_t t, int length)
{
  const int16x4_t T =
  {
    t, 0, 0, 0
  };
  const int32x2_t t16 =
  {
    t << 16
  };
  const int32x2_t k = vdup_n_s32(0xFFFF0000);
  const int32x4_t rav0 = vdupq_n_s32(__qadd(var_left->prod_raw_rav[0], 0x8000u));
  int32x2_t  rav12 =
  {
    __qadd(var_left->prod_raw_rav[1], 0x8000),
    __qadd(var_left->prod_raw_rav[2], 0x8000)
  };
  int16x4_t w0 = vdup_n_s16(var_left->LFSN_IIR_w[0]);
  int32x2_t  w12 =
  {
    var_left->LFSN_IIR_w[1] << 16,
    var_left->LFSN_IIR_w[2] << 16
  };
  int32x2_t w3 =
  {
    var_left->LFSN_IIR_w[3] << 16,
    var_right->LFSN_IIR_w[3] << 16
  };
  int32x2_t w4 =
  {
    var_left->LFSN_IIR_w[4] << 16,
    var_right->LFSN_IIR_w[4] << 16
  };
  int32x2_t d0 =
  {
    var_left->LFSN_IIR_d[0],
    var_right->LFSN_IIR_d[0]
  };
  int32x2_t d1 =
  {
    var_left->LFSN_IIR_d[1],
    var_right->LFSN_IIR_d[1]
  };
  int32x2_t tmp;
  int32x4_t outr, rav01, outl;
  int i;

  length >>= 2;

  for (i = 0; i < length; i++)
  {
    w12 = vand_s32(vqadd_s32(vqdmulh_lane_s32(w12, t16, 0), rav12), k);
    w0 = mul32_16_s32x2(w0, T, rav0);
    tmp = vext8_s16_2_s32x2(vreinterpret_s32_s16(T), w0);
    SET_LOW(outl, vqdmulh_lane_s32(d1, w12, 1));
    d1 = vqdmulh_s32(d1, w4);
    w0 = mul32_16_s32x2(w0, T, rav0);
    tmp = vext8_s16_2_s32x2(tmp, w0);
    w0 = mul32_16_s32x2(w0, T, rav0);
    tmp = vext8_s16_2_s32x2(tmp, w0);
    w0 = mul32_16_s32x2(w0, T, rav0);
    tmp = vext8_s16_2_s32x2(tmp, w0);
    outr = vshrq_n_s32(vqdmull_s16(vld1_s16(in_left),
                                 vreinterpret_s16_s32(tmp)), 16);
    rav01 = vshrq_n_s32(vqdmull_s16(vld1_s16(in_right),
                                  vreinterpret_s16_s32(tmp)), 16);
    tmp = vqadd_s32(vqshl_n_s32(vqdmulh_lane_s32(d0, w12, 0), 1), LOW(outl));
    _vtrnq_s32(outr, rav01);
    SET_LOW(outl, vqdmulh_s32(d0, w3));
    w12 = vand_s32(vqadd_s32(vqdmulh_lane_s32(w12, t16, 0), rav12), k);
    SET_LOW(outl, vqadd_s32(vqshl_n_s32(LOW(outl), 1), d1));
    d1 = vqsub_s32(LOW(outr), tmp);
    SET_LOW(outr, vqadd_s32(vqshl_n_s32(vqdmulh_lane_s32(d1, w12, 0), 1),
                          vqdmulh_lane_s32(d0, w12, 1)));

    w12 = vand_s32(vqadd_s32(vqdmulh_lane_s32(w12, t16, 0), rav12), k);
    SET_LOW(outl, vqadd_s32(d1, LOW(outl)));
    tmp = vqadd_s32(vqshl_n_s32(vqdmulh_s32(d1, w3), 1), vqdmulh_s32(d0, w4));
    d0 = vqsub_s32(LOW(rav01), LOW(outr));
    SET_LOW(rav01, vqdmulh_lane_s32(d1, w12, 1));
    SET_LOW(outr, vqadd_s32(d0, tmp));
    tmp = vqdmulh_lane_s32(d0, w12, 0);
    d1  = vqdmulh_s32(d1, w4);
    tmp = vqadd_s32(vqshl_n_s32(tmp, 1), LOW(rav01));
    w12 = vand_s32(vqadd_s32(vqdmulh_lane_s32(w12, t16, 0), rav12), k);
    SET_LOW(rav01, vqadd_s32(vqshl_n_s32(vqdmulh_s32(d0, w3), 1), d1));
    d1 = vqsub_s32(HIGH(outr), tmp);
    SET_HIGH(outl, vqadd_s32(d1, LOW(rav01)));
    SET_HIGH(outr, vqadd_s32(vqshl_n_s32(vqdmulh_lane_s32(d1, w12, 0), 1),
                           vqdmulh_lane_s32(d0, w12, 1)));
    tmp = vqadd_s32(vqshl_n_s32(vqdmulh_s32(d1, w3), 1), vqdmulh_s32(d0, w4));
    d0 = vqsub_s32(HIGH(rav01), HIGH(outr));
    SET_HIGH(outr, vqadd_s32(d0, tmp));
    _vtrnq_s32(outl, outr);

    vst1_s16(in_left, vqmovn_s32(outl));
    in_left += 4;

    vst1_s16(in_right, vqmovn_s32(outr));
    in_right += 4;
  }

  var_left->LFSN_IIR_d[0] = vget_lane_s32(d0, 0);
  var_left->LFSN_IIR_d[1] = vget_lane_s32(d1, 0);
  var_right->LFSN_IIR_d[0] = vget_lane_s32(d0, 1);
  var_right->LFSN_IIR_d[1] = vget_lane_s32(d1, 1);
  var_left->LFSN_IIR_w[0] = vget_lane_s16(w0, 0);
  var_left->LFSN_IIR_w[1] = vget_lane_s32(w12, 0) >> 16;
  var_left->LFSN_IIR_w[2] = vget_lane_s32(w12, 1) >> 16;

}

void
a_xprot_lfsn_mono(int16_t *in, XPROT_Variable *var, int16_t t, int length)
{
  int i;
  int32x2_t w12, w34, rav12, k, d0, d1;
  int32x4_t rav0;
  int32x2_t tmp, zero;
  int32_t t16;
  int16x4_t w0, T;
  int32x4_t OUT;

  T = vdup_n_s16(0);
  T = vset_lane_s16(t, T, 0);

  t16 = t << 16;

  zero = vdup_n_s32(0);
  k = vdup_n_s32(0xFFFF0000);

  w0 = vdup_n_s16(var->LFSN_IIR_w[0]);
  w12 = vset_lane_s32(var->LFSN_IIR_w[1] << 16, w12, 0);
  w12 = vset_lane_s32(var->LFSN_IIR_w[2] << 16, w12, 1);
  w34 = vset_lane_s32(var->LFSN_IIR_w[3] << 16, w34, 0);
  w34 = vset_lane_s32(var->LFSN_IIR_w[4] << 16, w34, 1);

  rav0 = vdupq_n_s32(__qadd(var->prod_raw_rav[0], 0x8000));
  rav12 = vset_lane_s32(__qadd(var->prod_raw_rav[1], 0x8000), rav12, 0);
  rav12 = vset_lane_s32(__qadd(var->prod_raw_rav[2], 0x8000), rav12, 1);

  d0 = vdup_n_s32(var->LFSN_IIR_d[0]);
  d1 = vdup_n_s32(var->LFSN_IIR_d[1]);

  length >>= 2;

  for (i = 0; i < length; i++)
  {
    int32x2x2_t tmp1_32x2x2, tmp2_32x2x2;

    SET_LOW(OUT, mul32_16_s16x4(vreinterpret_s32_s16(w0), T, rav0));
    w0 = vext8_s16_2(T, LOW(OUT));
    SET_LOW(OUT, mul32_16_s16x4(LOW(OUT), T, rav0));
    w0 = vext8_s16_2(w0, LOW(OUT));
    SET_LOW(OUT, mul32_16_s16x4(LOW(OUT), T, rav0));
    tmp = vreinterpret_s32_s16(vext8_s16_2(w0, LOW(OUT)));
    w0 = vreinterpret_s16_s32(mul32_16_s16x4(LOW(OUT), T, rav0));
    OUT =
        vshrq_n_s32(
          vqdmull_s16(
            vld1_s16(in), vext8_s16_2(vreinterpret_s16_s32(tmp),
                                      vreinterpret_s32_s16(w0))), 16);
    tmp1_32x2x2 = a_xprot_lfsn_vtrn(w12, t16, tmp, rav12, k, w34);
    tmp = vqdmulh_s32(d0, tmp1_32x2x2.val[0]);
    d1 = vqdmulh_s32(d1, tmp1_32x2x2.val[1]);
    tmp2_32x2x2 = vtrn_s32(vqadd_s32(vqshl_n_s32(tmp, 1), d1), zero);
    SET_LOW(OUT, vqsub_s32(LOW(OUT), tmp2_32x2x2.val[0]));
    d1 = vdup_n_s32(vget_lane_s32(LOW(OUT), 0));
    tmp = vqadd_s32(LOW(OUT), tmp2_32x2x2.val[1]);
    tmp1_32x2x2 = vtrn_s32(tmp1_32x2x2.val[0], tmp1_32x2x2.val[1]);
    tmp2_32x2x2 = a_xprot_lfsn_vtrn(tmp1_32x2x2.val[0], t16, tmp2_32x2x2.val[1],
                                    rav12, k, tmp1_32x2x2.val[1]);
    w12 = vqdmulh_s32(d1, tmp2_32x2x2.val[0]);
    tmp1_32x2x2 = vtrn_s32(zero, vqadd_s32(
                             vqshl_n_s32(w12, 1),
                             vqdmulh_s32(d0, tmp2_32x2x2.val[1])));
    tmp2_32x2x2 = vtrn_s32(tmp2_32x2x2.val[0], tmp2_32x2x2.val[1]);
    SET_LOW(OUT, vqsub_s32(tmp, tmp1_32x2x2.val[0]));
    d0 = vdup_n_s32(vget_lane_s32(LOW(OUT), 1));
    SET_LOW(OUT, vqadd_s32(LOW(OUT), tmp1_32x2x2.val[1]));
    tmp2_32x2x2 = a_xprot_lfsn_vtrn(tmp2_32x2x2.val[0], t16, tmp, rav12, k,
                                    tmp2_32x2x2.val[1]);
    d1 = vqdmulh_s32(d1, tmp2_32x2x2.val[1]);
    tmp1_32x2x2 =
        vtrn_s32(vqadd_s32(vqshl_n_s32(vqdmulh_s32(d0, tmp2_32x2x2.val[0]), 1),
                           d1), zero);
    tmp2_32x2x2 = vtrn_s32(tmp2_32x2x2.val[0], tmp2_32x2x2.val[1]);
    SET_HIGH(OUT, vqsub_s32(HIGH(OUT), tmp1_32x2x2.val[0]));
    d1 = vdup_n_s32(vget_lane_s32(HIGH(OUT), 0));
    SET_HIGH(OUT, vqadd_s32(HIGH(OUT), tmp1_32x2x2.val[1]));
    tmp1_32x2x2 = a_xprot_lfsn_vtrn(tmp2_32x2x2.val[0], t16, w12, rav12, k,
                                    tmp2_32x2x2.val[1]);
    tmp2_32x2x2 = vtrn_s32(tmp1_32x2x2.val[0], tmp1_32x2x2.val[1]);
    w12 = tmp2_32x2x2.val[0];
    w34 = tmp2_32x2x2.val[1];
    tmp2_32x2x2 =
        vtrn_s32(zero, vqadd_s32(vqshl_n_s32(
                                   vqdmulh_s32(d1, tmp1_32x2x2.val[0]), 1),
                                 vqdmulh_s32(d0, tmp1_32x2x2.val[1])));
    SET_HIGH(OUT, vqsub_s32(HIGH(OUT), tmp2_32x2x2.val[0]));
    d0 = vdup_n_s32(vget_lane_s32(HIGH(OUT), 1));
    SET_HIGH(OUT, vqadd_s32(HIGH(OUT), tmp2_32x2x2.val[1]));
    vst1_s16(in, vqmovn_s32(OUT));

    in += 4;
  }

  var->LFSN_IIR_d[0] = vget_lane_s32(d0, 0);
  var->LFSN_IIR_d[1] = vget_lane_s32(d1, 0);
  var->LFSN_IIR_w[0] =  vget_lane_s16(w0, 0);
  var->LFSN_IIR_w[1] =  vget_lane_s32(w12, 0) >> 16;
  var->LFSN_IIR_w[2] = vget_lane_s32(w12, 1) >> 16;
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
    int16 x_peak = a_xprot_dp_filter(in, var->DP_IIR_w, var->DP_IIR_d,
                                     var->lin_vol, fix->frame_length);

    if (var->x_peak < x_peak)
      var->x_peak = x_peak;

    var->x_peak = __sat_mul_add_16(fix->t_rav[0], var->x_peak) >> 16;
    a_xprot_coeff_calc(var, fix);
    a_xprot_lfsn_mono(in, var, fix->t_rav[2], fix->frame_length);

    if (fix->compute_nltm == 1)
    {
      a_xprot_dpvl_mono(in, var, fix->frame_length);
      var->u_d_sum = L_mult32_16(var->u_d_sum, fix->frame_average);
      var->x_d_sum = L_mult32_16(var->x_d_sum, fix->frame_average);
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
  if (temp_limit == 1)
  {
    if (var_left->T_coil_est <= var_right->T_coil_est)
      var_left->T_coil_est = var_right->T_coil_est;
    else
      var_right->T_coil_est = var_left->T_coil_est;

    a_xprot_temp_limiter(var_left, fix_left, in_left);
    a_xprot_temp_limiter(var_right, fix_right, in_right);
  }

  if ( displ_limit == 1 )
  {
    int16_t x_peak;
    int16_t x_peak_left = a_xprot_dp_filter(in_left,
                                            var_left->DP_IIR_w,
                                            var_left->DP_IIR_d,
                                            var_left->lin_vol,
                                            fix_left->frame_length);
    int16_t x_peak_right = a_xprot_dp_filter(in_right,
                                             var_right->DP_IIR_w,
                                             var_right->DP_IIR_d,
                                             var_left->lin_vol,
                                             fix_left->frame_length);

    if (x_peak_left < x_peak_right)
      x_peak_left = x_peak_right;

    x_peak =  (var_left->x_peak > var_right->x_peak) ? var_left->x_peak :
                                                       var_right->x_peak;

    if ( x_peak < x_peak_left )
      x_peak = x_peak_left;

    var_left->x_peak =
        __sat_mul_add_16(fix_left->t_rav[0], var_left->x_peak) >> 16;
    var_right->x_peak =
        __sat_mul_add_16(fix_right->t_rav[0], var_right->x_peak) >> 16;

    a_xprot_coeff_calc(var_left, fix_left);
    a_xprot_lfsn_stereo(in_left, in_right, var_left, var_right,
                        fix_left->t_rav[2], fix_left->frame_length);

    if (fix_left->compute_nltm == 1)
    {
      a_xprot_dpvl_mono(in_left, var_left, fix_left->frame_length);

      var_left->u_d_sum =
          L_mult32_16(var_left->u_d_sum, fix_left->frame_average);
      var_left->x_d_sum =
          L_mult32_16(var_left->x_d_sum, fix_left->frame_average);
    }

    if (fix_right->compute_nltm == 1)
    {
      a_xprot_dpvl_mono(in_right, var_left, fix_left->frame_length);

      var_right->u_d_sum =
          L_mult32_16(var_right->u_d_sum, fix_right->frame_average);
      var_right->x_d_sum =
          L_mult32_16(var_right->x_d_sum, fix_right->frame_average);
    }
  }

  if ( temp_limit == 1 )
  {
    a_xprot_temp_predictor(var_left, fix_left, in_left);
    a_xprot_temp_predictor(var_right, fix_right, in_right);
  }
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

  fix->s_pa1n = __normalize(cns->s_pa1n, 0xc0000000);
  fix->s_pa2n = __normalize(cns->s_pa2n, 0xe0000000);
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

  if (__normalize(fix->beta, fix->alfa) >> 16)
    fix->compute_nltm = 1;
}
