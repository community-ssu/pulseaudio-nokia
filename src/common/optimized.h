/* TODO: those functions are exported and should have a namespace, such as meego_rabbit_ */

#ifndef OPTIMIZE_H_
#define OPTIMIZE_H_

#include <stdint.h>

void move_16bit_to_32bit(int32_t *dst, const short *src, unsigned n);
void move_32bit_to_16bit(short *dst, const int32_t *src, unsigned n);

void interleave_mono_to_stereo(const short *src[], short *dst, unsigned n);
void deinterleave_stereo_to_mono(const short *src, short *dst[], unsigned n);
void extract_mono_from_interleaved_stereo(const short *src, short *dst, unsigned n);
void downmix_to_mono_from_interleaved_stereo(const short *src, short *dst, unsigned n);
void dup_mono_to_interleaved_stereo(const short *src, short *dst, unsigned n);
void symmetric_mix(const short *src1, const short *src2, short *dst, const unsigned n);
void mix_in_with_volume(const short volume, const short *src, short *dst, const unsigned n);
void apply_volume(const short volume, const short *src, short *dst, const unsigned n);

#endif
