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

#include "voice-mainloop-handler.h"
#include "voice-hw-sink-input.h"
#include "voice-hw-source-output.h"
#include "voice-cs-call-sink-input.h"
#include "voice-util.h"

static PA_DEFINE_CHECK_TYPE(voice_mainloop_handler, pa_msgobject);

static void handle_execute_message(struct userdata *u, voice_mainloop_handler_execute *e) {
    pa_assert(u);
    pa_assert(e);
    pa_assert(e->execute);
    e->execute(u, e->parameter);
}

static void mainloop_handler_free(pa_object *o) {
    voice_mainloop_handler *h = VOICE_MAINLOOP_HANDLER(o);
    pa_log_info("Free called");
    pa_xfree(h);
}

static int mainloop_handler_process_msg(pa_msgobject *o, int code, void *userdata, int64_t offset, pa_memchunk *chunk) {
    voice_mainloop_handler *h = VOICE_MAINLOOP_HANDLER(o);
    struct userdata *u;
    pa_source *source;

    voice_mainloop_handler_assert_ref(h);
    pa_assert_se(u = h->u);

    switch (code) {

        /* This is here only as on example. If you need to delegate something
           to maithread from IO-thread, add a new message. */
    case VOICE_MAINLOOP_HANDLER_EXECUTE:
        pa_log_debug("Got execute message !");
        handle_execute_message(h->u, (voice_mainloop_handler_execute *) userdata);
        return 0;

    case VOICE_MAINLOOP_HANDLER_BUFFERS_ALTERNATIVE:
        pa_log_debug("VOICE_MAINLOOP_HANDLER_BUFFERS_ALTERNATIVE");
        source = voice_get_original_master_source(u);
        if (source)
        {
            pa_proplist_sets(source->proplist,"x-maemo.alsa_source.buffers","alternative");
            pa_hook_fire(&u->core->hooks[PA_CORE_HOOK_SOURCE_PROPLIST_CHANGED],source);
        }
        return 0;

    case VOICE_MAINLOOP_HANDLER_BUFFERS_PRIMARY:
        pa_log_debug("VOICE_MAINLOOP_HANDLER_BUFFERS_PRIMARY");
        source = voice_get_original_master_source(u);
	if (source)
	{
            pa_proplist_sets(source->proplist,"x-maemo.alsa_source.buffers","primary");
            pa_hook_fire(&u->core->hooks[PA_CORE_HOOK_SOURCE_PROPLIST_CHANGED],source);
        }
        return 0;

    case VOICE_MAINLOOP_HANDLER_CMT_UL_STATE_CHANGE:
        pa_log_debug("VOICE_MAINLOOP_HANDLER_CMT_UL_STATE_CHANGE");
        voice_source_set_state(u->voip_source,u->raw_source,u->voip_source->state);
        return 0;

    case VOICE_MAINLOOP_HANDLER_CMT_DL_STATE_CHANGE:
        pa_log_debug("VOICE_MAINLOOP_HANDLER_CMT_DL_STATE_CHANGE");
        voice_sink_set_state(u->voip_sink,u->raw_sink,u->voip_sink->state);
        if (pa_atomic_load(&u->cmt_connection.dl_state) != 1 || u->cs_call_sink_input )
        {
            if (pa_atomic_load(&u->cmt_connection.dl_state) != 1 && u->cs_call_sink_input )
            {
                voice_delete_cs_call_sink_input(u);
            }
        }
        else
        {
            voice_create_cs_call_sink_input(u);
        }
        return 0;

    case VOICE_MAINLOOP_HANDLER_TEMPERATURE_START:
        pa_log_debug("VOICE_MAINLOOP_HANDLER_TEMPERATURE_START");
        voice_temperature_state(u);
        return 0;

    case VOICE_MAINLOOP_HANDLER_MESSAGE_MAX:
    default:
        pa_log_error("Unknown message code %d", code);
        return -1;
    }
}

pa_msgobject *voice_mainloop_handler_new(struct userdata *u) {
    voice_mainloop_handler *h;
    pa_assert(u);
    pa_assert(u->core);
    pa_assert_se(h = pa_msgobject_new(voice_mainloop_handler));
    h->parent.parent.free = mainloop_handler_free;
    h->parent.process_msg = mainloop_handler_process_msg;
    h->u = u;
    return (pa_msgobject *)h;
}
