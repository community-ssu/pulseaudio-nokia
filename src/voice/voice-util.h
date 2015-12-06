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
#ifndef voice_util_h
#define voice_util_h

#include "module-voice-userdata.h"

#define ONDEBUG_TOKENS(a)

#define ENTER() pa_log_debug("%d: %s() called", __LINE__, __FUNCTION__)

#define VOICE_TIMEVAL_INVALIDATE(TVal) ((TVal)->tv_usec = -1, (TVal)->tv_sec = 0)
#define VOICE_TIMEVAL_IS_VALID(TVal) ((pa_bool_t) ((TVal)->tv_usec >= 0))

static inline pa_bool_t
voice_pa_proplist_get_bool(pa_proplist *p, const char *key)
{
  const char *s = pa_proplist_gets(p, key);

  if (!s)
    s = "null";

  return pa_parse_boolean(s);
}

#define VOICE_MEMCHUNK_POOL_SIZE 128
typedef struct voice_memchunk_pool {
    pa_memchunk chunk;
    pa_atomic_ptr_t next;
} voice_memchunk_pool;

void voice_memchunk_pool_load(struct userdata *u);
void voice_memchunk_pool_unload(struct userdata *u);

static inline
pa_memchunk *voice_memchunk_pool_get(struct userdata *u)
{
    voice_memchunk_pool *mp;
    do
    {
        mp = (voice_memchunk_pool *) pa_atomic_ptr_load(&u->memchunk_pool);
    }
    while (mp != NULL &&
           !pa_atomic_ptr_cmpxchg(&u->memchunk_pool, mp,
                                  pa_atomic_ptr_load(&mp->next)));

    if (mp != NULL)
        return &mp->chunk;

    pa_log_warn("voice_memchunk_pool empty, all %d slots allocated",
                VOICE_MEMCHUNK_POOL_SIZE);

    return NULL;
}

inline static void
voice_memchunk_pool_free(struct userdata *u, pa_memchunk *chunk)
{
    voice_memchunk_pool *mp, *mp_new = (voice_memchunk_pool *)chunk;

    pa_memchunk_reset(chunk);

    do
    {
        mp = (voice_memchunk_pool *) pa_atomic_ptr_load(&u->memchunk_pool);
        pa_atomic_ptr_store(&mp_new->next, mp);
    }
    while (!pa_atomic_ptr_cmpxchg(&u->memchunk_pool, mp, mp_new));
}

void
voice_clear_up(struct userdata *u);

int
voice_source_set_state(pa_source *s, pa_source *other, pa_source_state_t state);

int
voice_sink_set_state(pa_sink *s, pa_sink *other, pa_sink_state_t state);

void
voice_sink_inputs_may_move(pa_sink *s, pa_bool_t move);
void
voice_source_outputs_may_move(pa_source *s, pa_bool_t move);

pa_sink *
voice_get_original_master_sink(struct userdata *u);
pa_source *
voice_get_original_master_source(struct userdata *u);

void
voice_sink_proplist_update(struct userdata *u, pa_sink *s);
/* BEGIN OF AEP-SIDETONE SPAGETHI */
void
voice_update_aep_volume(int16_t aep_step);
void
voice_set_aep_runtime_switch(const char *aep_runtime_src);
void
voice_shutdown_aep(void);
int
voice_pa_vol_to_aep_step(struct userdata *u,pa_volume_t vol);
int
voice_parse_aep_steps(struct userdata *u,const char *steps);
void
voice_update_parameters(struct userdata *u);

/* END OF AEP-SIDETONE SPAGETHI */

// For debugging...
void voice_append_chunk_to_file(struct userdata *u, const char *file_name, pa_memchunk *chunk);
#endif 
// voice_util_h
