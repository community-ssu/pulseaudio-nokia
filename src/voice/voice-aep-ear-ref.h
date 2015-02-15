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
#ifndef voice_aep_ear_ref_h
#define voice_aep_ear_ref_h

#include <pulsecore/core-rtclock.h>
#include <pulse/timeval.h>
#include "memory.h"
#include "voice-util.h"

#include "module-voice-userdata.h"

/* TODO: Split into separate .c file */

/* FIXME: These shared constants should be defined in some header file in the
 * main pulseaudio tree. */
#define PA_SINK_MESSAGE_GET_UNDERRUN    (PA_SINK_MESSAGE_MAX + 50)
#define PA_SOURCE_MESSAGE_GET_OVERRUN   (PA_SOURCE_MESSAGE_MAX + 1)

enum { VOICE_EAR_REF_RESET = 0,
       VOICE_EAR_REF_UL_READY,
       VOICE_EAR_REF_DL_READY,
       VOICE_EAR_REF_RUNNING,
       VOICE_EAR_REF_QUIT };

static inline
void voice_aep_ear_ref_loop_reset(struct userdata *u) {
    struct voice_aep_ear_ref *r = &u->ear_ref;
    pa_log_debug("Ear ref loop reset initiated");
    pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
}

static inline
void voice_aep_ear_ref_init(struct userdata *u) {
    pa_assert(u);
    struct voice_aep_ear_ref *r = &u->ear_ref;
    /* NOTE: 0 (zero) value would make more sense here. However, because
       everything was tuned with value corresponding to -3333 I do not
       want to change that in Frematle (not before S3 anyway).
    */
    r->loop_padding_usec = -3333; /* The default value (usec)*/
    pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
    VOICE_TIMEVAL_INVALIDATE(&r->loop_tstamp);
    r->loop_asyncq = pa_asyncq_new(16);
    pa_assert(r->loop_asyncq);
    r->loop_memblockq = pa_memblockq_new((int64_t)0, 20*u->aep_fragment_size, /* = 200ms */
					 0, pa_frame_size(&u->aep_sample_spec), 0, 0, 0, NULL);
    pa_assert(r->loop_memblockq);
}

static inline
void voice_aep_ear_ref_unload(struct userdata *u) {
    pa_assert(u);
    struct voice_aep_ear_ref *r = &u->ear_ref;
    pa_atomic_store(&r->loop_state, VOICE_EAR_REF_QUIT);
    pa_memchunk *chunk;
    while((chunk = pa_asyncq_pop(r->loop_asyncq, 0))) {
	voice_memchunk_pool_free(u, chunk);
    }
    pa_asyncq_free(r->loop_asyncq, NULL);
    VOICE_TIMEVAL_INVALIDATE(&r->loop_tstamp);
    pa_memblockq_free(r->loop_memblockq);
    r->loop_memblockq = NULL;
}

/* FIXME; Check underrun on in the appropriate IO thread... */
static inline
int voice_aep_ear_ref_check_xrun(struct userdata *u) {
    struct voice_aep_ear_ref *r = &u->ear_ref;
    pa_bool_t underrun, overrun;

    if (u->master_sink) {
        PA_MSGOBJECT(u->master_sink)->process_msg(
                PA_MSGOBJECT(u->master_sink), PA_SINK_MESSAGE_GET_UNDERRUN, &underrun, (int64_t)0, NULL);
    }

    if (u->master_source) {
        PA_MSGOBJECT(u->master_source)->process_msg(
                PA_MSGOBJECT(u->master_source), PA_SOURCE_MESSAGE_GET_OVERRUN, &overrun, (int64_t)0, NULL);
    }

    if (underrun || overrun) {
        pa_log_debug("XRUN -> reset");
        pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
        return 1;
    }

    return 0;
}

static inline
int voice_aep_ear_ref_dl_push_to_syncq(struct userdata *u, pa_memchunk *chunk) {
    pa_memchunk *qchunk = voice_memchunk_pool_get(u);
    if (qchunk == NULL)
	return -1;
    *qchunk = *chunk;
    pa_memblock_ref(qchunk->memblock);
    static int fail_count = 0;
    if (pa_asyncq_push(u->ear_ref.loop_asyncq, qchunk, FALSE)) {
	pa_memblock_unref(qchunk->memblock);
	voice_memchunk_pool_free(u, qchunk);
	if (fail_count == 0)
	    pa_log_debug("Failed to push dl frame to asyncq");
	fail_count++;
    }
    else if (fail_count > 0) {
	if (fail_count > 1)
	    pa_log_debug("Failed to push dl frame to asyncq %d times", fail_count);
	fail_count = 0;
    }

    return 0;
}

static inline
int voice_aep_ear_ref_dl(struct userdata *u, pa_memchunk *chunk) {
    pa_assert(u);
    struct voice_aep_ear_ref *r = &u->ear_ref;

    int loop_state = pa_atomic_load(&r->loop_state);
    switch (loop_state) {
    case  VOICE_EAR_REF_RUNNING:
    case  VOICE_EAR_REF_DL_READY: {
        if (!voice_aep_ear_ref_check_xrun(u)) {
            if (voice_aep_ear_ref_dl_push_to_syncq(u, chunk))
                return -1;
        }
	break;
    }
    case VOICE_EAR_REF_UL_READY: {
	struct timeval tv;
	pa_usec_t latency;
    PA_MSGOBJECT(u->master_sink)->process_msg(
        PA_MSGOBJECT(u->master_sink), PA_SINK_MESSAGE_GET_LATENCY, &latency, (int64_t)0, NULL);

	pa_rtclock_get(&tv);
	pa_timeval_add(&tv, latency);
	r->loop_tstamp = tv;
	pa_log_debug("Ear ref loop DL due at %d.%06d (%lld latency)",
		     (int)tv.tv_sec, (int)tv.tv_usec, latency);

	if (voice_aep_ear_ref_dl_push_to_syncq(u, chunk))
	    return -1;

	pa_atomic_store(&r->loop_state, VOICE_EAR_REF_DL_READY);
	break;
    }
    case VOICE_EAR_REF_QUIT:
    case VOICE_EAR_REF_RESET: {
	/* Reset sequence starts on UL side, nothing to do. */
	break;
    }
    }
    return 0;
}

static inline
int voice_aep_ear_ref_ul_drain_asyncq(struct userdata *u, pa_bool_t push_forward) {
    struct voice_aep_ear_ref *r = &u->ear_ref;
    pa_memchunk *chunk;
    int queue_counter = 0;
    while ((chunk = pa_asyncq_pop(r->loop_asyncq, FALSE))) {
	queue_counter++;
        if (push_forward) {
            if (pa_memblockq_push(r->loop_memblockq, chunk) < 0) {
                pa_log_debug("Failed to push %d bytes of ear ref data to loop_memblockq (len %d max %d )",
                             chunk->length, pa_memblockq_get_length(r->loop_memblockq),
                             pa_memblockq_get_maxlength(r->loop_memblockq));
                voice_aep_ear_ref_loop_reset(u);
            }
        }
	pa_memblock_unref(chunk->memblock);
	voice_memchunk_pool_free(u, chunk);
    }
    return queue_counter;
}

static inline
int voice_aep_ear_ref_ul(struct userdata *u, pa_memchunk *chunk) {
    pa_assert(u);
    struct voice_aep_ear_ref *r = &u->ear_ref;
    int ret = 0;
    while (ret == 0) {
	int loop_state = pa_atomic_load(&r->loop_state);
	switch (loop_state) {
	case VOICE_EAR_REF_RUNNING: {
            if (!voice_aep_ear_ref_check_xrun(u)) {
                voice_aep_ear_ref_ul_drain_asyncq(u, TRUE);
                if (util_memblockq_to_chunk(u->core->mempool, r->loop_memblockq, chunk, u->aep_fragment_size)) {
                    ret = 1;
                }
                else {
                    /* Queue has run out, reset the queue. */
                    pa_log_debug("Only %d bytes left in ear ref loop, let's reset the loop",
                                 pa_memblockq_get_length(r->loop_memblockq));
                    pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
                }
            }
	}
	    break;
	case VOICE_EAR_REF_RESET: {
	    voice_aep_ear_ref_ul_drain_asyncq(u, FALSE);
	    pa_memblockq_drop(r->loop_memblockq, pa_memblockq_get_length(r->loop_memblockq));
	    pa_atomic_store(&r->loop_state, VOICE_EAR_REF_UL_READY);
	}
	    break;
	case VOICE_EAR_REF_DL_READY: {
	    struct timeval tv_ul_tstamp;
	    struct timeval tv_dl_tstamp = r->loop_tstamp;
	    pa_assert(VOICE_TIMEVAL_IS_VALID(&tv_dl_tstamp));
	    pa_usec_t latency;
	    PA_MSGOBJECT(u->master_source)->process_msg(
		PA_MSGOBJECT(u->master_source), PA_SOURCE_MESSAGE_GET_LATENCY, &latency, (int64_t)0, NULL);
            latency += pa_bytes_to_usec((uint64_t)pa_memblockq_get_length(u->hw_source_memblockq),
                                        &u->hw_source_output->sample_spec);
	    pa_rtclock_get(&tv_ul_tstamp);
	    pa_timeval_sub(&tv_ul_tstamp, latency + r->loop_padding_usec);
	    pa_usec_t loop_padding_time = pa_timeval_diff(&tv_ul_tstamp, &tv_dl_tstamp);
	    size_t loop_padding_bytes = pa_usec_to_bytes_round_up(loop_padding_time, &u->aep_sample_spec);

	    pa_log_debug("Ear ref loop padding %d.%06d - %d.%06d = %lld = %d bytes (%lld latency %d extra padding)",
			 (int)tv_ul_tstamp.tv_sec, (int)tv_ul_tstamp.tv_usec,
			 (int)tv_dl_tstamp.tv_sec, (int)tv_dl_tstamp.tv_usec,
			 loop_padding_time, loop_padding_bytes,
			 latency, r->loop_padding_usec);
	    if (0 > pa_timeval_cmp(&tv_dl_tstamp, &tv_ul_tstamp)) {
		pa_log_debug("Dl stamp precedes UL stamp %d.%06d < %d.%06d, something went wrong -> reset",
			     (int)tv_dl_tstamp.tv_sec, (int)tv_dl_tstamp.tv_usec,
			     (int)tv_ul_tstamp.tv_sec, (int)tv_ul_tstamp.tv_usec);
		pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
		break;
	    }

	    if (loop_padding_bytes >= pa_memblockq_get_maxlength(r->loop_memblockq)) {
		pa_log_debug("Too long loop time %lld, reset init sequence", loop_padding_time);
		pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
		break;
	    }

	    pa_memchunk schunk;
	    pa_silence_memchunk_get(&u->core->silence_cache,
				    u->core->mempool,
				    &schunk,
				    &u->aep_sample_spec,
				    loop_padding_bytes);

	    if (pa_memblockq_push(r->loop_memblockq, &schunk) < 0) {
		pa_log_debug("Failed to push %d bytes of ear ref padding to memblockq (len %d max %d )",
			     loop_padding_bytes,
			     pa_memblockq_get_length(r->loop_memblockq),
			     pa_memblockq_get_maxlength(r->loop_memblockq));
	    }
	    pa_memblock_unref(schunk.memblock);
	    VOICE_TIMEVAL_INVALIDATE(&r->loop_tstamp);
	    pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RUNNING);
	    pa_log_debug("Ear ref loop init sequence ready.");
	}
	    break;
	case VOICE_EAR_REF_QUIT:
	case VOICE_EAR_REF_UL_READY: {
	    //pa_log_debug("Still waiting for loop initialization data from DL");
	    *chunk = u->aep_silence_memchunk;
	    pa_memblock_ref(chunk->memblock);
	    ret = -1;
	}
	    break;
	}
    }
    return ret;
}

static inline
void voice_aep_ear_ref_ul_drop(struct userdata *u, pa_usec_t drop_usecs) {
    struct voice_aep_ear_ref *r = &u->ear_ref;
    size_t drop_bytes = pa_usec_to_bytes_round_up(drop_usecs, &u->aep_sample_spec);
    if (pa_memblockq_get_length(r->loop_memblockq) >= drop_bytes + u->aep_fragment_size)
        pa_memblockq_drop(r->loop_memblockq, drop_bytes);
    else
        pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
}

static inline
void voice_aep_ear_ref_ul_drop_log(struct userdata *u, pa_usec_t drop_usecs) {
    struct voice_aep_ear_ref *r = &u->ear_ref;
    size_t drop_bytes = pa_usec_to_bytes_round_up(drop_usecs, &u->aep_sample_spec);
    if (pa_memblockq_get_length(r->loop_memblockq) >= drop_bytes + u->aep_fragment_size) {
        pa_memblockq_drop(r->loop_memblockq, drop_bytes);
        pa_log_debug("Dropped %lld usec = %d bytes, %d bytes left in loop", drop_usecs, drop_bytes,
                     pa_memblockq_get_length(r->loop_memblockq));
    } else {
        pa_log_debug("Not enough bytes in ear ref loop %d < %d + %d, resetting",
                     pa_memblockq_get_length(r->loop_memblockq), drop_bytes, u->aep_fragment_size);
        pa_atomic_store(&r->loop_state, VOICE_EAR_REF_RESET);
    }
}


#endif // voice_aep_ear_ref_h
