/*
 * This file is part of pulseaudio-meego
 *
 * Copyright (C) 2008, 2009 Nokia Corporation. All rights reserved.
 *
 * Contact: Maemo Multimedia <multimedia@maemo.org>
 *
 * This software, including documentation, is protected by copyright
 * controlled by Nokia Corporation. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating,
 * any or all of this material requires the prior written consent of
 * Nokia Corporation. This material also contains confidential
 * information which may not be disclosed to others without the prior
 * written consent of Nokia.
 */
#ifndef voice_aep_convert_h
#define voice_aep_convert_h

#include "module-voice-userdata.h"

/* TODO: Move init and free calls to pa__init and pa__done. The src wrappers should be
         moved to common */

static inline
int voice_convert_init(struct userdata *u) {
    pa_assert(u);

    u->hw_source_to_aep_resampler = alloc_src_48_to_8();

    u->aep_to_hw_sink_resampler = alloc_src_8_to_48();

    u->ear_to_aep_resampler = alloc_src_48_to_8();

    u->raw_sink_to_hw8khz_sink_resampler = alloc_src_48_to_8();

    u->hw8khz_source_to_raw_source_resampler = alloc_src_8_to_48();

    return 0;
}

static inline
int voice_convert_free(struct userdata *u) {
    pa_assert(u);

    free_src_48_to_8(u->hw_source_to_aep_resampler);

    free_src_8_to_48(u->aep_to_hw_sink_resampler);

    free_src_48_to_8(u->ear_to_aep_resampler);

    free_src_48_to_8(u->raw_sink_to_hw8khz_sink_resampler);

    free_src_8_to_48(u->hw8khz_source_to_raw_source_resampler);

    return 0;
}

static inline
int voice_convert_run_48_to_8(struct userdata *u, src_48_to_8 *s, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    int input_frames = ichunk->length/sizeof(short);
    int ouput_frames = output_frames_src_48_to_8_total(input_frames);
    pa_assert(ouput_frames > 0);

    ochunk->length = ouput_frames*sizeof(short);
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    ochunk->index = 0;
    short *output = pa_memblock_acquire(ochunk->memblock);
    short *input = (short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short);
    int i = 0;
    int o = 0;
    while (i < input_frames) {
        int iframes = input_frames - i;
        if (iframes > SRC_48_TO_8_MAX_INPUT_FRAMES)
            iframes = SRC_48_TO_8_MAX_INPUT_FRAMES;
        process_src_48_to_8(s, output + o, input + i, input_frames);
        i += iframes;
        o = output_frames_src_48_to_8_total(i);
    }
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

static inline
int voice_convert_run_48_stereo_to_8(struct userdata *u, src_48_to_8 *s, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    int input_samples = ichunk->length/sizeof(short);
    int output_frames = output_frames_src_48_to_8_total(input_samples/2);

    pa_assert(output_frames > 0);

    ochunk->length = output_frames*sizeof(short);
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    ochunk->index = 0;
    short *output = pa_memblock_acquire(ochunk->memblock);
    short *input = (short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short);
    int i = 0;
    int o = 0;
    while (i < input_samples) {
        int iframes = input_samples - i;
        if (iframes > SRC_48_TO_8_MAX_INPUT_FRAMES*2)
            iframes = SRC_48_TO_8_MAX_INPUT_FRAMES*2;
        process_src_48_to_8_stereo_to_mono(s, output + o, input + i, iframes);
        i += iframes;
        o = output_frames_src_48_to_8_total(i/2);
    }
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

static inline
int voice_convert_run_8_to_48(struct userdata *u, src_8_to_48 *s, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    int input_frames = ichunk->length/sizeof(short);
    int ouput_frames = output_frames_src_8_to_48(input_frames);
    pa_assert(ouput_frames > 0);

    ochunk->length = ouput_frames*sizeof(short);
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    ochunk->index = 0;
    short *output = pa_memblock_acquire(ochunk->memblock);
    short *input = (short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short);
    process_src_8_to_48(s, output, input, input_frames);
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

static inline
int voice_convert_run_8_to_48_stereo(struct userdata *u, src_8_to_48 *s, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    int input_frames = ichunk->length/sizeof(short);
    int ouput_frames = output_frames_src_8_to_48(input_frames);
    pa_assert(ouput_frames > 0);

    ochunk->length = ouput_frames*2*sizeof(short);
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    ochunk->index = 0;
    short *output = pa_memblock_acquire(ochunk->memblock);
    short *input = (short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short);
    process_src_8_to_48_mono_to_stereo(s, output, input, input_frames);
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

#endif // voice_aep_convert_h
