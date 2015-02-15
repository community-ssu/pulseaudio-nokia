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
#ifndef voice_voip_sink_h
#define voice_voip_sink_h
#include <pulsecore/sink.h>

static inline
pa_bool_t voice_voip_sink_active(struct userdata *u) {
    pa_assert(u);
    return (u->voip_sink && u->voip_sink->state == PA_SINK_RUNNING);
}

static inline
pa_bool_t voice_voip_sink_active_iothread(struct userdata *u) {
    pa_assert(u);
    return (u->voip_sink && u->voip_sink->thread_info.state == PA_SINK_RUNNING);
}

int voice_init_voip_sink(struct userdata *u, const char *name);

#endif // voice_voip_sink_h
