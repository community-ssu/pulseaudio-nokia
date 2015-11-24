struct fir_eq
{
  int dummy;//structure def not known yet
};

void fir_eq_free(struct fir_eq *eq);
void fir_eq_process_stereo(struct fir_eq *eq, int16_t *src_left, int16_t *src_right, int16_t *dst_left, int16_t *dst_right, int32_t samples);
void fir_eq_change_params(struct fir_eq *eq, void *parameters, size_t length);
struct fir_eq *fir_eq_new(int sampling_frequency, int channels);

