/*
 * This file is part of pulseaudio-meego
 *
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
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
#ifndef voice_voip_source_h
#define voice_voip_source_h

#include <pulsecore/source.h>

static inline
pa_bool_t voice_voip_source_active(struct userdata *u) {
    return (u->voip_source && (u->voip_source->state == PA_SOURCE_RUNNING ||
                               u->voip_source->state == PA_SOURCE_IDLE));
}

static inline
pa_bool_t voice_voip_source_active_iothread(struct userdata *u) {
    return (u->voip_source && (u->voip_source->thread_info.state == PA_SOURCE_RUNNING));
}

int voice_init_voip_source(struct userdata *u, const char *name);

#endif // voice_voip_source_h
