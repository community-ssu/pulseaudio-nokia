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

#include "module-voice-userdata.h"
#include "voice-raw-sink.h"
#include "voice-util.h"
#include <pulse/volume.h>

/*** raw sink callbacks ***/

/* Called from I/O thread context */
static int raw_sink_process_msg(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk) {
    struct userdata *u = PA_SINK(o)->userdata;

    if (!u->master_sink)
        return -1;

    switch (code) {

        case PA_SINK_MESSAGE_GET_LATENCY: {
            pa_usec_t usec = 0;

            if (PA_MSGOBJECT(u->master_sink)->process_msg(PA_MSGOBJECT(u->master_sink), PA_SINK_MESSAGE_GET_LATENCY, &usec, 0, NULL) < 0)
                usec = 0;

            *((pa_usec_t*) data) = usec;
            return 0;
        }

        case PA_SINK_MESSAGE_ADD_INPUT: {
            pa_sink_input *i = PA_SINK_INPUT(data);
            if (i == u->hw_sink_input) {
                pa_log_error("Denied loop connection");
                return -1;
            }

            break;
        }

    }

    return pa_sink_process_msg(o, code, data, offset, chunk);
}

/* Called from main context */
static int raw_sink_set_state(pa_sink *s, pa_sink_state_t state) {
    struct userdata *u;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    int  ret = voice_sink_set_state(s, u->voip_sink, state);
    pa_log_debug("(%p) called with %d", (void *)s, state);
    return ret;
}

/* Called from I/O thread context */
static void raw_sink_request_rewind(pa_sink *s) {
    struct userdata *u;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    /* Just hand this one over to the master sink */
    if (u->hw_sink_input)
        pa_sink_input_request_rewind(u->hw_sink_input, s->thread_info.rewind_nbytes, TRUE, FALSE, FALSE);
}

/* Called from I/O thread context */
static void raw_sink_update_requested_latency(pa_sink *s) {
    struct userdata *u;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    /* Just hand this one over to the master sink */
    pa_sink_input_set_requested_latency_within_thread(
            u->hw_sink_input,
            pa_sink_get_requested_latency_within_thread(s));
}

int voice_init_raw_sink(struct userdata *u, const char *name) {
    pa_sink_new_data sink_data;

    pa_assert(u);
    pa_assert(u->core);
    pa_assert(u->master_sink);

    pa_sink_new_data_init(&sink_data);
    sink_data.module = u->module;
    sink_data.driver = __FILE__;
    sink_data.flat_volume_sink = u->master_sink;
    pa_sink_new_data_set_name(&sink_data, name);
    pa_sink_new_data_set_sample_spec(&sink_data, &u->master_sink->sample_spec);
    pa_sink_new_data_set_channel_map(&sink_data, &u->master_sink->channel_map);
    pa_proplist_setf(sink_data.proplist, PA_PROP_DEVICE_DESCRIPTION, "%s connected to %s", name, u->master_sink->name);
    pa_proplist_sets(sink_data.proplist, PA_PROP_DEVICE_MASTER_DEVICE, u->master_sink->name);
    pa_proplist_sets(sink_data.proplist, "module-suspend-on-idle.timeout", "1");

    /* Create sink */
    u->raw_sink = pa_sink_new(u->core, &sink_data, PA_SINK_LATENCY);
    pa_sink_new_data_done(&sink_data);
    /* Create sink */
    if (!u->raw_sink) {
        pa_log_error("Failed to create sink.");
        return -1;
    }

    u->raw_sink->parent.process_msg = raw_sink_process_msg;
    u->raw_sink->set_state = raw_sink_set_state;
    u->raw_sink->update_requested_latency = raw_sink_update_requested_latency;
    u->raw_sink->request_rewind = raw_sink_request_rewind;
    u->raw_sink->userdata = u;
    u->raw_sink->flags = PA_SINK_LATENCY;

    pa_sink_set_asyncmsgq(u->raw_sink, u->master_sink->asyncmsgq);
    pa_sink_set_rtpoll(u->raw_sink, u->master_sink->rtpoll);

    return 0;
}
