#ifndef __EQ_IIR_H__
#define __EQ_IIR_H__

struct eq_channel
{
  int16_t *coeff[2];
  int32_t *delay[2];
};

struct iir_eq
{
  struct eq_channel channel;
  int32_t *scratch;
};

void iir_eq_process_mono(struct iir_eq *eq, int16_t *src, int16_t *dst,
                         int32_t samples);
void iir_eq_free(struct iir_eq *eq);
struct iir_eq *iir_eq_new(int max_samples_per_frame, int channels);
void iir_eq_change_params(struct iir_eq *eq, const void *parameters,
                          size_t length);

#endif
