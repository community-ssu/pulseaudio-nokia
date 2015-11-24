#ifndef VOICESIDETONE_H
#define VOICESIDETONE_H

int voice_limit_sidetone(int val);
void voice_update_sidetone_gain(int16_t gain);
void sidetone_write_parameters(struct userdata *u);
void voice_update_shc(struct userdata *u, int tone);
void voice_turn_sidetone_down(void);
void voice_enable_sidetone(struct userdata *u, pa_bool_t enable);

#endif // VOICESIDETONE_H
