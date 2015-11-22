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

#include "module-nokia-cmtspeech-symdef.h"
#include "module-nokia-cmtspeech.h"
#include "module-voice-api.h"
#include "cmtspeech-connection.h"
#include "cmtspeech-dbus.h"
#include "cmtspeech-source-output.h"
#include "cmtspeech-sink-input.h"

#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>
#include <pulsecore/msgobject.h>

PA_MODULE_AUTHOR("Jyri Sarha");
PA_MODULE_DESCRIPTION("Nokia cmtspeech module");
PA_MODULE_USAGE(
    "sink=<sink to connect to> "
    "source=<source to connect to> "
    "dbus_type=<defaults to session> "
);
PA_MODULE_VERSION(PACKAGE_VERSION);

static const char* const valid_modargs[] = {
    "sink",
    "source",
    "dbus_type",
    NULL,
};

/* TODO: Make sure policy triggers UL buffer switching */

int cmtspeech_check_sink_api(pa_sink *s) {
    if (strcmp(PA_PROP_SINK_API_EXTENSION_PROPERTY_VALUE,
                pa_strnull(pa_proplist_gets(s->proplist, PA_PROP_SINK_API_EXTENSION_PROPERTY_NAME)))) {
        pa_log_error("Sink \"%s\" does not support %s version %s", s->name,
                     PA_PROP_SINK_API_EXTENSION_PROPERTY_NAME,
                     PA_PROP_SINK_API_EXTENSION_PROPERTY_VALUE);
        pa_log_debug("'%s' != '%s'", PA_PROP_SINK_API_EXTENSION_PROPERTY_VALUE,
                     pa_strnull(pa_proplist_gets(s->proplist, PA_PROP_SINK_API_EXTENSION_PROPERTY_NAME)));
        return -1;
    }
    return 0;
}

int cmtspeech_check_source_api(pa_source *s) {
    if (strcmp(PA_PROP_SOURCE_API_EXTENSION_PROPERTY_VALUE,
                pa_strnull(pa_proplist_gets(s->proplist, PA_PROP_SOURCE_API_EXTENSION_PROPERTY_NAME)))) {
        pa_log_error("Source \"%s\" does not support %s version %s", s->name,
                     PA_PROP_SOURCE_API_EXTENSION_PROPERTY_NAME,
                     PA_PROP_SOURCE_API_EXTENSION_PROPERTY_VALUE);
        pa_log_debug("'%s' != '%s'", PA_PROP_SOURCE_API_EXTENSION_PROPERTY_VALUE,
                     pa_strnull(pa_proplist_gets(s->proplist, PA_PROP_SOURCE_API_EXTENSION_PROPERTY_NAME)));
        return -1;
    }
    return 0;
}

static void cmtspeech_unload_defer_cb(pa_mainloop_api *ma, pa_defer_event *de, void *userdata) {
    pa_module *m;
    pa_assert_se(m = (pa_module *) userdata);

    pa_module_unload(m->core, m, TRUE);
}

void cmtspeech_trigger_unload(struct userdata *u) {

    if (u->unload_de) {
        pa_log_info("Unload already triggered");
        return;
    }

    u->unload_de = u->core->mainloop->defer_new(u->core->mainloop, cmtspeech_unload_defer_cb, u->module);
}

int pa__init(pa_module*m) {
    pa_modargs *ma = NULL;
    struct userdata *u;
    const char *sink_name, *source_name, *dbus_type;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log_error("Failed to parse module arguments");
        goto fail;
    }

    sink_name = pa_modargs_get_value(ma, "sink", NULL);
    source_name = pa_modargs_get_value(ma, "source", NULL);
    dbus_type = pa_modargs_get_value(ma, "dbus_type", "session");

    pa_log_debug("Got arguments: sink=\"%s\" source=\"%s\" dbus_type=\"%s\"",
                 sink_name, source_name, dbus_type);

    u = pa_xnew0(struct userdata, 1);
    m->userdata = u;
    u->core = m->core;
    u->module = m;

    u->ss.format = PA_SAMPLE_S16NE;
    u->ss.rate = CMTSPEECH_SAMPLERATE;
    u->ss.channels = 1;
    pa_channel_map_init_mono(&u->map);
    /* The result is rounded down incorrectly thus +1 */
    u->dl_frame_size = pa_usec_to_bytes(VOICE_SINK_FRAMESIZE+1, &u->ss);
    u->ul_frame_size = pa_usec_to_bytes(VOICE_SOURCE_FRAMESIZE+1, &u->ss);

    if (!(u->source = pa_namereg_get(m->core, source_name, PA_NAMEREG_SOURCE))) {
        pa_log_error("Source \"%s\" not found", source_name);
        goto fail;
    }

    if (!(u->sink = pa_namereg_get(m->core, sink_name, PA_NAMEREG_SINK))) {
        pa_log_error("Sink \"%s\" not found", sink_name);
        goto fail;
    }

    if (cmtspeech_check_source_api(u->source))
        goto fail;

    if (cmtspeech_check_sink_api(u->sink))
        goto fail;

    u->sink_input = NULL;
    u->source_output = NULL;

    u->local_sideinfoq = pa_queue_new();
    u->voice_sideinfoq = NULL;
    u->continuous_dl_stream = FALSE,
    u->dl_memblockq =
  pa_memblockq_new(0, 4*u->dl_frame_size, 0, pa_frame_size(&u->ss), 0, 0, 0, NULL);

    u->mainloop_handler = cmtspeech_mainloop_handler_new(u);

    if (cmtspeech_dbus_init(u, dbus_type))
        goto fail;

    if (cmtspeech_connection_init(u))
  goto fail;

    cmtspeech_create_sink_input(u);
    cmtspeech_create_source_output(u);

    return 0;
fail:
    if (ma)
        pa_modargs_free(ma);

    pa__done(m);
    return -1;
}

void pa__done(pa_module*m) {
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata))
        return;

    if (u->unload_de) {
        u->core->mainloop->defer_free(u->unload_de);
        u->unload_de = NULL;
    }

    cmtspeech_dbus_unload(u);

    cmtspeech_connection_unload(u);

    cmtspeech_delete_source_output(u);

    cmtspeech_delete_sink_input(u);

    if (u->mainloop_handler) {
        u->mainloop_handler->parent.free((pa_object *)u->mainloop_handler);
        u->mainloop_handler = NULL;
    }

    if (u->local_sideinfoq) {
        pa_queue_free(u->local_sideinfoq, NULL, NULL);
        u->local_sideinfoq = NULL;
    }

    if (u->dl_memblockq) {
        pa_memblockq_free(u->dl_memblockq);
        u->dl_memblockq = NULL;
    }

    pa_xfree(u);
}
