/*
 * This file is part of pulseaudio-nokia
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

#ifndef module_nokia_cmtspeech_h
#define module_nokia_cmtspeech_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/source-output.h>
#include <pulsecore/sink-input.h>
#include <pulsecore/module.h>
#include <pulsecore/core.h>
#include <pulsecore/msgobject.h>
#include <pulsecore/atomic.h>
#include <pulsecore/semaphore.h>
#include <pulsecore/mutex.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/thread.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/asyncq.h>
#include <pulsecore/dbus-shared.h>

#include <cmtspeech.h>

#define CMTSPEECH_SAMPLERATE   (8000)

#define ENTER() pa_log_debug("%d: %s() called", __LINE__, __FUNCTION__)
#define ONDEBUG_TOKENS(a)

#define PROPLIST_SINK "sink.hw0"

struct userdata {
    pa_core *core;
    pa_module *module;

    pa_defer_event *unload_de;

    pa_channel_map map;
    pa_sample_spec ss;
    size_t dl_frame_size;
    size_t ul_frame_size;

    pa_sink *sink;
    pa_source *source;

    pa_sink_input *sink_input;
    pa_source_output *source_output;

    /* Access only from sink IO-thread */
    pa_queue *local_sideinfoq;
    pa_queue *voice_sideinfoq;
    pa_bool_t continuous_dl_stream;
    pa_memblockq *dl_memblockq;

    pa_msgobject *mainloop_handler;

    struct cmtspeech_dbus_conn {
	DBusBusType dbus_type;
	pa_dbus_connection *dbus_conn;
	char *dbus_match_rules[32];
    } dbus_conn;

    struct cmtspeech_connection {
        pa_msgobject *cmt_handler;
	pa_atomic_t thread_state;
	pa_fdsem *thread_state_change;
	cmtspeech_t *cmtspeech;
	pa_mutex *cmtspeech_mutex;
	pa_rtpoll *rtpoll;
	pa_rtpoll_item *cmt_poll_item;
	pa_rtpoll_item *thread_state_poll_item;
        pa_thread *thread;
	pa_thread_mq thread_mq;

	pa_asyncq *dl_frame_queue;

	pa_bool_t call_ul;			/* set according to DBus signals */
	pa_bool_t call_dl;			/* set according to DBus signals */
	pa_bool_t call_emergency;		/* set according to DBus signals */
	pa_bool_t first_dl_frame_received;	/* internal state */
	pa_bool_t record_running;		/* internal state */
	pa_bool_t playback_running;		/* internal state */
    } cmt_connection;
};

int cmtspeech_check_sink_api(pa_sink *s);
int cmtspeech_check_source_api(pa_source *s);

void cmtspeech_trigger_unload(struct userdata *u);

#endif /* module_nokia_cmtspeech_h */
