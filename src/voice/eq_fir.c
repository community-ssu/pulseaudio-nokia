#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pulse/xmalloc.h>
#include <pulsecore/log.h>
#include <string.h>

#include "eq_fir.h"
struct IFIREQ_Status
{
  int32_t lastRnd;
  int32_t FEQ_D_threshold;
  int16_t fir_coeffs_left[64];
  int16_t fir_coeffs_right[64];
  uint16_t nb_coeffs_left;
  uint16_t nb_coeffs_right;
  uint16_t length_of_delay_line_left;
  uint16_t length_of_delay_line_right;
  int16_t UFGC_gain_left;
  int16_t UFGC_gain_right;
  int16_t FEQ_D_attack;
  int16_t FEQ_D_release;
  int16_t FEQ_D_hold_attack;
  int16_t integrator_coeffs_left[3];
  int16_t integrator_coeffs_right[3];
  int16_t use_fireq;
  int16_t use_dyn_resp;
};

void fir_eq_free(struct fir_eq *eq)
{
  free(eq->scratch);
  free(eq->conf->b_lf);
  free(eq->conf->a_lf);
  free(eq->conf->dp_r);
  free(eq->conf->dp_l);
  free(eq->conf->b_eq);
  free(eq->conf);
  free(eq);
}

void fir_eq_process_stereo(struct fir_eq *eq, int16_t *src_left, int16_t *src_right, int16_t *dst_left, int16_t *dst_right, int32_t samples)
{
  filter_samples2(src_left, src_right, dst_left, dst_right, samples, 1, eq->conf, eq->scratch);
}

void fir_eq_change_params(struct fir_eq *eq, const void *parameters, size_t length)
{
  fir_eq_conf *conf = eq->conf;
  struct IFIREQ_Status *params = (struct IFIREQ_Status *)parameters;
  conf->glin = params->UFGC_gain_left;
  int32_t nb_coeffs_left = params->nb_coeffs_left;
  conf->n_eq = nb_coeffs_left;
  conf->dither_old = params->lastRnd;
  int32_t fir_order_left = nb_coeffs_left & 0xFFFFFFF8;
  if (nb_coeffs_left & 7)
    fir_order_left = (nb_coeffs_left & 0xFFFFFFF8) + 8;
  int i;
  for (i = 0;i < conf->n_eq;i++)
  {
    conf->b_eq[i] = params->fir_coeffs_left[i];
  }
  for (;i < fir_order_left;i++)
  {
    conf->b_eq[i] = 0;
  }
  conf->n_eq = fir_order_left;
  conf->a_lf[0] = params->integrator_coeffs_left[0];
  conf->b_lf[0] = params->integrator_coeffs_left[1];
  conf->b_lf[1] = params->integrator_coeffs_left[2];
  conf->drop_gain_threshold = params->FEQ_D_threshold;
  conf->attack = params->FEQ_D_attack;
  conf->decay = params->FEQ_D_release;
  conf->hold_attack = params->FEQ_D_hold_attack;
  conf->c2 = 0x8000 - conf->attack;
  conf->gnom = conf->glin;
  conf->hold_gain = 9600;
  conf->coef = 0;
  conf->b_zero = 0;
  conf->attack_count = 0;
  conf->glin_count = 0;
  conf->ca = 0;
  conf->c1 = 0;
  conf->dither_old = 0;
  conf->l_d = 0;
  conf->r_d = 0;
  conf->anoteqd = 0;
  conf->seed = 1;
  conf->amaxeqd = 3650;
  eq->use_fireq = params->use_fireq;
  eq->use_dyn_resp = params->use_dyn_resp;
  pa_log_debug("FIR EQ params:");
  pa_log_debug("glin:                %d", eq->conf->glin);
  pa_log_debug("gnom:                %d", eq->conf->gnom);
  pa_log_debug("hold_attack:         %d", eq->conf->hold_attack);
  pa_log_debug("glin_count:          %d", eq->conf->glin_count);
  pa_log_debug("hold_gain:           %d", eq->conf->hold_gain);
  pa_log_debug("coef:                %d", eq->conf->coef);
  pa_log_debug("c1:                  %d", eq->conf->c1);
  pa_log_debug("c2:                  %d", eq->conf->c2);
  pa_log_debug("amaxeqd:             %d", eq->conf->amaxeqd);
  pa_log_debug("anoteqd:             %d", eq->conf->anoteqd);
  pa_log_debug("attack:              %d", eq->conf->attack);
  pa_log_debug("decay:               %d", eq->conf->decay);
  pa_log_debug("drop_gain_threshold: %d", eq->conf->drop_gain_threshold);
}

struct fir_eq *fir_eq_new(int sampling_frequency, int channels)
{
  struct fir_eq *eq = pa_xnew(struct fir_eq, 1);
  if (!eq)
    return 0;

  fir_eq_conf *conf = pa_xnew(fir_eq_conf, 1);
  eq->conf = conf;
  if (conf)
  {
    conf->b_eq = (int16_t *)pa_xmalloc0(128);
    if (conf->b_eq)
    {
      conf->dp_l = (int16_t *)pa_xmalloc0(208);
      if (conf->dp_l)
      {
        conf->dp_r = (int16_t *)pa_xmalloc0(208);
        if (conf->dp_r)
        {
          conf->a_lf = (int16_t *)pa_xmalloc0(2);
          if (conf->a_lf)
          {
            conf->b_lf = (int16_t *)pa_xmalloc0(4);
            if (conf->b_lf)
            {
              eq->scratch = (int32_t *)pa_xmalloc(160 * channels);
              if (eq->scratch)
                return eq;
              free(eq->conf->b_lf);
            }
            free(eq->conf->a_lf);
          }
          free(eq->conf->dp_r);
        }
        free(eq->conf->dp_l);
      }
      free(eq->conf->b_eq);
    }
    free(eq->conf);
  }
  free(eq);
  return 0;
}
