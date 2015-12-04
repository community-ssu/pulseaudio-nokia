#include "equ.h"

struct fir_eq
{
  fir_eq_conf *conf;
  int use_fireq;
  int use_dyn_resp;
  int32_t *scratch;
  int unk1;
  int unk2;
};

void fir_eq_free(struct fir_eq *eq);
void fir_eq_process_stereo(struct fir_eq *eq, int16_t *src_left, int16_t *src_right, int16_t *dst_left, int16_t *dst_right, int32_t samples);
void fir_eq_change_params(struct fir_eq *eq, const void *parameters, size_t length);
struct fir_eq *fir_eq_new(int sampling_frequency, int channels);

