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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/namereg.h>

#include "module-nokia-cmtspeech.h"
#include "module-voice-api.h"
#include "memory.h"
#include "cmtspeech-source-output.h"
#include "cmtspeech-connection.h"

/* Called from thread context */
static void cmtspeech_source_output_push_cb(pa_source_output *o, const pa_memchunk *chunk) {
    struct userdata *u;
    uint8_t *buf;

    pa_assert(o);
    pa_assert_se(u = o->userdata);

    if (chunk->length != u->ul_frame_size) {
        pa_log_warn("Pushed UL audio frame has wrong size %d", chunk->length);
        return;
    }

    buf = ((uint8_t *) pa_memblock_acquire(chunk->memblock)) + chunk->index;

    (void)cmtspeech_send_ul_frame(u, buf, chunk->length);

    pa_memblock_release(chunk->memblock);
}

/* Called from I/O thread context */
static void cmtspeech_source_output_detach_cb(pa_source_output *o) {
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_assert_se(u = o->userdata);

    pa_log_debug("CMT source output detach called");
}

/* Called from I/O thread context */
static void cmtspeech_source_output_attach_cb(pa_source_output *o) {
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_assert_se(u = o->userdata);

    pa_assert(u->source == o->source);

    pa_log_debug("CMT source output connected to %s", o->source->name);
}

/* Called from I/O thread context */
static void cmtspeech_source_output_state_change_cb(pa_source_output *o, pa_source_output_state_t state) {
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_assert_se(u = o->userdata);

    pa_log_debug("State changed %d -> %d", o->thread_info.state, state);

    if (state == PA_SOURCE_OUTPUT_CORKED && o->thread_info.state != PA_SOURCE_OUTPUT_CORKED) {
        /* UL closing - nothing to do */
        pa_log_info("UL corked");
    }
    else if (state != PA_SOURCE_OUTPUT_CORKED && o->thread_info.state == PA_SOURCE_OUTPUT_CORKED) {
        /* UL opening - nothing to do */
        pa_log_info("UL uncorked");
    }
}

/* Called from main context */
static void cmtspeech_source_output_kill_cb(pa_source_output* o) {
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_assert_se(u = o->userdata);
    pa_assert(u->source_output == o);

    pa_log_warn("Kill called for cmtspeech source output");
    cmtspeech_trigger_unload(u);

    pa_source_output_unref(u->source_output);
    u->source_output = NULL;
}

/* These callbacks do not have any relevance as long as we apply
   PA_SOURCE_OUTPUT_DONT_MOVE flag */
/* Called from main context */
static void cmtspeech_source_output_moving_cb(pa_source_output *o, pa_source *dest) {
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_assert_se(u = o->userdata);

    u->source = o->source;

    pa_log_debug("CMT Source output moving to %s", dest ? dest->name : "(null)");
}

/* Called from main context */
static pa_bool_t cmtspeech_source_output_may_move_to_cb(pa_source_output *o, pa_source *dest) {
  struct userdata *u;
  ENTER();

  pa_source_output_assert_ref(o);
  pa_assert_se(u = o->userdata);

  if (cmtspeech_check_source_api(dest))
      return FALSE;

  return TRUE;
}

int cmtspeech_create_source_output(struct userdata *u)
{
    pa_source_output_new_data data;
    char t[256];

    pa_assert(u);
    ENTER();

    if (u->source_output) {
        pa_log_warn("Create called but output already exists");
        return 1;
    }

    pa_source_output_new_data_init(&data);
    data.driver = __FILE__;
    data.module = u->module;
    data.source = u->source;
    snprintf(t, sizeof(t), "Cellular call up link");
    pa_proplist_sets(data.proplist, PA_PROP_MEDIA_NAME, t);
    snprintf(t, sizeof(t), "phone");
    pa_proplist_sets(data.proplist, PA_PROP_MEDIA_ROLE, t);
    snprintf(t, sizeof(t), "cmtspeech module");
    pa_proplist_sets(data.proplist, PA_PROP_APPLICATION_NAME, t);
    pa_source_output_new_data_set_sample_spec(&data, &u->ss);
    pa_source_output_new_data_set_channel_map(&data, &u->map);

    pa_source_output_new(&u->source_output, u->core, &data, PA_SOURCE_OUTPUT_DONT_MOVE|PA_SOURCE_OUTPUT_START_CORKED);
    pa_source_output_new_data_done(&data);

    if (!u->source_output) {
        pa_log("Creating cmtspeech source output failed");
        return -1;
    }

    u->source_output->push = cmtspeech_source_output_push_cb;
    u->source_output->kill = cmtspeech_source_output_kill_cb;
    u->source_output->attach = cmtspeech_source_output_attach_cb;
    u->source_output->detach = cmtspeech_source_output_detach_cb;
    u->source_output->moving = cmtspeech_source_output_moving_cb;
    u->source_output->state_change = cmtspeech_source_output_state_change_cb;
    u->source_output->may_move_to = cmtspeech_source_output_may_move_to_cb;
    u->source_output->userdata = u;

    pa_source_output_put(u->source_output);

    return 0;
}

void cmtspeech_delete_source_output(struct userdata *u) {
    pa_assert(u);
    ENTER();

    if (!u->source_output) {
        pa_log_warn("Delete called but no source output exists");
        return;
    }

    pa_source_output_unlink(u->source_output);
    pa_source_output_unref(u->source_output);
    u->source_output = NULL;
}
