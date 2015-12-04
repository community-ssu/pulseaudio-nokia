#ifndef __IIR_H__
#define __IIR_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fir_eq_conf_str
{
  int16_t b_zero;
  int16_t *b_eq;
  int16_t *b_lf;
  int16_t *a_lf;
  int32_t n_eq;
  int32_t seed;
  int32_t dither_old;
  int16_t glin;
  int16_t gnom;
  int16_t hold_attack;
  int16_t attack_count;
  int16_t glin_count;
  int16_t hold_gain;
  int16_t coef;
  int16_t ca;
  int16_t c1;
  int16_t c2;
  int32_t amaxeqd;
  int32_t anoteqd;
  int16_t attack;
  int16_t decay;
  int16_t drop_gain_threshold;
  int32_t r_d;
  int32_t l_d;
  int16_t *dp_l;
  int16_t *dp_r;
};

typedef struct fir_eq_conf_str fir_eq_conf;

void
a_equ(int16_t *src, int16_t *coeff, int32_t *delay, int16_t *dst,
      int32_t samples, int32_t *scratch);

void
filter_samples2(int16_t *samples_left, int16_t *samples_right,
                int16_t *out_left, int16_t *out_right, int32_t numsamples,
                int16_t stereo, fir_eq_conf *fir_eq_conf_var,
                int32_t *scratch);

#ifdef __cplusplus
}
#endif

#endif
