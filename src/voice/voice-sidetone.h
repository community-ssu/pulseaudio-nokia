#ifndef VOICESIDETONE_H
#define VOICESIDETONE_H

int32_t voice_limit_sidetone(int32_t val);
void voice_update_sidetone_gain(int16_t gain_idx);
void sidetone_write_parameters(struct userdata *u);
void voice_update_shc(struct userdata *u, int32_t tone);
void voice_turn_sidetone_down(void);
void voice_enable_sidetone(struct userdata *u, pa_bool_t enable);

#endif // VOICESIDETONE_H
