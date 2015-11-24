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

#include <pulse/xmalloc.h>

#include <pulsecore/sink-input.h>
#include <pulsecore/source-output.h>
#include <pulsecore/module.h>
#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>
#include <pulsecore/log.h>
#include <pulsecore/mutex.h>
#include <pulsecore/atomic.h>
#include <pulsecore/semaphore.h>
#include <pulsecore/thread.h>
#include <pulsecore/sample-util.h>

#include "module-voice-userdata.h"
#include "voice-hw-sink-input.h"
#include "voice-util.h"
#include "voice-voip-sink.h"
#include "voice-raw-sink.h"
#include "voice-aep-ear-ref.h"
#include "voice-convert.h"
#include "voice-optimized.h"
#include "memory.h"
#include "voice-voip-source.h"

#include "module-voice-api.h"

static unsigned int voice_dl_sideinfo_pop(struct userdata *u, int length) {
    unsigned int spc_flags = 0;

    pa_assert(u);
    pa_assert(length % u->aep_fragment_size == 0);

    while (length) {
        spc_flags = (unsigned int)pa_queue_pop(u->dl_sideinfo_queue);
        length -= u->aep_fragment_size;
    }

    return spc_flags & ~0x8000;
}

/* Called from IO thread context. */
//todo address 0x000140B8
static void voice_aep_sink_process(struct userdata *u, pa_memchunk *chunk) {
    unsigned int spc_flags = 0;

    pa_assert(0);
    pa_assert(u);

    if (voice_voip_sink_active_iothread(u)) {
        pa_sink_render_full(u->voip_sink, u->aep_fragment_size, chunk);
        spc_flags = voice_dl_sideinfo_pop(u, u->aep_fragment_size);
    }
    else {
        pa_silence_memchunk_get(&u->core->silence_cache,
                                u->core->mempool,
                                chunk,
                                &u->aep_sample_spec,
                                u->aep_fragment_size);
    }

    if (!pa_memblock_is_silence(chunk->memblock)) {
        aep_downlink params;

        params.chunk = chunk;
        params.spc_flags = spc_flags;
        /* TODO: get rid of cmt boolean */
        params.cmt = TRUE;

        /* TODO: I think this should be called from behind the hook */
        pa_memchunk_make_writable(chunk, u->aep_fragment_size);
/*
        pa_hook_fire(u->hooks[HOOK_AEP_DOWNLINK], &params);*/
    }
}

static void hw_sink_input_nb_ear_iir_eq_process(struct userdata *u, pa_memchunk *chunk)
{
    if (u->nb_eeq_enable)
    {
        if (!u->field_414)
        {
            short *input = (short *)pa_memblock_acquire(chunk->memblock) + chunk->index/sizeof(short);
            pa_memblock *block = pa_memblock_new(u->core->mempool, chunk->length);
            size_t len = chunk->length;
            short *output = (short *)pa_memblock_acquire(block);
            iir_eq_process_mono(u->nb_ear_iir_eq,input,output,chunk->length / 2);
            pa_memblock_release(block);
            pa_memblock_release(chunk->memblock);
            pa_memblock_unref(chunk->memblock);
            chunk->memblock = block;
            chunk->index = 0;
            chunk->length = len;
        }
    }
}

void hw_sink_input_xprot_process(struct userdata *u, pa_memchunk *chunk)
{
    if (u->xprot_enable)
    {
        if (!u->field_414)
        {
            voice_temperature_update(u);
            short *input = (short *)pa_memblock_acquire(chunk->memblock) + chunk->index/sizeof(short);
            u->xprot->stereo = 0;
            xprot_process_stereo_srcdst(u->xprot,input,0,chunk->length / 2);
            pa_memblock_release(chunk->memblock);
        }
    }
    pa_memchunk ochunk;
    voice_mono_to_stereo(u,chunk,&ochunk);
    pa_memblock_unref(chunk->memblock);
    chunk->memblock = ochunk.memblock;
    chunk->index = ochunk.index;
    chunk->length = ochunk.length;
}

/*** sink_input callbacks ***/
//todo address 0x000148EC
static int hw_sink_input_pop_cb(pa_sink_input *i, size_t length, pa_memchunk *chunk) {
    struct userdata *u;
    pa_memchunk aepchunk = { 0, 0, 0 };
    pa_memchunk rawchunk = { 0, 0, 0 };
    pa_volume_t aep_volume = PA_VOLUME_NORM;
    pa_assert(0);
    pa_assert(i);
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    pa_assert(chunk);

#ifdef SINK_TIMING_DEBUG_ON
    static struct timeval tv_last = { 0, 0 };
    struct timeval tv_new;
    pa_rtclock_get(&tv_new);
    pa_usec_t ched_period = pa_timeval_diff(&tv_last, &tv_new);
    tv_last = tv_new;
#endif

    /* We only operate with N * u->hw_fragment_size chunks. */
    if (length > u->hw_fragment_size_max)
        length = u->hw_fragment_size_max;
    else if (0 != (length % u->hw_fragment_size))
        length += u->hw_fragment_size - (length % u->hw_fragment_size);

    if (u->aep_sink_input && PA_SINK_INPUT_IS_LINKED(
      u->aep_sink_input->thread_info.state)) {
        aep_volume = u->aep_sink_input->thread_info.muted ?
            PA_VOLUME_MUTED : u->aep_sink_input->thread_info.soft_volume.values[0];
    }

    if (voice_voip_sink_active_iothread(u)) {
        if (u->voip_sink->thread_info.rewind_requested)
            pa_sink_process_rewind(u->voip_sink, 0);
        voice_aep_sink_process(u, &aepchunk);
  if (aep_volume != PA_VOLUME_MUTED && !pa_memblock_is_silence(aepchunk.memblock)) {
      if (aep_volume != PA_VOLUME_NORM) {
    pa_memchunk_make_writable(&aepchunk, 0);
    voice_apply_volume(&aepchunk, aep_volume);
      }
  }
  else if (!pa_memblock_is_silence(aepchunk.memblock)) {
      pa_memblock_unref(aepchunk.memblock);
      pa_silence_memchunk_get(&u->core->silence_cache,
            u->core->mempool,
                                    &aepchunk,
            &u->aep_sample_spec,
            aepchunk.length);
  }
        length = 2*(48/8)*aepchunk.length;
    }

    if (voice_raw_sink_active_iothread(u)) {
        if (u->raw_sink->thread_info.rewind_requested)
            pa_sink_process_rewind(u->raw_sink, 0);
  if (aepchunk.length > 0) {
            pa_sink_render_full(u->raw_sink, length, &rawchunk);
  } else {
      pa_sink_render_full(u->raw_sink, length, &rawchunk);
  }

        if (pa_atomic_load(&u->mixer_state) == PROP_MIXER_TUNING_ALT &&
            u->alt_mixer_compensation != PA_VOLUME_NORM &&
            !pa_memblock_is_silence(rawchunk.memblock)) {
            pa_memchunk_make_writable(&rawchunk, 0);
            voice_apply_volume(&rawchunk, u->alt_mixer_compensation);
        }
    }

    if (aepchunk.length > 0 && !pa_memblock_is_silence(aepchunk.memblock)) {
  if (rawchunk.length > 0 && !pa_memblock_is_silence(rawchunk.memblock)) {
#if 0
#if 1 /* Use only NB IIR EQ and down mix raw sink to mono when in a call */
      pa_memchunk monochunk, stereochunk;
      pa_hook_fire(u->hooks[HOOK_NARROWBAND_EAR_EQU_MONO], &aepchunk);
      voice_convert_run_8_to_48(u, u->aep_to_hw_sink_resampler, &aepchunk, chunk);
      voice_downmix_to_mono(u, &rawchunk, &monochunk);
      pa_memblock_unref(rawchunk.memblock);
      pa_memchunk_reset(&rawchunk);
      pa_assert(monochunk.length == chunk->length);
      voice_equal_mix_in(chunk, &monochunk);
      pa_memblock_unref(monochunk.memblock);
            pa_hook_fire(u->hooks[HOOK_XPROT_MONO], chunk);
            voice_mono_to_stereo(u, chunk, &stereochunk);
            pa_memblock_unref(chunk->memblock);
            *chunk = stereochunk;
#else /* Do full stereo processing if the raw and aep inputs are both available */
      voice_convert_run_8_to_48_stereo(u, u->aep_to_hw_sink_resampler, &aepchunk, chunk);
      pa_assert(chunk->length == rawchunk.length);
      voice_equal_mix_in(chunk, &rawchunk);
      pa_hook_fire(u->hooks[HOOK_HW_SINK_PROCESS], chunk);
#endif
#endif
  }
  else {/*
            pa_memchunk stereochunk;
            pa_hook_fire(u->hooks[HOOK_NARROWBAND_EAR_EQU_MONO], &aepchunk);
      voice_convert_run_8_to_48(u, u->aep_to_hw_sink_resampler, &aepchunk, chunk);
            pa_hook_fire(u->hooks[HOOK_XPROT_MONO], chunk);
            voice_mono_to_stereo(u, chunk, &stereochunk);
            pa_memblock_unref(chunk->memblock);
            *chunk = stereochunk;*/
  }
    }
    else if (rawchunk.length > 0 && !pa_memblock_is_silence(rawchunk.memblock)) {
  /**chunk = rawchunk;
  pa_memchunk_reset(&rawchunk);
        pa_hook_fire(u->hooks[HOOK_HW_SINK_PROCESS], chunk);*/
    }
    else {
        pa_silence_memchunk_get(&u->core->silence_cache,
                                u->core->mempool,
                                chunk,
                                &i->sample_spec,
                                length);
    }
    if (rawchunk.memblock) {
        pa_memblock_unref(rawchunk.memblock);
        pa_memchunk_reset(&rawchunk);
    }
    if (aepchunk.memblock) {
        pa_memblock_unref(aepchunk.memblock);
        pa_memchunk_reset(&aepchunk);
    }

    /* FIXME: voice_voip_source_active_iothread() should only be called from
     * the source's main thread. The solution will possibly be copying the
     * source state in some appropriate place to a variable that can be safely
     * accessed from the sink's IO thread. The fact that we run the IO threads
     * with FIFO scheduling should remove any synchronization issues between
     * two IO threads, though, but this is still wrong in principle. */

    /* FIXME: We should have a local atomic indicator to follow source side activity */
    if (voice_voip_source_active(u)) {
        pa_memchunk earref;
        if (pa_memblock_is_silence(chunk->memblock))
            pa_silence_memchunk_get(&u->core->silence_cache,
                                    u->core->mempool,
                                    &earref,
                                    &u->aep_sample_spec,
                                    chunk->length/(2*(48/8)));
        else
            voice_convert_run_48_stereo_to_8(u, u->ear_to_aep_resampler, chunk, &earref);
        voice_aep_ear_ref_dl(u, &earref);
        pa_memblock_unref(earref.memblock);
    }

#ifdef SINK_TIMING_DEBUG_ON
    pa_rtclock_get(&tv_new);
    pa_usec_t process_delay = pa_timeval_diff(&tv_last, &tv_new);
    printf("%d,%d ", (int)ched_period, (int)process_delay);
    /*
      pa_usec_t latency;
      PA_MSGOBJECT(u->master_sink)->process_msg(PA_MSGOBJECT(u->master_sink), PA_SINK_MESSAGE_GET_LATENCY, &latency, 0, NULL);
      printf("%lld ", latency);
    */
#endif

    return 0;
}

/*** sink_input callbacks ***/
//todo address 0x0001526
static int hw_sink_input_pop_8k_mono_cb(pa_sink_input *i, size_t length, pa_memchunk *chunk) {
    struct userdata *u;
    pa_bool_t have_aep_frame = 0;
    pa_bool_t have_raw_frame = 0;

    pa_assert(i);
    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    pa_assert(chunk);

#ifdef SINK_TIMING_DEBUG_ON
    static struct timeval tv_last = { 0, 0 };
    struct timeval tv_new;
    pa_rtclock_get(&tv_new);
    pa_usec_t ched_period = pa_timeval_diff(&tv_last, &tv_new);
    tv_last = tv_new;
#endif

    pa_volume_t aep_volume = PA_VOLUME_NORM;
    if (u->aep_sink_input && PA_SINK_INPUT_IS_LINKED(
      u->aep_sink_input->thread_info.state)) {
        aep_volume = u->aep_sink_input->thread_info.muted ?
            PA_VOLUME_MUTED : u->aep_sink_input->thread_info.soft_volume.values[0];
    }

    if (voice_voip_sink_active_iothread(u)) {
        pa_memchunk ichunk;
        voice_aep_sink_process(u, &ichunk);
  if (aep_volume != PA_VOLUME_MUTED) {
      *chunk = ichunk;
  }
  else {
      pa_memblock_unref(ichunk.memblock);
      pa_silence_memchunk_get(&u->core->silence_cache,
            u->core->mempool,
                                    chunk,
            &u->aep_sample_spec,
            ichunk.length);
        }
        have_aep_frame = 1;
    }

    if (voice_raw_sink_active_iothread(u)) {
        if (u->raw_sink->thread_info.rewind_requested)
            pa_sink_process_rewind(u->raw_sink, 0);
        if (have_aep_frame) {
            pa_memchunk tchunk, ichunk;
            pa_sink_render_full(u->raw_sink, 2*(48/8)*chunk->length, &tchunk);
      voice_convert_run_48_stereo_to_8(u, u->raw_sink_to_hw8khz_sink_resampler, &tchunk, &ichunk);
      pa_assert(ichunk.length == chunk->length);
      pa_memblock_unref(tchunk.memblock);
            if (!pa_memblock_is_silence(chunk->memblock)) {
                if (aep_volume == PA_VOLUME_NORM)
                    voice_equal_mix_in(&ichunk, chunk);
                else
                    voice_mix_in_with_volume(&ichunk, chunk, aep_volume);
            }
            pa_memblock_unref(chunk->memblock);
            *chunk = ichunk;
        }
        else {
      pa_memchunk ichunk;
      pa_sink_render_full(u->raw_sink, u->hw_fragment_size, &ichunk);
      voice_convert_run_48_stereo_to_8(u, u->raw_sink_to_hw8khz_sink_resampler, &ichunk, chunk);
      pa_memblock_unref(ichunk.memblock);
        }
        have_raw_frame = 1;
    } else if (have_aep_frame && aep_volume != PA_VOLUME_NORM &&
               !pa_memblock_is_silence(chunk->memblock)) {
  voice_apply_volume(chunk, aep_volume);
    }

    if (!have_raw_frame && !have_aep_frame) {
  pa_silence_memchunk_get(&u->core->silence_cache,
        u->core->mempool,
        chunk,
        &i->sample_spec,
        length);
    }

    /* FIXME: voice_voip_source_active_iothread() should only be called from
     * the source's main thread. The solution will possibly be copying the
     * source state in some appropriate place to a variable that can be safely
     * accessed from the sink's IO thread. The fact that we run the IO threads
     * with FIFO scheduling should remove any synchronization issues between
     * two IO threads, though, but this is still wrong in principle. */
    /* FIXME: We should have a local atomic indicator to follow source side activity */
    if (voice_voip_source_active_iothread(u))
        voice_aep_ear_ref_dl(u, chunk);

#ifdef SINK_TIMING_DEBUG_ON
    pa_rtclock_get(&tv_new);
    pa_usec_t process_delay = pa_timeval_diff(&tv_last, &tv_new);
    printf("%d,%d ", (int)ched_period, (int)process_delay);
    /*
      pa_usec_t latency;
      PA_MSGOBJECT(u->master_sink)->process_msg(PA_MSGOBJECT(u->master_sink), PA_SINK_MESSAGE_GET_LATENCY, &latency, 0, NULL);
      printf("%lld ", latency);
    */
#endif
    return 0;
}

static
size_t hw_sink_input_convert_bytes(pa_sink_input *i, pa_sink *s, size_t nbytes)
{
    return (nbytes/pa_frame_size(&i->thread_info.sample_spec))*pa_frame_size(&s->sample_spec);
}

/* Called from I/O thread context */
static void hw_sink_input_process_rewind_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (!PA_SINK_INPUT_IS_LINKED(i->thread_info.state))
        return;

    if (u->raw_sink && PA_SINK_IS_OPENED(u->raw_sink->thread_info.state)) {
  size_t amount = hw_sink_input_convert_bytes(i, u->raw_sink, nbytes);
  if (u->raw_sink->thread_info.rewind_nbytes > 0) {
      amount = PA_MIN(u->raw_sink->thread_info.rewind_nbytes, amount);
      u->raw_sink->thread_info.rewind_nbytes = 0;
  }
  pa_sink_process_rewind(u->raw_sink, amount);
    }

    if (u->voip_sink && PA_SINK_IS_OPENED(u->voip_sink->thread_info.state)) {
  size_t amount = hw_sink_input_convert_bytes(i, u->voip_sink, nbytes);
  if (u->voip_sink->thread_info.rewind_nbytes > 0) {
      amount = PA_MIN(u->voip_sink->thread_info.rewind_nbytes, amount);
      u->voip_sink->thread_info.rewind_nbytes = 0;
  }
  pa_sink_process_rewind(u->voip_sink, amount);
    }
}

/* Called from I/O thread context */
static void hw_sink_input_update_max_rewind_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (!PA_SINK_INPUT_IS_LINKED(i->thread_info.state))
        return;

    if (u->raw_sink && PA_SINK_IS_LINKED(u->raw_sink->thread_info.state))
        pa_sink_set_max_rewind_within_thread(u->raw_sink, hw_sink_input_convert_bytes(i, u->raw_sink, nbytes));

    if (u->voip_sink && PA_SINK_IS_LINKED(u->voip_sink->thread_info.state))
        pa_sink_set_max_rewind_within_thread(u->voip_sink, hw_sink_input_convert_bytes(i, u->voip_sink, nbytes));
}

/* Called from I/O thread context */
static void hw_sink_input_update_max_request_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (!PA_SINK_INPUT_IS_LINKED(i->thread_info.state))
        return;

    if (u->raw_sink && PA_SINK_IS_LINKED(u->raw_sink->thread_info.state))
        pa_sink_set_max_request_within_thread(u->raw_sink, hw_sink_input_convert_bytes(i, u->raw_sink, nbytes));

    if (u->voip_sink && PA_SINK_IS_LINKED(u->voip_sink->thread_info.state))
        pa_sink_set_max_request_within_thread(u->voip_sink, hw_sink_input_convert_bytes(i, u->voip_sink, nbytes));
}

/* Called from I/O thread context */
static void hw_sink_input_update_sink_latency_range_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (u->raw_sink && PA_SINK_IS_LINKED(u->raw_sink->thread_info.state))
  pa_sink_set_latency_range_within_thread(u->raw_sink, i->sink->thread_info.min_latency, i->sink->thread_info.max_latency);

    if (u->voip_sink && PA_SINK_IS_LINKED(u->voip_sink->thread_info.state))
  pa_sink_set_latency_range_within_thread(u->voip_sink, i->sink->thread_info.min_latency, i->sink->thread_info.max_latency);
}


/* Called from I/O thread context */
static void hw_sink_input_detach_slave_sink(pa_sink *sink) {
    pa_assert(sink);

    pa_sink_detach_within_thread(sink);
    sink->flat_volume_sink = NULL;
    pa_sink_set_asyncmsgq(sink, NULL);
    pa_sink_set_rtpoll(sink, NULL);

    voice_sink_inputs_may_move(sink, FALSE);
}

/* Called from I/O thread context */
static void hw_sink_input_detach_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    u->master_sink = NULL;

    if (u->raw_sink && PA_SINK_IS_LINKED(u->raw_sink->thread_info.state)) {
        hw_sink_input_detach_slave_sink(u->raw_sink);
    }

    if (u->voip_sink && PA_SINK_IS_LINKED(u->voip_sink->thread_info.state)) {
        hw_sink_input_detach_slave_sink(u->voip_sink);
    }

    pa_log_debug("Detach called");
}

/* Called from main thread context */
static void hw_sink_input_update_slave_sink(struct userdata *u, pa_sink *sink, pa_sink *to_sink) {
    pa_assert(sink);
    pa_proplist *p;

    p = pa_proplist_new();
    pa_proplist_setf(p, PA_PROP_DEVICE_DESCRIPTION, "%s connected to %s", sink->name, u->master_sink->name);
    pa_proplist_sets(p, PA_PROP_DEVICE_MASTER_DEVICE, u->master_sink->name);
    pa_sink_update_proplist(sink, PA_UPDATE_REPLACE, p);
    pa_proplist_free(p);
}

/* Called from I/O thread context */
static void hw_sink_input_attach_slave_sink(struct userdata *u, pa_sink *sink, pa_sink *to_sink) {
    pa_assert(sink);
    hw_sink_input_update_slave_sink(u,sink,to_sink);

    /* XXX: _set_asyncmsgq() shouldn't be called while the IO thread is
     * running, but we "have to" (ie. no better way to handle this has been
     * figured out). This call is one of the reasons for that we had to comment
     * out the assertion from pa_sink_set_asyncmsgq() that checks that the call
     * is done from the main thread. */
    pa_sink_set_asyncmsgq(sink, to_sink->asyncmsgq);

    pa_sink_set_rtpoll(sink, to_sink->rtpoll);
    voice_sink_inputs_may_move(sink, TRUE);
    sink->flat_volume_sink = to_sink;
    pa_sink_attach_within_thread(sink);
    pa_sink_set_latency_range_within_thread(sink, u->master_sink->thread_info.min_latency, u->master_sink->thread_info.max_latency);
    pa_sink_set_max_rewind_within_thread(sink, u->master_sink->thread_info.max_rewind);
}

static void voice_hw_sink_input_reset_volume_defer_cb(pa_mainloop_api *m, pa_defer_event *de, void *userdata) {
    pa_cvolume v;
    pa_sink_input *i;

    pa_assert_se(i = userdata);

    m->defer_enable(de, 0);

    /* FIXME: this is kind of UGLY way to solve the volume being
       relative to sink when detached... (ie origin_sink == NULL) */
    pa_cvolume_reset(&v, i->sample_spec.channels);
    i->soft_volume = v;

    if (!i->sink)
        return;
    pa_sink_input_set_volume(i, &v, FALSE, TRUE);

    pa_assert_se(pa_asyncmsgq_send(i->sink->asyncmsgq, PA_MSGOBJECT(i->sink), PA_SINK_MESSAGE_SYNC_VOLUMES, NULL, 0, NULL) == 0);
}

/* Called from I/O thread context */
static void hw_sink_input_attach_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    u->master_sink = i->sink;
    u->core->mainloop->defer_new(u->core->mainloop, voice_hw_sink_input_reset_volume_defer_cb, i);

    if (u->raw_sink && PA_SINK_IS_LINKED(u->raw_sink->thread_info.state)) {
        hw_sink_input_attach_slave_sink(u, u->raw_sink, i->sink);
    }

    if (u->voip_sink && PA_SINK_IS_LINKED(u->voip_sink->thread_info.state)) {
        hw_sink_input_attach_slave_sink(u, u->voip_sink, i->sink);
    }

    pa_log_debug("Attach called, new master %p %s", (void*)u->master_sink, u->master_sink->name);
}

/* Called from main context */
static void hw_sink_input_kill_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_log_debug("Kill called");

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_sink_unlink(u->voip_sink);
    pa_sink_unlink(u->raw_sink);

    pa_sink_input_unlink(u->hw_sink_input);

    pa_sink_unref(u->voip_sink);
    u->voip_sink = NULL;
    pa_sink_unref(u->raw_sink);
    u->raw_sink = NULL;

    /* FIXME: this is sort-of understandable with the may_move hack... we avoid abort in free() here */
    u->hw_sink_input->thread_info.attached = FALSE;
    pa_sink_input_unref(u->hw_sink_input);
    u->hw_sink_input = NULL;
}

/* Called from main context */
static void hw_sink_input_moving_cb(pa_sink_input *i, pa_sink *dest){
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_log_debug("Sink input moving to %s", dest ? dest->name : "(null)");

    if ((i->sample_spec.rate == SAMPLE_RATE_AEP_HZ &&
   dest->sample_spec.rate != SAMPLE_RATE_AEP_HZ) ||
  (i->sample_spec.rate != SAMPLE_RATE_AEP_HZ &&
   dest->sample_spec.rate == SAMPLE_RATE_AEP_HZ)) {
  pa_log_info("Reinitialize due to samplerate change %d->%d.",
                    i->sample_spec.rate, dest->sample_spec.rate);
        pa_log_debug("New sink format %s", pa_sample_format_to_string(dest->sample_spec.format)) ;
        pa_log_debug("New sink rate %d", dest->sample_spec.rate);
        pa_log_debug("New sink channels %d", dest->sample_spec.channels);

        voice_reinit_hw_sink_input(u);
    }
}

static pa_bool_t hw_sink_input_may_move_to_cb(pa_sink_input *i, pa_sink *dest) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (u->master_sink == NULL)
        return TRUE;

    return ((u->master_sink != dest) && (u->master_sink->asyncmsgq != dest->asyncmsgq));
}

static pa_hook_result_t hw_sink_input_move_fail_cb(pa_core *c, pa_sink_input *i, struct userdata *u) {
    const char *master_sink;
    pa_sink *s = NULL;

    pa_assert(u);
    pa_sink_input_assert_ref(i);

    if (i != u->hw_sink_input)
        return PA_HOOK_OK;

    master_sink = pa_modargs_get_value(u->modargs, "master_sink", NULL);

    if (!master_sink
        || !(s = pa_namereg_get(u->core, master_sink, PA_NAMEREG_SINK))) {

        pa_log("Master sink \"%s\" not found", master_sink);
        return PA_HOOK_OK;
    }

    if (pa_sink_input_finish_move(i, s, TRUE) >= 0)
        return PA_HOOK_STOP;

    pa_log("Failed to fallback on \"%s\".", master_sink);

    /* ok fallback on destroying the hw_sink_input (voice module will be unloaded) */
    return PA_HOOK_OK;
}

static pa_hook_result_t hw_sink_input_move_finish_cb(pa_core *c, pa_sink_input *i, struct userdata *u)
{
    pa_sink *sink = i->sink;
    if (sink)
    {
        voice_sink_proplist_update(u,sink);
    }
    return PA_HOOK_OK;
}

static pa_sink_input *voice_hw_sink_input_new(struct userdata *u, pa_sink_input_flags_t flags) {
    pa_sink_input_new_data sink_input_data;
    pa_sink_input *new_sink_input;
    char t[256];

    pa_assert(u);
    pa_assert(u->master_sink);
    ENTER();

    snprintf(t, sizeof(t), "output of %s", u->raw_sink->name);

    pa_sink_input_new_data_init(&sink_input_data);
    sink_input_data.driver = __FILE__;
    sink_input_data.module = u->module;
    sink_input_data.sink = u->master_sink;
    sink_input_data.origin_sink = u->raw_sink;
    sink_input_data.volume_is_absolute = TRUE;
    sink_input_data.volume_is_set = TRUE;

    pa_proplist_sets(sink_input_data.proplist, PA_PROP_MEDIA_NAME, t);
    pa_proplist_sets(sink_input_data.proplist, PA_PROP_APPLICATION_NAME, t); /* this is the default value used by PA modules */
    if (u->master_sink->sample_spec.rate == SAMPLE_RATE_AEP_HZ) {
  pa_sink_input_new_data_set_sample_spec(&sink_input_data, &u->aep_sample_spec);
  pa_sink_input_new_data_set_channel_map(&sink_input_data, &u->aep_channel_map);
    }
    else {
  pa_sink_input_new_data_set_sample_spec(&sink_input_data, &u->hw_sample_spec);
  pa_sink_input_new_data_set_channel_map(&sink_input_data, &u->stereo_map);
    }
    pa_cvolume_reset(&sink_input_data.volume, sink_input_data.sample_spec.channels);

    pa_sink_input_new(&new_sink_input, u->core, &sink_input_data, flags);
    pa_sink_input_new_data_done(&sink_input_data);

    if (!new_sink_input) {
        pa_log_warn("Creating sink input failed");
        return NULL;
    }

    if (u->master_sink->sample_spec.rate == SAMPLE_RATE_AEP_HZ)
        new_sink_input->pop = hw_sink_input_pop_8k_mono_cb;
    else
        new_sink_input->pop = hw_sink_input_pop_cb;
    new_sink_input->process_rewind = hw_sink_input_process_rewind_cb;
    new_sink_input->update_max_rewind = hw_sink_input_update_max_rewind_cb;
    new_sink_input->update_max_request = hw_sink_input_update_max_request_cb;
    new_sink_input->update_sink_latency_range = hw_sink_input_update_sink_latency_range_cb;
    new_sink_input->kill = hw_sink_input_kill_cb;
    new_sink_input->attach = hw_sink_input_attach_cb;
    new_sink_input->detach = hw_sink_input_detach_cb;
    new_sink_input->moving = hw_sink_input_moving_cb;
    new_sink_input->may_move_to = hw_sink_input_may_move_to_cb;
    new_sink_input->userdata = u;

    return new_sink_input;
}

int voice_init_hw_sink_input(struct userdata *u) {
    pa_assert(u);

    u->hw_sink_input = voice_hw_sink_input_new(u, 0);
    pa_return_val_if_fail (u->hw_sink_input, -1);

    u->hw_sink_input_move_fail_slot = pa_hook_connect(&u->core->hooks[PA_CORE_HOOK_SINK_INPUT_MOVE_FAIL], PA_HOOK_EARLY, (pa_hook_cb_t) hw_sink_input_move_fail_cb, u);
    u->hw_sink_input_move_finish_slot = pa_hook_connect(&u->core->hooks[PA_CORE_HOOK_SINK_INPUT_MOVE_FINISH], PA_HOOK_EARLY, (pa_hook_cb_t) hw_sink_input_move_finish_cb, u);

    return 0;
}

struct voice_hw_sink_input_reinit_defered {
    struct userdata *u;
    pa_defer_event *defer;
};

static void voice_hw_sink_input_reinit_defer_cb(pa_mainloop_api *m, pa_defer_event *de, void *userdata) {
    struct voice_hw_sink_input_reinit_defered *d;
    pa_sink_input *new_si, *old_si;
    struct userdata *u;
    pa_bool_t start_uncorked;

    pa_assert_se(d = userdata);	
    pa_assert_se(u = d->u);
    pa_assert_se(old_si = u->hw_sink_input);

    m->defer_enable(d->defer, 0);
    pa_xfree(d);
    d = NULL;

    start_uncorked = PA_SINK_IS_OPENED(pa_sink_get_state(u->raw_sink)) ||
        PA_SINK_IS_OPENED(pa_sink_get_state(u->voip_sink)) ||
        pa_atomic_load(&u->cmt_connection.dl_state) == 1 ||
        pa_sink_input_get_state(old_si) != PA_SINK_INPUT_CORKED;
    pa_log("HWSI START UNCORKED: %d", start_uncorked);

    new_si = voice_hw_sink_input_new(u, start_uncorked ? 0 : PA_SINK_INPUT_START_CORKED);
    pa_return_if_fail(new_si);

    pa_sink_input_cork(old_si, TRUE);

    pa_log_debug("reinitialize hw sink-input %s %p", u->master_sink->name, (void*)new_si);

    u->hw_sink_input = new_si;
    pa_sink_input_put(u->hw_sink_input);

    pa_log_debug("Detaching the old sink input %p", (void*)old_si);

    old_si->detach = NULL;
    pa_sink_input_unlink(old_si);
    pa_sink_input_unref(old_si);

    voice_aep_ear_ref_loop_reset(u);
}

void voice_reinit_hw_sink_input(struct userdata *u) {
    struct voice_hw_sink_input_reinit_defered *d;
    pa_assert(u);

    d = pa_xnew0(struct voice_hw_sink_input_reinit_defered, 1); /* in theory should be tracked if pulseaudio exit before defer called (could be added to userdata, but would need to be queued */
    d->u = u;
    d->defer = u->core->mainloop->defer_new(u->core->mainloop, voice_hw_sink_input_reinit_defer_cb, d);
}
