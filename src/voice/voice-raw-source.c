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
#include "module-voice-userdata.h"
#include "voice-raw-source.h"
#include "voice-util.h"

/*** raw source callbacks ***/

/* Called from I/O thread context */
static int raw_source_process_msg(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk) {
    struct userdata *u = PA_SOURCE(o)->userdata;

    switch (code) {

        case PA_SOURCE_MESSAGE_GET_LATENCY: {
            pa_usec_t usec = 0;

            if (PA_MSGOBJECT(u->master_source)->process_msg(
        PA_MSGOBJECT(u->master_source), PA_SOURCE_MESSAGE_GET_LATENCY, &usec, 0, NULL) < 0)
                usec = 0;

            *((pa_usec_t*) data) = usec + pa_bytes_to_usec(pa_memblockq_get_length(u->hw_source_memblockq),
                 &u->raw_source->sample_spec);
            return 0;
        }
    }

    return pa_source_process_msg(o, code, data, offset, chunk);
}

/* Called from main context */
static int raw_source_set_state(pa_source *s, pa_source_state_t state) {
    struct userdata *u;
    int ret;
    ENTER();

    pa_source_assert_ref(s);
    pa_assert_se(u = s->userdata);

    ret = voice_source_set_state(s, u->voip_source, state);

    pa_log_debug("(%p): called with %d", (void *)s, state);

    return ret;
}

int voice_init_raw_source(struct userdata *u, const char *name) {
    pa_source_new_data data;
    ENTER();

    pa_assert(u);
    pa_assert(u->master_source);

    pa_source_new_data_init(&data);
    data.driver = __FILE__;
    data.module = u->module;
    pa_source_new_data_set_name(&data, name);
    pa_proplist_setf(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "%s source connected to %s", name, u->master_source->name);
    pa_proplist_sets(data.proplist, PA_PROP_DEVICE_MASTER_DEVICE, u->master_source->name);
    pa_proplist_sets(data.proplist, "module-suspend-on-idle.timeout", "1");

    pa_source_new_data_set_sample_spec(&data, &u->hw_mono_sample_spec);
    pa_source_new_data_set_channel_map(&data, &u->mono_map);

    u->raw_source = pa_source_new(u->core, &data, 0);
    pa_source_new_data_done(&data);

    if (!u->raw_source) {
        pa_log_error("Failed to create source.");
  return -1;
    }

    u->raw_source->parent.process_msg = raw_source_process_msg;
    u->raw_source->set_state = raw_source_set_state;
    u->raw_source->userdata = u;
    u->raw_source->flags = 0; // PA_SOURCE_CAN_SUSPEND

    pa_source_set_asyncmsgq(u->raw_source, u->master_source->asyncmsgq);
    pa_source_set_rtpoll(u->raw_source, u->master_source->rtpoll);

    return 0;
}
