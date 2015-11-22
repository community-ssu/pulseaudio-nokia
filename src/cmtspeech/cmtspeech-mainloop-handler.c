/*
 * This file is part of pulseaudio-nokia
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

#include "cmtspeech-mainloop-handler.h"
#include <pulsecore/namereg.h>
#include "proplist-nokia.h"

PA_DEFINE_CHECK_TYPE(cmtspeech_mainloop_handler, pa_msgobject);

static void mainloop_handler_free(pa_object *o) {
    cmtspeech_mainloop_handler *h = CMTSPEECH_MAINLOOP_HANDLER(o);
    pa_log_info("Free called");
    pa_xfree(h);
}

#define PA_ALSA_SOURCE_PROP_BUFFERS "x-maemo.alsa_source.buffers"
#define PA_ALSA_PROP_BUFFERS_PRIMARY "primary"
#define PA_ALSA_PROP_BUFFERS_ALTERNATIVE "alternative"

static void set_source_buffer_mode(struct userdata *u, const char *mode) {
    pa_source *om_source;
    pa_assert(u);

    om_source = pa_namereg_get(u->module->core, "source.hw0", PA_NAMEREG_SOURCE);

    if (om_source) {
        pa_log_debug("Setting property %s for %s to %s", PA_ALSA_SOURCE_PROP_BUFFERS,
                     om_source->name, mode);
        pa_proplist_sets(om_source->proplist, PA_ALSA_SOURCE_PROP_BUFFERS, mode);
        pa_hook_fire(&u->core->hooks[PA_CORE_HOOK_SOURCE_PROPLIST_CHANGED], om_source);
    }
    else
        pa_log_warn("Source not found");
}

static int mainloop_handler_process_msg(pa_msgobject *o, int code, void *userdata, int64_t offset, pa_memchunk *chunk) {
    cmtspeech_mainloop_handler *h = CMTSPEECH_MAINLOOP_HANDLER(o);
    struct userdata *u;

    cmtspeech_mainloop_handler_assert_ref(h);
    pa_assert_se(u = h->u);

    switch (code) {

    case CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_CONNECT:
        pa_log_debug("Handling CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_CONNECT");
        if (u->source_output && PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->state)) {
            set_source_buffer_mode(u, PA_ALSA_PROP_BUFFERS_ALTERNATIVE);
            if (u->source_output->state == PA_SOURCE_OUTPUT_RUNNING)
                pa_log_warn("UL_CONNECT: Source output is already running");
            else
                pa_source_output_cork(u->source_output, FALSE);
        }
        else
            pa_log_warn("UL_CONNECT: Source output not there anymore (state = %d)",
                        u->source_output ? (int) u->source_output->state : -1);
        return 0;

    case CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_DISCONNECT:
        pa_log_debug("Handling CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_DISCONNECT");
        if (u->source_output && PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->state)) {
            set_source_buffer_mode(u, PA_ALSA_PROP_BUFFERS_PRIMARY);
            if (u->source_output->state == PA_SOURCE_OUTPUT_CORKED)
                pa_log_warn("UL_DISCONNECT: Source output is already corked");
            else
                pa_source_output_cork(u->source_output, TRUE);
        }
        else
            pa_log_warn("UL_DISCONNECT: Source output not there anymore");
        return 0;

    case CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_CONNECT:
        pa_log_debug("Handling CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_CONNECT");
        if (u->sink_input && PA_SINK_INPUT_IS_LINKED(u->sink_input->state)) {
            if (u->sink_input->state == PA_SINK_INPUT_RUNNING)
                pa_log_warn("DL_CONNECT: Sink input is already running");
            else
                pa_sink_input_cork(u->sink_input, FALSE);
        }
        else
            pa_log_warn("DL_CONNECT: Sink input not there anymore");
        return 0;

    case CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_DISCONNECT:
        pa_log_debug("Handling CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_DISCONNECT");
        if (u->sink_input && PA_SINK_INPUT_IS_LINKED(u->sink_input->state)) {
                if (u->sink_input->state == PA_SINK_INPUT_CORKED)
                    pa_log_warn("DL_DISCONNECT: Sink input is already corked");
                else
                    pa_sink_input_cork(u->sink_input, TRUE);
        }
        else
            pa_log_warn("DL_DISCONNECT: Sink input not there anymore");
        return 0;

   default:
        pa_log_error("Unknown message code %d", code);
        return -1;
    }
}

pa_msgobject *cmtspeech_mainloop_handler_new(struct userdata *u) {
    cmtspeech_mainloop_handler *h;
    pa_assert(u);
    pa_assert(u->core);
    pa_assert_se(h = pa_msgobject_new(cmtspeech_mainloop_handler));
    h->parent.parent.free = mainloop_handler_free;
    h->parent.process_msg = mainloop_handler_process_msg;
    h->u = u;
    return (pa_msgobject *)h;
}
