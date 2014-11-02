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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <pulse/volume.h>
#include <pulse/xmalloc.h>
#include <pulse/proplist.h>

#include <pulsecore/sink-input.h>
#include <pulsecore/module.h>
#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>
#include <pulsecore/log.h>
#include <pulsecore/mutex.h>
#include <pulsecore/atomic.h>
#include <pulsecore/thread.h>
#include <pulsecore/sample-util.h>

#include "eap_memory.h"
#include "eap_multiband_drc_int32.h"
#include "eap_multiband_drc_control_int32.h"
#include "drc.h"
#include "module-nokia-music-symdef.h"
#include "optimized.h"
#include "memory.h"

PA_MODULE_AUTHOR("Jyri Sarha");
PA_MODULE_DESCRIPTION("Nokia music module");
PA_MODULE_USAGE("master_sink=<sink to connect to> "
                "sink_name=<name of created sink>");
PA_MODULE_VERSION(PACKAGE_VERSION);

static const char* const valid_modargs[] = {
  "master_sink",
  "sink_name",
  NULL,
};

#define SAMPLE_RATE_HW_HZ (48000)
#define PROPLIST_SINK "sink.hw0"

struct userdata {
  pa_core *core;
  pa_module *module;

  size_t window_size;

  pa_sink *master_sink;
  pa_sink *sink;

  pa_sink_input *sink_input;
  pa_memchunk silence_memchunk;

  mumdrc_userdata_t mudrc;
  char usedrc : 1;
  pa_hook_slot *hook_slot;
  float mudrc_volume;
};

static pa_hook_result_t sink_proplist_changed_hook_callback(pa_core *c, pa_sink *s, struct userdata *u) {
  const char *proplist = pa_proplist_gets(s->proplist, "x-maemo.mumdrc.dl");
  if (proplist) {
    u->usedrc = pa_parse_boolean(proplist);
  }
  const void *data;
  size_t nbytes;
  if (!pa_proplist_get(s->proplist, "x-maemo.mumdrc.dl.parameters", &data, &nbytes)) {
    mumdrc_write_parameters(u->mudrc.drc, data, nbytes);
  }
  if (!pa_proplist_get(s->proplist, "x-maemo.limiter.dl.parameters", &data, &nbytes)) {
    limiter_write_parameters(u->mudrc.drc, data, nbytes);
  }
  return PA_HOOK_OK;
}

static pa_cvolume* pa_cvolume_merge(pa_cvolume *dest, const pa_cvolume *a, const pa_cvolume *b) {
    unsigned i;

    pa_assert(dest);
    pa_assert(a);
    pa_assert(b);

    pa_return_val_if_fail(pa_cvolume_valid(a), NULL);
    pa_return_val_if_fail(pa_cvolume_valid(b), NULL);

    for (i = 0; i < a->channels && i < b->channels; i++)
        dest->values[i] = PA_MAX(a->values[i], b->values[i]);

    dest->channels = (uint8_t) i;

    return dest;
}

static void get_max_input_volume(pa_sink *s, pa_cvolume *max_volume, const pa_channel_map *channel_map) {
  pa_sink_input *i;
  uint32_t idx;
  pa_assert(max_volume);
  pa_sink_assert_ref(s);

  PA_IDXSET_FOREACH(i, s->inputs, idx) {
    pa_cvolume remapped_volume;

    if (i->origin_sink) {
      /* go recursively on slaved flatten sink
             * and ignore this intermediate sink-input. (This is not really needed) */
      get_max_input_volume(i->origin_sink, max_volume, channel_map);
      continue;
    }

    memcpy(&remapped_volume, &i->virtual_volume, sizeof(remapped_volume));
    pa_cvolume_remap(&remapped_volume, &i->channel_map, channel_map);
    pa_cvolume_merge(max_volume, max_volume, &remapped_volume);
  }
}

static void update_mdrc_volume(struct userdata *u) {
  pa_cvolume max_input_volume;
  pa_assert(u);

  pa_cvolume_mute(&max_input_volume, u->sink->channel_map.channels);
  get_max_input_volume(u->sink, &max_input_volume, &u->sink->channel_map);

  u->mudrc_volume = pa_sw_volume_to_dB(pa_cvolume_avg(&max_input_volume));
  if (u->mudrc_volume > -10.1)
  {
    pa_log_debug("MDRC volume changing to %f", u->mudrc_volume);
    set_drc_volume(&u->mudrc, u->mudrc_volume);
  }
}

/*** sink callbacks ***/

/* Called from I/O thread context */
static int sink_process_msg(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk) {
  struct userdata *u = PA_SINK(o)->userdata;

  switch (code) {

  case PA_SINK_MESSAGE_GET_LATENCY: {
    pa_usec_t usec = 0;

    if ((u->master_sink == NULL) || PA_MSGOBJECT(u->master_sink)->process_msg(
          PA_MSGOBJECT(u->master_sink), PA_SINK_MESSAGE_GET_LATENCY, &usec, 0, NULL) < 0)
      usec = 0;

    *((pa_usec_t*) data) = usec;
    return 0;
  }
  case PA_SINK_MESSAGE_SET_VOLUME: {
    update_mdrc_volume(u);
    // Pass trough to pa_sink_process_msg
    break;
  }
  case PA_SINK_MESSAGE_ADD_INPUT: {
    pa_sink_input *i = PA_SINK_INPUT(data);
    pa_assert(i != u->sink_input);
    // Pass trough to pa_sink_process_msg
    break;
  }

  }

  return pa_sink_process_msg(o, code, data, offset, chunk);
}

/* Called from main context */
static int sink_set_state(pa_sink *s, pa_sink_state_t state) {
  struct userdata *u;

  pa_sink_assert_ref(s);
  pa_assert_se(u = s->userdata);

  if (PA_SINK_IS_LINKED(state) && u->sink_input && PA_SINK_INPUT_IS_LINKED(pa_sink_input_get_state(u->sink_input)))
    pa_sink_input_cork(u->sink_input, state == PA_SINK_SUSPENDED);

  pa_log_debug("sink_set_state() called with %d", state);
  return 0;
}

/* Called from I/O thread context */
static void sink_request_rewind(pa_sink *s) {
  struct userdata *u;

  pa_sink_assert_ref(s);
  pa_assert_se(u = s->userdata);

  /* Just hand this one over to the master sink */
  pa_sink_input_request_rewind(u->sink_input, s->thread_info.rewind_nbytes, TRUE, FALSE, FALSE);

}

/* Called from I/O thread context */
static void sink_update_requested_latency(pa_sink *s) {
  struct userdata *u;

  pa_sink_assert_ref(s);
  pa_assert_se(u = s->userdata);

  /* Just hand this one over to the master sink */
  pa_sink_input_set_requested_latency_within_thread(
        u->sink_input,
        pa_sink_get_requested_latency_within_thread(s));
}

/*** sink_input callbacks ***/
static int sink_input_pop_cb(pa_sink_input *i, size_t length, pa_memchunk *chunk) {
//  assert(0); //possibly incorrect, needs verification against Fremantle function at 3E5C, the blah & 1 checks are for "use stereo widening" and the blah & 2 checks are for "use drc"
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  u = i->userdata;
  pa_assert(chunk && u);

  if (u->sink->thread_info.rewind_requested)
    pa_sink_process_rewind(u->sink, 0);

  if (!PA_SINK_IS_OPENED(u->sink->thread_info.state)) {
    /* There are no clients playing to music sink,
     let's just cut out silence and be done with it. */
    pa_silence_memchunk_get(&u->core->silence_cache,
                            u->core->mempool,
                            chunk,
                            &i->sample_spec,
                            length);
  } else {
    pa_sink_render_full(u->sink, u->window_size, chunk);
  }

  if (u->usedrc && u->mudrc_volume > -10.1) {
    pa_memblock *left_stw_block = pa_memblock_new(i->sink->core->mempool, chunk->length >> 1);
    pa_memblock *right_stw_block = pa_memblock_new(i->sink->core->mempool, chunk->length >> 1);
    pa_memblock *left_drc_block = pa_memblock_new(i->sink->core->mempool, chunk->length);
    pa_memblock *right_drc_block = pa_memblock_new(i->sink->core->mempool, chunk->length);
    short *data = (short *)((char *)pa_memblock_acquire(chunk->memblock) + (chunk->index & (~1)));

    short *stw_data[2];

    stw_data[0] = (short *) pa_memblock_acquire(left_stw_block);
    stw_data[1] = (short *) pa_memblock_acquire(right_stw_block);

    int32_t *left_drc_data = (int32_t *) pa_memblock_acquire(left_drc_block);
    int32_t *right_drc_data = (int32_t *) pa_memblock_acquire(right_drc_block);

    int len = chunk->length;
    int n = len >> 1;

    deinterleave_stereo_to_mono(data, stw_data, n);

    if (u->usedrc && u->mudrc_volume > -10.1) {
      move_16bit_to_32bit(left_drc_data, stw_data[0], n);
      move_16bit_to_32bit(right_drc_data, stw_data[1], n);
      mudrc_process(&u->mudrc, left_drc_data, right_drc_data, left_drc_data, right_drc_data, n);
      move_32bit_to_16bit(stw_data[0], left_drc_data, n);
      move_32bit_to_16bit(stw_data[1], right_drc_data, n);
    }

    interleave_mono_to_stereo(stw_data, right_drc_data, n);

    pa_memblock_release(left_stw_block);
    pa_memblock_release(right_stw_block);
    pa_memblock_release(left_drc_block);
    pa_memblock_release(right_drc_block);
    pa_memblock_release(chunk->memblock);
    pa_memblock_unref(left_stw_block);
    pa_memblock_unref(right_stw_block);
    pa_memblock_unref(left_drc_block);
    pa_memblock_unref(chunk->memblock);

    chunk->memblock = right_drc_block;
    chunk->length = len;
    chunk->index = 0;
  }

  return 0;
}

/* Called from I/O thread context */
static void sink_input_process_rewind_cb(pa_sink_input *i, size_t nbytes) {
  struct userdata *u;
  size_t amount = 0;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);

  if (!u->sink || !PA_SINK_IS_OPENED(u->sink->thread_info.state))
    return;

  if (u->sink->thread_info.rewind_nbytes > 0) {

    amount = PA_MIN(u->sink->thread_info.rewind_nbytes, nbytes);
    u->sink->thread_info.rewind_nbytes = 0;
  }

  pa_sink_process_rewind(u->sink, amount);
}

/* Called from I/O thread context */
static void sink_input_update_max_rewind_cb(pa_sink_input *i, size_t nbytes) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);

  if (!u->sink || !PA_SINK_IS_LINKED(u->sink->thread_info.state))
    return;

  pa_sink_set_max_rewind_within_thread(u->sink, nbytes);
}

/* Called from I/O thread context */
static void sink_input_update_max_request_cb(pa_sink_input *i, size_t nbytes) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);

  if (!u->sink || !PA_SINK_IS_LINKED(u->sink->thread_info.state))
    return;

  pa_sink_set_max_request_within_thread(u->sink, nbytes);
}

/* Called from I/O thread context */
static void sink_input_update_sink_latency_range_cb(pa_sink_input *i) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);

  if (!u->sink || !PA_SINK_IS_LINKED(u->sink->thread_info.state))
    return;

  pa_sink_set_latency_range_within_thread(u->sink, i->sink->thread_info.min_latency, i->sink->thread_info.max_latency);
}

static void sink_inputs_may_move(pa_sink *s, pa_bool_t move) {
  pa_sink_input *i;
  uint32_t idx;

  for (i = PA_SINK_INPUT(pa_idxset_first(s->inputs, &idx)); i; i = PA_SINK_INPUT(pa_idxset_next(s->inputs, &idx))) {
    if (move)
      i->flags &= ~PA_SINK_INPUT_DONT_MOVE;
    else
      i->flags |= PA_SINK_INPUT_DONT_MOVE;
  }
}


/* Called from I/O thread context */
static void sink_input_detach_cb(pa_sink_input *i) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);
  u->master_sink = 0;
  pa_proplist *p = pa_proplist_new();
  pa_proplist_setf(p, "device.description", "%s (not connected)", u->sink->name);
  pa_proplist_sets(p, "device.master_device", "(null)");
  pa_sink_update_proplist(u->sink, PA_UPDATE_REPLACE, p);
  pa_proplist_free(p);

  if (PA_SINK_IS_LINKED(u->sink->thread_info.state))
    pa_sink_detach_within_thread(u->sink);
  else
    pa_log("fixme: !PA_SINK_IS_LINKED ?");

  /* XXX: _set_asyncmsgq() shouldn't be called while the IO thread is
     * running, but we "have to" (ie. no better way to handle this has been
     * figured out). This call is one of the reasons for that we had to comment
     * out the assertion from pa_sink_set_asyncmsgq() that checks that the call
     * is done from the main thread. */
  pa_sink_set_asyncmsgq(u->sink, NULL);

  pa_sink_set_rtpoll(u->sink, NULL);
  sink_inputs_may_move(u->sink, FALSE);
}

/* Called from I/O thread context */
static void sink_input_attach_cb(pa_sink_input *i) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);

  if (!u->sink || !PA_SINK_IS_LINKED(u->sink->thread_info.state))
    return;

  /* XXX: _set_asyncmsgq() shouldn't be called while the IO thread is
     * running, but we "have to" (ie. no better way to handle this has been
     * figured out). This call is one of the reasons for that we had to comment
     * out the assertion from pa_sink_set_asyncmsgq() that checks that the call
     * is done from the main thread. */
  u->master_sink = i->sink;
  pa_proplist *p = pa_proplist_new();
  pa_proplist_setf(p, "device.description", "%s connected to %s", u->sink->name, i->sink->name);
  pa_proplist_sets(p, "device.master_device", i->sink->name);
  pa_sink_update_proplist(u->sink, PA_UPDATE_REPLACE, p);
  pa_proplist_free(p);
  u->sink->flat_volume_sink = i->sink;
  pa_sink_set_asyncmsgq(u->sink, i->sink->asyncmsgq);

  sink_inputs_may_move(u->sink, TRUE);
  pa_sink_set_rtpoll(u->sink, i->sink->rtpoll);
  pa_sink_attach_within_thread(u->sink);

  pa_sink_set_latency_range_within_thread(u->sink, i->sink->thread_info.min_latency, i->sink->thread_info.max_latency);
  pa_sink_set_max_rewind_within_thread(u->sink, i->sink->thread_info.max_rewind);
}

/* Called from main context */
static void sink_input_kill_cb(pa_sink_input *i) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);

  pa_sink_unlink(u->sink);

  /* FIXME: this is sort-of understandable with the may_move hack... we avoid abort in free() here */
  u->sink_input->thread_info.attached = FALSE;
  pa_sink_input_unlink(u->sink_input);

  pa_sink_unref(u->sink);
  u->sink = NULL;
  pa_sink_input_unref(u->sink_input);
  u->sink_input = NULL;

  pa_module_unload_request(u->module, TRUE);
}

/* Called from IO thread context */
static void sink_input_state_change_cb(pa_sink_input *i, pa_sink_input_state_t state) {
  struct userdata *u;

  pa_sink_input_assert_ref(i);
  pa_assert_se(u = i->userdata);
}

int pa__init(pa_module*m) {
  pa_modargs *ma = NULL;
  struct userdata *u;
  const char *sink_name, *master_sink_name;
  pa_sink *master_sink;
  pa_sample_spec ss;
  pa_channel_map map;
  char t[256];
  pa_sink_input_new_data sink_input_data;
  pa_sink_new_data sink_data;
  u = pa_xnew0(struct userdata, 1);

  pa_assert(m);

  mudrc_init(&u->mudrc, 3840, 48000.0);
  if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
    pa_log("Failed to parse module arguments");
    goto fail;
  }

  sink_name = pa_modargs_get_value(ma, "sink_name", NULL);
  master_sink_name = pa_modargs_get_value(ma, "master_sink", NULL);

  pa_log_debug("Got arguments: sink_name=\"%s\" master_sink=\"%s\".",
               sink_name, master_sink_name);

  if (!(master_sink = pa_namereg_get(m->core, master_sink_name, PA_NAMEREG_SINK))) {
    pa_log("Master sink \"%s\" not found", master_sink_name);
    goto fail;
  }

  /*
      ss = master_sink->sample_spec;
      map = master_sink->channel_map;
    */

  ss.format = PA_SAMPLE_S16LE;

  //ss.format = master_sink->sample_spec.format;
  ss.rate = SAMPLE_RATE_HW_HZ;
  ss.channels = 2;
  pa_channel_map_init_stereo(&map);

  m->userdata = u;
  u->core = m->core;
  u->usedrc = 0;
  u->module = m;
  // The result is rounded down incorrectly thus 5001...
  // 5000 us = 5 ms
  // 20000 us = 20 ms
  u->window_size = pa_usec_to_bytes(20001, &ss);
  //u->window_size = 160;
  //u->window_size = 960;
  pa_log_debug("window size: %d frame size: %d",  u->window_size, pa_frame_size(&ss));
  u->master_sink = master_sink;
  u->sink = NULL;
  u->sink_input = NULL;
  pa_silence_memchunk_get(&u->core->silence_cache,
                          u->core->mempool,
                          &u->silence_memchunk,
                          &ss,
                          u->window_size);

  pa_sink_new_data_init(&sink_data);
  sink_data.module = m;
  sink_data.driver = __FILE__;
  sink_data.flat_volume_sink = master_sink;
  pa_sink_new_data_set_name(&sink_data, sink_name);
  pa_sink_new_data_set_sample_spec(&sink_data, &ss);
  pa_sink_new_data_set_channel_map(&sink_data, &map);
  pa_proplist_setf(sink_data.proplist, PA_PROP_DEVICE_DESCRIPTION, "%s connected to %s", sink_name, master_sink->name);
  pa_proplist_sets(sink_data.proplist, PA_PROP_DEVICE_MASTER_DEVICE, master_sink->name);
  pa_proplist_sets(sink_data.proplist, "module-suspend-on-idle.timeout", "1");

  /* Create sink */
  u->sink = pa_sink_new(m->core, &sink_data, PA_SINK_LATENCY);
  pa_sink_new_data_done(&sink_data);
  if (!u->sink) {
    pa_log("Failed to create sink.");
    goto fail;
  }

  u->sink->parent.process_msg = sink_process_msg;
  u->sink->set_state = sink_set_state;
  u->sink->update_requested_latency = sink_update_requested_latency;
  u->sink->request_rewind = sink_request_rewind;
  u->sink->userdata = u;
  u->sink->flags = PA_SINK_LATENCY;
  pa_memblock_ref(u->silence_memchunk.memblock);
  u->sink->silence = u->silence_memchunk;

  pa_sink_set_asyncmsgq(u->sink, u->master_sink->asyncmsgq);
  pa_sink_set_rtpoll(u->sink, u->master_sink->rtpoll);

  pa_sink_input_new_data_init(&sink_input_data);
  snprintf(t, sizeof(t), "output of %s", sink_name);
  pa_proplist_sets(sink_input_data.proplist, PA_PROP_MEDIA_NAME, t);
  pa_proplist_sets(sink_input_data.proplist, PA_PROP_APPLICATION_NAME, t); /* this is the default value used by PA modules */
  sink_input_data.sink = master_sink;
  sink_input_data.driver = __FILE__;
  sink_input_data.module = m;
  sink_input_data.origin_sink = u->sink;
  pa_sink_input_new_data_set_sample_spec(&sink_input_data, &ss);
  pa_sink_input_new_data_set_channel_map(&sink_input_data, &map);

  pa_sink_input_new(&u->sink_input, m->core, &sink_input_data, 0);
  pa_sink_input_new_data_done(&sink_input_data);
  if (!u->sink_input) {
    pa_log("Failed to create sink input.");
    goto fail;
  }

  u->sink_input->pop = sink_input_pop_cb;
  u->sink_input->process_rewind = sink_input_process_rewind_cb;
  u->sink_input->update_max_rewind = sink_input_update_max_rewind_cb;
  u->sink_input->update_max_request = sink_input_update_max_request_cb;
  u->sink_input->update_sink_latency_range = sink_input_update_sink_latency_range_cb;
  u->sink_input->kill = sink_input_kill_cb;
  u->sink_input->attach = sink_input_attach_cb;
  u->sink_input->detach = sink_input_detach_cb;
  u->sink_input->state_change = sink_input_state_change_cb;
  u->sink_input->userdata = u;

  pa_sink_put(u->sink);
  pa_sink_input_put(u->sink_input);

  pa_modargs_free(ma);
  void *res = pa_namereg_get(u->core, PROPLIST_SINK, PA_NAMEREG_SINK);
  if (res) {
    sink_proplist_changed_hook_callback(u->core, res, u);
  }
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

  if (u->hook_slot) {
    pa_hook_slot_free(u->hook_slot);
  }

  if (u->sink_input) {
    pa_sink_input_unlink(u->sink_input);
    pa_sink_input_unref(u->sink_input);
  }

  if (u->sink) {
    pa_sink_unlink(u->sink);
    pa_sink_unref(u->sink);
  }

  if (u->silence_memchunk.memblock)
    pa_memblock_unref(u->silence_memchunk.memblock);
  mudrc_deinit(&u->mudrc);
  pa_xfree(u);
}
