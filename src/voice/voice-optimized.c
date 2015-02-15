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

#include "voice-optimized.h"
#include "src/common/optimized.h"

int voice_take_channel1(struct userdata *u, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    pa_assert(0 == (ichunk->length % (16*sizeof(short))));

    ochunk->length = ichunk->length/2;
    ochunk->index = 0;
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    short *output = (short *) pa_memblock_acquire(ochunk->memblock);
    const short *input = ((short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short));
    extract_mono_from_interleaved_stereo(input, output, ichunk->length/sizeof(short));
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);
    return 0;
}

int voice_downmix_to_mono(struct userdata *u, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    pa_assert(0 == (ichunk->length % (16*sizeof(short))));

    ochunk->length = ichunk->length/2;
    ochunk->index = 0;
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    short *output = (short *) pa_memblock_acquire(ochunk->memblock);
    const short *input = ((short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short));
    downmix_to_mono_from_interleaved_stereo(input, output, ichunk->length/sizeof(short));
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

int voice_equal_mix_in(pa_memchunk *ochunk, const pa_memchunk *ichunk) {
    /* This assert is too much for inlining the function : */
    /* pa_assert(ochunk && ochunk->memblock && ichunk && ichunk->memblock); */
    pa_assert(ochunk->length == ichunk->length);
    pa_assert(0 == (ichunk->length % (8*sizeof(short))));

    short *output = ((short *)pa_memblock_acquire(ochunk->memblock) + ochunk->index/sizeof(short));
    const short *input = ((short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short));
    symmetric_mix(input, output, output, ichunk->length/sizeof(short));
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

int voice_mix_in_with_volume(pa_memchunk *ochunk, const pa_memchunk *ichunk, const pa_volume_t vol) {
    pa_assert(ochunk);
    pa_assert(ochunk->memblock);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    pa_assert(ochunk->length == ichunk->length);
    pa_assert(0 == (ichunk->length % (8*sizeof(short))));

    short volume = INT16_MAX;
    if (vol < PA_VOLUME_NORM)
	volume = (short) lrint(pa_sw_volume_to_linear(vol)*INT16_MAX);
    pa_log_debug("pavolume 0x%x, volume %d (linear %f)", vol, volume, pa_sw_volume_to_linear(vol));
    short *output = ((short *)pa_memblock_acquire(ochunk->memblock) + ochunk->index/sizeof(short));
    const short *input = ((short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short));
    mix_in_with_volume(volume, input, output, ichunk->length/sizeof(short));
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

int voice_apply_volume(pa_memchunk *chunk, const pa_volume_t vol) {
    pa_assert(chunk);
    pa_assert(chunk->memblock);
    pa_assert(0 == (chunk->length % (8*sizeof(short))));

    short volume = INT16_MAX;
    if (vol < PA_VOLUME_NORM)
	volume = (short) lrint(pa_sw_volume_to_linear(vol)*INT16_MAX);
    short *input = ((short *)pa_memblock_acquire(chunk->memblock) + chunk->index/sizeof(short));
    apply_volume(volume, input, input, chunk->length/sizeof(short));
    pa_memblock_release(chunk->memblock);

    return 0;
}

int voice_mono_to_stereo(struct userdata *u, const pa_memchunk *ichunk, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk);
    pa_assert(ichunk->memblock);
    pa_assert(0 == (ichunk->length % (8*sizeof(short))));

    ochunk->length = 2*ichunk->length;
    ochunk->index = 0;
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    short *output = (short *) pa_memblock_acquire(ochunk->memblock);
    const short *input = ((short *)pa_memblock_acquire(ichunk->memblock) + ichunk->index/sizeof(short));
    dup_mono_to_interleaved_stereo(input, output, ichunk->length/sizeof(short));
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk->memblock);

    return 0;
}

int voice_interleave_stereo(struct userdata *u, const pa_memchunk *ichunk1, const pa_memchunk *ichunk2, pa_memchunk *ochunk) {
    pa_assert(u);
    pa_assert(ochunk);
    pa_assert(ichunk1);
    pa_assert(ichunk2);
    pa_assert(ichunk1->memblock);
    pa_assert(ichunk2->memblock);
    pa_assert(0 == (ichunk1->length % (8*sizeof(short))));
    pa_assert(ichunk1->length == ichunk2->length);

    ochunk->length = 2*ichunk1->length;
    ochunk->index = 0;
    ochunk->memblock = pa_memblock_new(u->core->mempool, ochunk->length);
    short *output = (short *) pa_memblock_acquire(ochunk->memblock);
    const short *input1 = ((short *)pa_memblock_acquire(ichunk1->memblock) + ichunk1->index/sizeof(short));
    const short *input2 = ((short *)pa_memblock_acquire(ichunk2->memblock) + ichunk2->index/sizeof(short));
    const short *bufs[2] = { input1, input2 };
    interleave_mono_to_stereo(bufs, output, ichunk1->length/sizeof(short));
    pa_memblock_release(ochunk->memblock);
    pa_memblock_release(ichunk1->memblock);
    pa_memblock_release(ichunk2->memblock);

    return 0;
}

