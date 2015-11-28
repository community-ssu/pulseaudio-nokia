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
#include "voice-aep-sink-input.h"
#include "voice-util.h"

/* This is a dummy stream and it's only purpose is to provide volume control */
/* for AEP DL audio. */

/* NOTE: This stream tries to remain corked at all cost on IO-thread side
 *       even if the main thread thinks it is running. However this quite
 *       hackish and may break on some future version of pulseaudio.
 */

/*** sink_input callbacks ***/
/* Called from I/O thread context */
static int aep_sink_input_pop_cb(pa_sink_input *i, size_t length,
                                 pa_memchunk *chunk)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    u = i->userdata;
    pa_assert(chunk && u);

    pa_log_debug("aep_sink_input_pop_cb should not be called, corking");
    pa_sink_input_set_state_within_thread(i, PA_SINK_INPUT_CORKED);

    pa_silence_memchunk_get(&u->core->silence_cache, u->core->mempool, chunk,
                            &i->sample_spec, length);

    return 0;
}

/* Called from I/O thread context */
static void aep_sink_input_process_rewind_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

/* Called from I/O thread context */
static void aep_sink_input_update_max_rewind_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

/* Called from I/O thread context */
static void aep_sink_input_update_max_request_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

/* Called from I/O thread context */
static void aep_sink_input_update_sink_latency_range_cb(pa_sink_input *i) {
    struct userdata *u;
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

/* Called from I/O thread context */
static void aep_sink_input_detach_cb(pa_sink_input *i) {
    struct userdata *u;
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    pa_log_debug("Detach called");
}

/* Called from I/O thread context */
static void aep_sink_input_attach_cb(pa_sink_input *i) {
    struct userdata *u;
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    pa_log_debug("Attach called, new master %p %s", (void*)u->raw_sink, u->raw_sink->name);
}

/* Called from main context */
static void aep_sink_input_kill_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_log_debug("Kill called");

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    /* FIXME: this is sort-of understandable with the may_move hack... we avoid abort in free() here */
    u->aep_sink_input->thread_info.attached = FALSE;
    pa_sink_input_unlink(u->aep_sink_input);
    pa_sink_input_unref(u->aep_sink_input);
    u->aep_sink_input = NULL;

    pa_module_unload_request(u->module, TRUE);
}

/* Called from IO thread context */
static void aep_sink_input_state_change_cb(pa_sink_input *i, pa_sink_input_state_t state) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_log_debug("State change cb %d", state);

    /* What ever the new state was, we should remain corked. */
    i->thread_info.state = PA_SINK_INPUT_CORKED;
}

int voice_init_aep_sink_input(struct userdata *u) {
    pa_sink_input_new_data data;
    char t[256];
    pa_assert(u);
    pa_assert(u->raw_sink);
    ENTER();

    pa_sink_input_new_data_init(&data);
    snprintf(t, sizeof(t), "output of %s", u->voip_sink->name);
    pa_proplist_sets(data.proplist, PA_PROP_MEDIA_NAME, t);
    data.sink = u->raw_sink;
    data.driver = __FILE__;
    data.module = u->module;
    data.origin_sink = u->voip_sink;
    pa_sink_input_new_data_set_sample_spec(&data, &u->raw_sink->sample_spec);
    pa_sink_input_new_data_set_channel_map(&data, &u->raw_sink->channel_map);
    pa_sink_input_new(&u->aep_sink_input, u->core, &data, PA_SINK_INPUT_START_CORKED|PA_SINK_INPUT_DONT_MOVE);
    pa_sink_input_new_data_done(&data);

    if (!u->aep_sink_input) {
	pa_log_debug("Creating sink input failed");
	return -1;
    }

    u->aep_sink_input->state_change = aep_sink_input_state_change_cb;
    u->aep_sink_input->pop = aep_sink_input_pop_cb;
    u->aep_sink_input->process_rewind = aep_sink_input_process_rewind_cb;
    u->aep_sink_input->update_max_rewind = aep_sink_input_update_max_rewind_cb;
    u->aep_sink_input->update_max_request = aep_sink_input_update_max_request_cb;
    u->aep_sink_input->update_sink_latency_range = aep_sink_input_update_sink_latency_range_cb;
    u->aep_sink_input->kill = aep_sink_input_kill_cb;
    u->aep_sink_input->attach = aep_sink_input_attach_cb;
    u->aep_sink_input->detach = aep_sink_input_detach_cb;
    u->aep_sink_input->userdata = u;

    return 0;
}
