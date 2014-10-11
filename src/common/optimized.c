#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "optimized.h"

#ifdef __ARM_NEON__
#include <arm_neon.h>

#define OPTIMIZE_MOVE
#define OPTIMIZE_INTERLEAVE
#define OPTIMIZE_MIX
#else

/* Get rid of PA dependecy */
#if 0
#include "config.h"
#include <pulsecore/macro.h>
#else
#define PA_UNLIKELY(x) (x)
#define PA_CLAMP_UNLIKELY(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

#endif

#ifdef OPTIMIZE_MOVE

void move_16bit_to_32bit(int32_t *dst, const short *src, unsigned n)
{
    unsigned i;
    int16x4x2_t input;
    int32x4x2_t mid;
    int32x4x2_t output;

    for (i = 0; i < n/2; i += 8) {
        input = vld2_s16(src + i);
        mid.val[0] = vmovl_s16(input.val[0]);
        mid.val[1] = vmovl_s16(input.val[1]);
        output.val[0] = vshlq_n_s32(mid.val[0], 8);
        output.val[1] = vshlq_n_s32(mid.val[1], 8);
        vst2q_s32((int32_t *)dst + i, output);
    }
}

void move_32bit_to_16bit(short *dst, const int32_t *src, unsigned n)
{
    unsigned i;
    int32x4x2_t input;
    int32x4x2_t mid;
    int16x4x2_t output;

    for (i = 0; i < n/2; i += 8) {
        input = vld2q_s32((int32_t *)src + i);
        mid.val[0] = vshrq_n_s32(input.val[0], 8);
        mid.val[1] = vshrq_n_s32(input.val[1], 8);
        output.val[0] = vqmovn_s32(mid.val[0]);
        output.val[1] = vqmovn_s32(mid.val[1]);
        vst2_s16(dst + i, output);
    }
}
#endif

#ifdef OPTIMIZE_INTERLEAVE
void interleave_mono_to_stereo(const short *src[], short *dst, unsigned n)
{
    unsigned i = 0;
    unsigned offset = 16;
    int16x8x2_t loaded;

    for (i = 0; i < n; i += 8) {
        loaded.val[0] = vld1q_s16 (src[0] + i);
        loaded.val[1] = vld1q_s16 (src[1] + i);
        vst2q_s16 (dst, loaded);
        dst += offset;
    }
}

void deinterleave_stereo_to_mono(const short *src, short *dst[], unsigned n)
{
    unsigned i;
    unsigned offset = 8;
    short *channel_1 = dst[0];
    short *channel_2 = dst[1];
    int16x8x2_t result;

    for (i = 0; i < n; i += 16) {
        result = vld2q_s16 (src + i);
        vst1q_s16 (channel_1, result.val[0]);
        vst1q_s16 (channel_2, result.val[1]);
        channel_1 += offset;
        channel_2 += offset;
    }
}

void extract_mono_from_interleaved_stereo(const short *src, short *dst, unsigned n)
{
    unsigned i;
    unsigned offset = 8;
    int16x8x2_t result;

    for (i = 0; i < n; i += 16) {
        result = vld2q_s16 (src + i);
        vst1q_s16 (dst, result.val[0]);
       dst += offset;
    }
}

void downmix_to_mono_from_interleaved_stereo(const short *src, short *dst, unsigned n)
{
    unsigned i;
    unsigned offset = 8;
    int16x8x2_t stereo_samples;
    int16x8_t mono_samples;

    for (i = 0; i < n; i += 16) {
        stereo_samples = vld2q_s16 (src + i);
	/* Shift right before downmixing */
	vrshrq_n_s16(stereo_samples.val[0], 1);
	vrshrq_n_s16(stereo_samples.val[1], 1);

	mono_samples = vqaddq_s16(stereo_samples.val[0], stereo_samples.val[1]);
	vst1q_s16(dst, mono_samples);
        dst += offset;
    }
}

void dup_mono_to_interleaved_stereo(const short *src, short *dst, unsigned n)
{
    unsigned i;
    int16x8_t input;
    int16x8x2_t result;

    for (i = 0; i < n; i += 8) {
        input = vld1q_s16 (src + i);
        result.val[0] = input;
        result.val[1] = input;
        vst2q_s16 (dst, result);
        dst += 16;
    }
}
#endif

#ifdef OPTIMIZE_MIX

void symmetric_mix(const short *src1, const short *src2, short *dst, const unsigned n)
{
    unsigned i;
    int16x8_t input1;
    int16x8_t input2;
    int16x8_t result;

    for (i = 0; i < n; i += 8) {
        input1 = vld1q_s16 (src1 + i);
        input2 = vld1q_s16 (src2 + i);
        result = vqaddq_s16 (input1, input2);
        vst1q_s16 (dst + i, result);
    }
}

void mix_in_with_volume(const short v, const short *src, short *dst, const unsigned n)
{
    unsigned i;
    int16_t volume = v/2;
    int16x4_t input;
    int32x4_t sum;
    int16x4_t result;

    /* TODO: Running vqdmlal_lane_s16 in 2 or 4 lanes in parallel would probably help */
    for (i = 0; i < n; i += 4) {
        input = vld1_s16(src + i);
	result = vld1_s16(dst + i);
	sum = vshll_n_s16(result, 15);
	sum = vqdmlal_n_s16(sum, input, volume);
	result = vqrshrn_n_s32(sum, 15);
        vst1_s16 (dst + i, result);
    }
}

void apply_volume(const short volume, const short *src, short *dst, const unsigned n)
{
    unsigned i;
    int16x4_t input;
    int32x4_t wideres;
    int16x4_t result;

    /* TODO: Running vmull_lane_s16 in 2 or 4 lanes in parallel would probably help */
    for (i = 0; i < n; i += 4) {
        input = vld1_s16(src + i);
	wideres = vmull_n_s16(input, volume);
	result = vqrshrn_n_s32(wideres, 15);
        vst1_s16 (dst + i, result);
    }
}


#endif

#ifndef OPTIMIZE_MOVE
void move_16bit_to_32bit(int32_t *dst, const short *src, unsigned n)
{
    unsigned i;

    for (i = 0; i < n; i ++) {
        *(dst + i) = *(src + i) << 8;
    }
}

void move_32bit_to_16bit(short *dst, const int32_t *src, unsigned n)
{
    unsigned i;
    int32_t t;

    for (i = 0; i < n; i ++) {
        t = *(src + i);
        if (t > 8388607)
            *(dst + i) = 32767;
        else if (t < -8388608)
            *(dst + i) = -32768;
        else
            *(dst + i) = (short) (t >> 8);
    }
}
#endif

#ifndef OPTIMIZE_INTERLEAVE
void interleave_mono_to_stereo(const short *src[], short *dst, unsigned n)
{
    unsigned i;
    unsigned offset = 16;

    for (i = 0; i < n; i += 8) {
        unsigned i1 = 0, i2 = 0;

        while (i2 < 8) {
            *(dst + i1) = *(src[0] + i + i2); i1++;
            *(dst + i1) = *(src[1] + i + i2); i1++; i2++;
        }

        dst += offset;
    }
}

void deinterleave_stereo_to_mono(const short *src, short *dst[], unsigned n)
{
    unsigned i;
    unsigned offset = 8;
    short *channel_1 = dst[0];
    short *channel_2 = dst[1];

    for (i = 0; i < n; i += 16) {
        unsigned i1 = 0, i2 = 0;

        while (i2 < 8) {
            *(channel_1 + i2) = *(src + i + i1); i1++;
            *(channel_2 + i2) = *(src + i + i1); i1++; i2++;
        }

        channel_1 += offset;
        channel_2 += offset;
    }
}

void extract_mono_from_interleaved_stereo(const short *src, short *dst, unsigned n)
{
    unsigned i;
    unsigned offset = 8;

    for (i = 0; i < n; i += 16) {
        unsigned i1 = 0, i2 = 0;

        while (i2 < 8) {
            *(dst + i2) = *(src + i + i1); i1++;
            i1++; i2++;
        }

        dst += offset;
    }
}

void downmix_to_mono_from_interleaved_stereo(const short *src, short *dst, unsigned n)
{
    unsigned i;
    unsigned offset = 8;

    for (i = 0; i < n; i += 16) {
        unsigned i1 = 0, i2 = 0;

        while (i2 < 8) {
	    int sum;
            sum = *(src + i + i1); i1++;
            sum += *(src + i + i1); i1++;
	    *(dst + i2) = (short)PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF); i2++;
        }

        dst += offset;
    }
}

void dup_mono_to_interleaved_stereo(const short *src, short *dst, unsigned n)
{
    unsigned i;

    for (i = 0; i < n; i += 8) {
        unsigned i1 = 0, i2 = 0;

        while (i2 < 8) {
            *(dst + i1) = *(src + i + i2); i1++;
            *(dst + i1) = *(src + i + i2); i1++; i2++;
        }

        dst += 16;
    }
}
#endif


#ifndef OPTIMIZE_MIX

void symmetric_mix(const short *src1, const short *src2, short *dst, const unsigned n)
{
    unsigned i, j;

    for (i = 0; i < n; i += 8)
        for (j = i; j < i+8; ++j)
            dst[j] = (short)PA_CLAMP_UNLIKELY((int)src1[j] + (int)src2[j], -0x8000, 0x7FFF);
}

void mix_in_with_volume(const short volume, const short *src, short *dst, const unsigned n)
{
    unsigned i, j;

    for (i = 0; i < n; i += 4)
        for (j = i; j < i+4; ++j) {
	    dst[j] = (short)PA_CLAMP_UNLIKELY((int)dst[j] + ((2*(int)src[j]*(int)volume)>>16), -0x8000, 0x7FFF);
	}
}

void apply_volume(const short volume, const short *src, short *dst, const unsigned n)
{
    unsigned i, j;

    for (i = 0; i < n; i += 4)
        for (j = i; j < i+4; ++j) {
	    dst[j] = (short)PA_CLAMP_UNLIKELY((int)((2*(int)src[j]*(int)volume)>>16), -0x8000, 0x7FFF);
	}
}

#endif
