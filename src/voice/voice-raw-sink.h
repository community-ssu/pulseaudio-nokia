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
#ifndef voice_raw_sink_h
#define voice_raw_sink_h

static inline
pa_bool_t voice_raw_sink_active(struct userdata *u) {
    return (u->raw_sink && (u->raw_sink->state == PA_SINK_RUNNING ||
                            u->raw_sink->state == PA_SINK_IDLE));
}

static inline
pa_bool_t voice_raw_sink_active_iothread(struct userdata *u) {
    return (u->raw_sink && (u->raw_sink->thread_info.state == PA_SINK_RUNNING));
}


int voice_init_raw_sink(struct userdata *u, const char *name);

#endif // voice_raw_sink_h
