#include "module-voice-userdata.h"
#include "voice-cmtspeech.h"
#include "voice-convert.h"
#include "voice-event-forwarder.h"
#include "voice-aep-ear-ref.h"

#include "voice-util.h"

static voice_memchunk_pool *voice_memchunk_pool_table = NULL;
void voice_memchunk_pool_load(struct userdata *u)
{
    int i;

    pa_assert(0 == offsetof(voice_memchunk_pool, chunk));
    pa_atomic_ptr_store(&u->memchunk_pool, NULL);

    voice_memchunk_pool_table =
        pa_xmalloc0(sizeof(voice_memchunk_pool)*VOICE_MEMCHUNK_POOL_SIZE);
    pa_assert(voice_memchunk_pool_table);

    for (i = 0; i < VOICE_MEMCHUNK_POOL_SIZE; i++)
        voice_memchunk_pool_free(u,
                                 (pa_memchunk *)&voice_memchunk_pool_table[i]);
}

void voice_memchunk_pool_unload(struct userdata *u)
{
    int i = 0;

    if (voice_memchunk_pool_table == NULL)
        return;

    while (voice_memchunk_pool_get(u)) i++;

    if (i < VOICE_MEMCHUNK_POOL_SIZE)
        pa_log("voice_memchunk_pool only %d element of %d allocated was retured to pool",
               i, VOICE_MEMCHUNK_POOL_SIZE);

    pa_xfree(voice_memchunk_pool_table);
    voice_memchunk_pool_table = NULL;
}

/*** Deallocate stuff ***/
void voice_clear_up(struct userdata *u)
{
  pa_assert(u);

  voice_unload_cmtspeech(u);

  if (u->mainloop_handler)
  {
      u->mainloop_handler->parent.free((pa_object *)u->mainloop_handler);
      u->mainloop_handler = NULL;
  }

  if (u->hw_sink_input)
  {
    pa_sink_input_unlink(u->hw_sink_input);
    pa_sink_input_unref(u->hw_sink_input);
    u->hw_sink_input = NULL;
  }

  if (u->raw_sink)
  {
      pa_sink_unlink(u->raw_sink);
      pa_sink_unref(u->raw_sink);
      u->raw_sink = NULL;
  }

  if (u->unused_memblockq)
  {
    pa_memblockq_free(u->unused_memblockq);
    u->unused_memblockq = NULL;
  }

  if (u->voip_sink)
  {
      pa_sink_unlink(u->voip_sink);
      pa_sink_unref(u->voip_sink);
      u->voip_sink = NULL;
  }

  if (u->hw_source_output)
  {
      pa_source_output_unlink(u->hw_source_output);
      pa_source_output_unref(u->hw_source_output);
      u->hw_source_output = NULL;
  }

  if (u->voip_source)
  {
      pa_source_unlink(u->voip_source);
      pa_source_unref(u->voip_source);
      u->voip_source = NULL;
  }

  if (u->raw_source)
  {
      pa_source_unlink(u->raw_source);
      pa_source_unref(u->raw_source);
      u->raw_source = NULL;
  }

  if (u->hw_source_memblockq)
  {
      pa_memblockq_free(u->hw_source_memblockq);
      u->hw_source_memblockq = NULL;
  }

  if (u->ul_memblockq)
  {
      pa_memblockq_free(u->ul_memblockq);
      u->ul_memblockq = NULL;
  }

  if (u->dl_sideinfo_queue)
  {
      pa_queue_free(u->dl_sideinfo_queue, NULL, u);
      u->dl_sideinfo_queue = NULL;
  }

  voice_aep_ear_ref_unload(u);

  if (u->aep_silence_memchunk.memblock)
  {
      pa_memblock_unref(u->aep_silence_memchunk.memblock);
      pa_memchunk_reset(&u->aep_silence_memchunk);
  }

  if (u->sink_temp_buff)
  {
      pa_xfree(u->sink_temp_buff);
      u->sink_temp_buff = NULL;
  }

  if (u->sink_subscription)
  {
      pa_subscription_free(u->sink_subscription);
      u->sink_subscription = NULL;
  }

  if (u->sink_proplist_changed_slot)
  {
    pa_hook_slot_free(u->sink_proplist_changed_slot);
    u->sink_proplist_changed_slot = NULL;
  }

  if (u->source_proplist_changed_slot)
  {
    pa_hook_slot_free(u->source_proplist_changed_slot);
    u->source_proplist_changed_slot = NULL;
  }

  voice_convert_free(u);
  voice_memchunk_pool_unload(u);
  voice_unload_event_forwarder(u);
}

void voice_sink_inputs_may_move(pa_sink *s, pa_bool_t move) {
    pa_sink_input *i;
    uint32_t idx;

    for (i = PA_SINK_INPUT(pa_idxset_first(s->inputs, &idx)); i; i = PA_SINK_INPUT(pa_idxset_next(s->inputs, &idx))) {
        if (move)
            i->flags &= ~PA_SINK_INPUT_DONT_MOVE;
        else
            i->flags |= PA_SINK_INPUT_DONT_MOVE;
    }
}

void voice_source_outputs_may_move(pa_source *s, pa_bool_t move) {
    pa_source_output *i;
    uint32_t idx;

    for (i = PA_SOURCE_OUTPUT(pa_idxset_first(s->outputs, &idx)); i; i = PA_SOURCE_OUTPUT(pa_idxset_next(s->outputs, &idx))) {
        if (move)
            i->flags &= ~PA_SOURCE_OUTPUT_DONT_MOVE;
        else
            i->flags |= PA_SOURCE_OUTPUT_DONT_MOVE;
    }
}

