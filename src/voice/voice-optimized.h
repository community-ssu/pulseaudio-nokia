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
#ifndef voice_optimized_h
#define voice_optimized_h

#include "module-voice-userdata.h"

int voice_take_channel1(struct userdata *u, const pa_memchunk *ichunk, pa_memchunk *ochunk);
int voice_downmix_to_mono(struct userdata *u, const pa_memchunk *ichunk, pa_memchunk *ochunk);
int voice_equal_mix_in(pa_memchunk *ochunk, const pa_memchunk *ichunk);
int voice_mix_in_with_volume(pa_memchunk *ochunk, const pa_memchunk *ichunk, const pa_volume_t vol);
int voice_apply_volume(pa_memchunk *chunk, const pa_volume_t vol);
int voice_mono_to_stereo(struct userdata *u, const pa_memchunk *ichunk, pa_memchunk *ochunk);
int voice_interleave_stereo(struct userdata *u, const pa_memchunk *ichunk1, const pa_memchunk *ichunk2, pa_memchunk *ochunk);

#endif // voice_optimized_h
