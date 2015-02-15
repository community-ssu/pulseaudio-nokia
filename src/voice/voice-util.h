#ifndef VOICEUTIL_H
#define VOICEUTIL_H

#define ONDEBUG_TOKENS(a)

#define ENTER() pa_log_debug("%d: %s() called", __LINE__, __FUNCTION__)

/* TODO: Change ear ref loop to use pa_usec_t and get rid off these */
#define VOICE_TIMEVAL_INVALIDATE(TVal) ((TVal)->tv_usec = -1, (TVal)->tv_sec = 0)
#define VOICE_TIMEVAL_IS_VALID(TVal) ((pa_bool_t) ((TVal)->tv_usec >= 0))

#define VOICE_MEMCHUNK_POOL_SIZE 128
typedef struct voice_memchunk_pool {
    pa_memchunk chunk;
    pa_atomic_ptr_t next;
} voice_memchunk_pool;

void voice_memchunk_pool_load(struct userdata *u);
void voice_memchunk_pool_unload(struct userdata *u);

static inline
pa_memchunk *voice_memchunk_pool_get(struct userdata *u) {
    voice_memchunk_pool *mp;
    do {
        mp = (voice_memchunk_pool *) pa_atomic_ptr_load(&u->memchunk_pool);
    } while (mp != NULL &&
             !pa_atomic_ptr_cmpxchg(&u->memchunk_pool, mp, pa_atomic_ptr_load(&mp->next)));
    if (mp != NULL)
        return &mp->chunk;
    pa_log_warn("voice_memchunk_pool empty, all %d slots allocated", VOICE_MEMCHUNK_POOL_SIZE);
    return NULL;
}

static inline
void voice_memchunk_pool_free(struct userdata *u, pa_memchunk *chunk) {
    voice_memchunk_pool *mp, *mp_new = (voice_memchunk_pool *)chunk;
    pa_memchunk_reset(chunk);
    do {
        mp = (voice_memchunk_pool *) pa_atomic_ptr_load(&u->memchunk_pool);
        pa_atomic_ptr_store(&mp_new->next, mp);
    } while (!pa_atomic_ptr_cmpxchg(&u->memchunk_pool, mp, mp_new));
}

void voice_clear_up(struct userdata *u);

void voice_sink_inputs_may_move(pa_sink *s, pa_bool_t move);
void voice_source_outputs_may_move(pa_source *s, pa_bool_t move);

#endif // VOICEUTIL_H
