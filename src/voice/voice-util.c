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

#include <errno.h>
#include <pulsecore/namereg.h>

#include "module-voice-userdata.h"
#include "voice-util.h"
#include "voice-aep-ear-ref.h"
#include "voice-convert.h"
#include "voice-mainloop-handler.h"
#include "voice-event-forwarder.h"
#include "voice-sidetone.h"
#include "voice-cmtspeech.h"

#include "voice-voip-source.h"
#include "voice-voip-sink.h"

#include "proplist-nokia.h"
#include "proplist-file.h"

struct aep_switches_s
{
  int16_t field_0;
  int16_t field_2;
  int16_t field_4;
};

struct aep_switches_s aep_switches;
char aep_runtime_switch[30];

void voice_update_aep_volume(int16_t aep_step)
{
    assert(0 && "TODO voice_update_aep_volume address 0x00018AD0");
}

void voice_shutdown_aep(void)
{
    assert(0 && "TODO voice_shutdown_aep address 0x00018C2C");
}

void voice_set_aep_runtime_switch(const char *aep_runtime_src)
{
    assert(0 && "TODO voice_set_aep_runtime_switch address 0x00018C7C");
}

int
voice_pa_vol_to_aep_step(struct userdata *u,pa_volume_t vol)
{
  int i = 0;
  double vol_dB;
  struct aep_volume_steps_s *s = &u->aep_volume_steps;

  if (!s->count)
  {
    pa_log_warn("AEP volume steps table not set.");

    return -1;
  }

  vol_dB = pa_sw_volume_to_dB(vol) * 100.0;

  for (i = 0; i < s->count && s->steps[i] < vol_dB; i ++);

  return i;
}

int voice_parse_aep_steps(struct userdata *u, const char *steps)
{
    char *token;
    const char *state;
    int i = -1;
    int32_t step;
    int32_t old = 0;

    for (token = pa_split(steps, ",", &state); token; i ++)
    {
        if (i > (int)ARRAY_SIZE(u->aep_volume_steps.steps))
        {
            pa_log_error("Too many elements in aep volume steps table: %d > %d",
                         i, ARRAY_SIZE(u->aep_volume_steps.steps));
            pa_xfree(token);
            goto error;
        }

        if (pa_atoi(token, &step))
        {
            pa_xfree(token);
            goto error;
        }

        if (i < 0)
            old = step;
        else
            u->aep_volume_steps.steps[i] = (old + step) / 2;

        pa_xfree(token);
    }

    u->aep_volume_steps.count = i;
    pa_log_info("AEP volume steps table read, %d steps found", i + 1);

    return 0;

error:

    pa_log_error("Error near token '%s' when parsing parameter %s: %s", token,
                 PA_NOKIA_PROP_AUDIO_AEP_mB_STEPS, steps);
    u->aep_volume_steps.count = 0;

    return -1;
}

/* FIXME */
void *current_aep_tuning = 0;
int init_main(int args, const char *argc[])
{
  pa_assert(0);

  return 0;
}

void voice_update_parameters(struct userdata *u)
{
  pa_sink *sink;
  const char *s;
  double tmp_d, old_d;
  int tmp, old;
  size_t nbytes;
  const void *data;

  ENTER();

  sink = voice_get_original_master_sink(u);

  if (!sink)
  {
      pa_log_warn("Original master sink not found, parameters not updated.");
      return;
  }

  u->updating_parameters = TRUE;

  if (!pa_proplist_get(sink->proplist, "x-maemo.xprot.parameters.left", &data, &nbytes))
    xprot_change_params(u->xprot, data, nbytes, 0);

  if (!pa_proplist_get(sink->proplist,"x-maemo.xprot.parameters.right", &data, &nbytes))
      xprot_change_params(u->xprot, data, nbytes, 1);

  s = voice_pa_proplist_gets(sink->proplist, "x-maemo.cmt.ul_timing_advance");
  old = u->ul_timing_advance;
  if (!pa_atoi(s, &tmp) && tmp > -5000 && tmp < 5000)
    u->ul_timing_advance = tmp;
  pa_log_debug("cmt_ul_timing_advance \"%s\" %d %d", s, u->ul_timing_advance, old);

  s = voice_pa_proplist_gets(sink->proplist, "x-maemo.alt_mixer_compensation");
  /* CHECKME */
  old_d = u->alt_mixer_compensation;
  if (!pa_atod(s, &tmp_d) && tmp_d > 0.0 && tmp_d <= 60.0) /* < 60.0 ? */
    u->alt_mixer_compensation = pa_sw_volume_from_dB(tmp_d);
  pa_log_debug("alt_mixer_compensation \"%s\" %d %f", s, u->alt_mixer_compensation, old_d);

  s = voice_pa_proplist_gets(sink->proplist, "x-maemo.ear_ref_padding");
  old = u->ear_ref.loop_padding_usec ;
  if (!pa_atoi(s, &tmp) && tmp > -10000 && tmp < 199999)
    u->ear_ref.loop_padding_usec  = tmp;
  pa_log_debug("ear_ref_padding \"%s\" %d %d", s, u->ear_ref.loop_padding_usec, old);

  voice_parse_aep_steps(u, voice_pa_proplist_gets(sink->proplist, "x-maemo.audio_aep_mb_steps"));

  s = voice_pa_proplist_gets(sink->proplist, "x-maemo.nrec");
  u->nrec_enable = pa_parse_boolean(s);

  if (u->master_source &&
      pa_proplist_gets(u->master_source->proplist, "bluetooth.nrec"))
  {
    /* WTF ?!? */
    u->sidetone_enable = pa_parse_boolean(s) && u->nrec_enable;
  }

  if (!pa_proplist_get(sink->proplist, "x-maemo.aep.switches",
                       (const void **)&data, &nbytes) )
  {
    uint16_t *as = (uint16_t *)data;

    aep_switches.field_0 = as[0];
    aep_switches.field_2 = as[1];
    aep_switches.field_4 = 0;

    if ( aep_switches.field_0 & 0x400 )
      aep_switches.field_4 = 0x30;

    if (aep_switches.field_0 & 1)
      aep_switches.field_4 |= 0x300u;

    aep_switches.field_4 |= 0x1800u;
  }

  if (!pa_proplist_get(sink->proplist, "x-maemo.aep.parameters", &data, &nbytes))
  {
    const char *argv[7] =
    {
      "../execute/d4gnt560",
      "b-ai-1n------0---u",
      "/dev/null",
      "/dev/null",
      "/dev/null",
      "/dev/null",
      "/dev/null",
    };

    if (strlen(aep_runtime_switch) >= strlen(argv[1]))
      argv[1] = aep_runtime_switch;

    fprintf(stderr, "AEP runtime switch %s\n", argv[1]);

    current_aep_tuning = (void *)data;

    init_main(7, argv);
    voice_aep_ear_ref_loop_reset(u);
  }

  sidetone_write_parameters(u);

  if (!pa_proplist_get(sink->proplist, "x-maemo.wb_meq.parameters", &data, &nbytes))
    iir_eq_change_params(u->wb_mic_iir_eq, data, nbytes);

  if (!pa_proplist_get(sink->proplist, "x-maemo.nb_meq.parameters", &data, &nbytes) )
    iir_eq_change_params(u->nb_mic_iir_eq, data, nbytes);

  if (!pa_proplist_get(sink->proplist,"x-maemo.wb_eeq.parameters", &data, &nbytes))
    fir_eq_change_params(u->wb_ear_iir_eq, data, nbytes);

  if (!pa_proplist_get(sink->proplist, "x-maemo.nb_eeq.parameters", &data, &nbytes))
    iir_eq_change_params(u->nb_ear_iir_eq, data, nbytes);

  u->aep_enable = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.aep");
  u->wb_meq_enable = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.wb_meq");
  u->wb_eeq_enable = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.wb_eeq");
  u->nb_meq_enable = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.nb_meq");
  u->nb_eeq_enable = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.nb_eeq");
  u->xprot->displ_limit = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.xprot.displacement");
  u->xprot->temp_limit = voice_pa_proplist_get_bool(sink->proplist, "x-maemo.xprot.temperature");
  u->xprot_enable = u->xprot->displ_limit || u->xprot->temp_limit;

  u->updating_parameters = FALSE;
}

void
voice_sink_proplist_update(struct userdata *u, pa_sink *s)
{
  pa_sink *master_sink;
  const char *mode;
  const char *accessory_hwid;

  pa_proplist *p;
  char *hash_str;
  unsigned int hash;
  const char *file;
  char fname[256];
  size_t nbytes;
  const void *data;

  master_sink = voice_get_original_master_sink(u);

  ENTER();

  if (!master_sink)
  {
    pa_log_warn("Original master sink not found, parameters not loaded.");
    return;
  }

  if (!pa_proplist_get(s->proplist, "x-maemo.aep.trace-func", &data, &nbytes))
    memcpy(&u->trace_func, data, sizeof(u->trace_func));

  mode = pa_proplist_gets(s->proplist, PA_NOKIA_PROP_AUDIO_MODE);

  accessory_hwid = pa_proplist_gets(s->proplist,
                                    PA_NOKIA_PROP_AUDIO_ACCESSORY_HWID);

  if (!accessory_hwid || !mode)
    return;

  if (master_sink != s)
  {
    p = pa_proplist_new();
    pa_proplist_sets(p, PA_NOKIA_PROP_AUDIO_MODE, mode);
    pa_proplist_sets(p, PA_NOKIA_PROP_AUDIO_ACCESSORY_HWID, accessory_hwid);
    pa_proplist_update(master_sink->proplist, PA_UPDATE_REPLACE, p);
    pa_proplist_free(p);
  }

  hash_str = pa_sprintf_malloc("%s%s", mode, accessory_hwid);
  hash = pa_idxset_string_hash_func(hash_str);
  pa_xfree(hash_str);

  if (hash == u->mode_accessory_hwid_hash &&
      !voice_pa_proplist_get_bool(master_sink->proplist, "x-maemo.tuning"))
    return;

  u->mode_accessory_hwid_hash = hash;
  file = pa_proplist_gets(master_sink->proplist, "x-maemo.file");

  if (!file)
    file = "/var/lib/pulse-nokia/%s%s.parameters";

  pa_snprintf(fname, sizeof(fname), file, mode);

  pa_log_debug("Loading tuning parameters from file: %s",fname);

  p = pa_nokia_proplist_from_file(fname);

  if (!pa_proplist_contains(p, "x-maemo.aep") )
  {
    pa_log_warn("Parameter file not valid: %s", fname);
    pa_proplist_free(p);
    return;
  }

  u->btmono = FALSE;

  if (!strcmp(mode, "ihf"))
    aep_runtime_switch[3] = 'i';
  else if (!strcmp(mode, "hs"))
    aep_runtime_switch[3] = 't';
  else if (!strcmp(mode, "btmono"))
  {
    aep_runtime_switch[3] = 't';
    u->btmono = TRUE;
  }
  else if (!strcmp(mode, "hp"))
      aep_runtime_switch[3] = 'p';
  else if (!strcmp(mode, "lineout"))
    aep_runtime_switch[3] = 'f';
  else
    aep_runtime_switch[3] = 't';

  if (master_sink == s)
     pa_proplist_update(master_sink->proplist, PA_UPDATE_REPLACE, p);
  else
    pa_sink_update_proplist(master_sink, PA_UPDATE_REPLACE, p);

  pa_proplist_free(p);
  voice_update_parameters(u);

  return;
}

/*** Deallocate stuff ***/
void voice_clear_up(struct userdata *u) {
    pa_assert(u);

    if (u->mainloop_handler) {
        u->mainloop_handler->parent.free((pa_object *)u->mainloop_handler);
        u->mainloop_handler = NULL;
    }

    if (u->hw_sink_input) {
        pa_sink_input_unlink(u->hw_sink_input);
        pa_sink_input_unref(u->hw_sink_input);
        u->hw_sink_input = NULL;
    }

    if (u->raw_sink) {
        pa_sink_unlink(u->raw_sink);
        pa_sink_unref(u->raw_sink);
        u->raw_sink = NULL;
    }

    if (u->dl_memblockq) {
        pa_memblockq_free(u->dl_memblockq);
        u->dl_memblockq = NULL;
    }

    if (u->voip_sink) {
        pa_sink_unlink(u->voip_sink);
        pa_sink_unref(u->voip_sink);
        u->voip_sink = NULL;
    }

    if (u->hw_source_output) {
        pa_source_output_unlink(u->hw_source_output);
        pa_source_output_unref(u->hw_source_output);
        u->hw_source_output = NULL;
    }

    if (u->voip_source) {
        pa_source_unlink(u->voip_source);
        pa_source_unref(u->voip_source);
        u->voip_source = NULL;
    }

    if (u->raw_source) {
        pa_source_unlink(u->raw_source);
        pa_source_unref(u->raw_source);
        u->raw_source = NULL;
    }

    if (u->hw_source_memblockq) {
        pa_memblockq_free(u->hw_source_memblockq);
        u->hw_source_memblockq = NULL;
    }

    if (u->ul_memblockq) {
        pa_memblockq_free(u->ul_memblockq);
        u->ul_memblockq = NULL;
    }

    if (u->dl_sideinfo_queue) {
        pa_queue_free(u->dl_sideinfo_queue, NULL, u);
        u->dl_sideinfo_queue = NULL;
    }

    voice_aep_ear_ref_unload(u);

    if (u->aep_silence_memchunk.memblock) {
        pa_memblock_unref(u->aep_silence_memchunk.memblock);
        pa_memchunk_reset(&u->aep_silence_memchunk);
    }

    if (u->sink_temp_buff) {
        pa_xfree(u->sink_temp_buff);
        u->sink_temp_buff = NULL;
    }

    if (u->sink_subscription) {
        pa_subscription_free(u->sink_subscription);
        u->sink_subscription = NULL;
    }

    if (u->sink_proplist_changed_slot) {
        pa_hook_slot_free(u->sink_proplist_changed_slot);
        u->sink_proplist_changed_slot = NULL;
    }

    if (u->source_proplist_changed_slot) {
        pa_hook_slot_free(u->source_proplist_changed_slot);
        u->source_proplist_changed_slot = NULL;
    }

    voice_convert_free(u);
    voice_memchunk_pool_unload(u);
    voice_unload_event_forwarder(u);
}

static voice_memchunk_pool *voice_memchunk_pool_table = NULL;
void voice_memchunk_pool_load(struct userdata *u) {
    int i;

    pa_assert(0 == offsetof(voice_memchunk_pool, chunk));
    pa_atomic_ptr_store(&u->memchunk_pool, NULL);

    voice_memchunk_pool_table = pa_xmalloc0(sizeof(voice_memchunk_pool)*VOICE_MEMCHUNK_POOL_SIZE);
    pa_assert(voice_memchunk_pool_table);

    for (i = 0; i<VOICE_MEMCHUNK_POOL_SIZE; i++)
        voice_memchunk_pool_free(u, (pa_memchunk *)&voice_memchunk_pool_table[i]);
}

void voice_memchunk_pool_unload(struct userdata *u) {
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

/* Generic source state change logic. Used by raw_source and voice_source */
int voice_source_set_state(pa_source *s, pa_source *other, pa_source_state_t state) {
    struct userdata *u;

    pa_source_assert_ref(s);
    pa_assert_se(u = s->userdata);
    if (!other) {
        pa_log_debug("other source not initialized or freed");
        return 0;
    }
    pa_source_assert_ref(other);

    if (u->hw_source_output) {
        if (pa_source_output_get_state(u->hw_source_output) == PA_SOURCE_OUTPUT_RUNNING) {
            if (state == PA_SOURCE_SUSPENDED &&
                pa_source_get_state(other) == PA_SOURCE_SUSPENDED &&
                pa_atomic_load(&u->cmt_connection.ul_state) != CMT_UL_ACTIVE) {
                pa_source_output_cork(u->hw_source_output, TRUE);
                pa_log_debug("hw_source_output corked");
            }
        }
        else if (pa_source_output_get_state(u->hw_source_output) == PA_SOURCE_OUTPUT_CORKED) {
            if (PA_SOURCE_IS_OPENED(state) ||
                PA_SOURCE_IS_OPENED(pa_source_get_state(other)) ||
                pa_atomic_load(&u->cmt_connection.ul_state) == CMT_UL_ACTIVE) {
                pa_source_output_cork(u->hw_source_output, FALSE);
                pa_log_debug("hw_source_output uncorked");
            }
        }
    }
    if (pa_atomic_load(&u->cmt_connection.ul_state) != CMT_UL_ACTIVE && 
        !PA_SOURCE_IS_OPENED(pa_source_get_state(u->voip_source)))
    {
        voice_aep_ear_ref_loop_reset(u);
    }
    return 0;
}

/* Generic sink state change logic. Used by raw_sink and voip_sink */
int voice_sink_set_state(pa_sink *s, pa_sink *other, pa_sink_state_t state) {
    struct userdata *u;
    pa_sink *om_sink;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);
    if (!other) {
        pa_log_debug("other sink not initialized or freed");
        return 0;
    }
    pa_sink_assert_ref(other);
    om_sink = u->master_sink;

    if (u->hw_sink_input && PA_SINK_INPUT_IS_LINKED(pa_sink_input_get_state(u->hw_sink_input))) {
        if (pa_sink_input_get_state(u->hw_sink_input) == PA_SINK_INPUT_CORKED) {
            if (PA_SINK_IS_OPENED(state) ||
                PA_SINK_IS_OPENED(pa_sink_get_state(other)) ||
                pa_atomic_load(&u->cmt_connection.dl_state) == CMT_DL_ACTIVE) {
                pa_sink_input_cork(u->hw_sink_input, FALSE);
                pa_log_debug("hw_sink_input uncorked");
            }
        }
        else {
            if (state == PA_SINK_SUSPENDED &&
                pa_sink_get_state(other) == PA_SINK_SUSPENDED &&
                pa_atomic_load(&u->cmt_connection.dl_state) != CMT_DL_ACTIVE) {
                pa_sink_input_cork(u->hw_sink_input, TRUE);
                pa_log_debug("hw_sink_input corked");
            }
        }
    }

    if (om_sink == NULL) {
        pa_log_info("No master sink, assuming primary mixer tuning.\n");
        pa_atomic_store(&u->mixer_state, PROP_MIXER_TUNING_PRI);
    }
    else if (pa_atomic_load(&u->cmt_connection.dl_state) == CMT_DL_ACTIVE ||
            (pa_sink_get_state(u->voip_sink) <= PA_SINK_SUSPENDED &&
             voice_voip_sink_used_by(u))) {
        if (pa_atomic_load(&u->mixer_state) == PROP_MIXER_TUNING_PRI) {
             pa_proplist *p = pa_proplist_new();
             pa_assert(p);
             pa_proplist_sets(p, PROP_MIXER_TUNING_MODE, PROP_MIXER_TUNING_ALT_S);
             pa_sink_update_proplist(om_sink, PA_UPDATE_REPLACE, p);
             pa_atomic_store(&u->mixer_state, PROP_MIXER_TUNING_ALT);
             pa_proplist_free(p);
             if (u->sidetone_enable)
                 voice_enable_sidetone(u,1);
        }
    }
    else {
        if (pa_atomic_load(&u->mixer_state) == PROP_MIXER_TUNING_ALT) {
            pa_proplist *p = pa_proplist_new();
            pa_assert(p);
            pa_proplist_sets(p, PROP_MIXER_TUNING_MODE, PROP_MIXER_TUNING_PRI_S);
            pa_sink_update_proplist(om_sink, PA_UPDATE_REPLACE, p);
            pa_atomic_store(&u->mixer_state, PROP_MIXER_TUNING_PRI);
            pa_proplist_free(p);
            voice_enable_sidetone(u,0);

        }
    }

    return 0;
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

pa_sink *voice_get_original_master_sink(struct userdata *u) {
    const char *om_name;
    pa_sink *om_sink;
    pa_assert(u);
    pa_assert(u->modargs);
    pa_assert(u->core);
    om_name = pa_modargs_get_value(u->modargs, "master_sink", NULL);
    if (!om_name) {
        pa_log_error("Master sink name not found from modargs!");
        return NULL;
    }
    if (!(om_sink = pa_namereg_get(u->core, om_name, PA_NAMEREG_SINK))) {
        pa_log_error("Original master sink \"%s\" not found", om_name);
        return NULL;
    }
    return om_sink;
}

pa_source *voice_get_original_master_source(struct userdata *u) {
    const char *om_name;
    pa_source *om_source;
    pa_assert(u);
    pa_assert(u->modargs);
    pa_assert(u->core);
    om_name = pa_modargs_get_value(u->modargs, "master_source", NULL);
    if (!om_name) {
        pa_log_error("Master source name not found from modargs!");
        return NULL;
    }
    if (!(om_source = pa_namereg_get(u->core, om_name, PA_NAMEREG_SOURCE))) {
        pa_log_error("Original master source \"%s\" not found", om_name);
        return NULL;
    }
    return om_source;
}

#ifdef EXTRA_DEBUG
/* This code is not finished and probably dont work. */

struct file_table_entry {
    const char *name;
    FILE *file;
};

#define FILE_TABLE_SIZE 16
static const int file_table_size = FILE_TABLE_SIZE;
static struct file_table_entry file_table[FILE_TABLE_SIZE] = { { NULL, NULL } };

void voice_append_chunk_to_file(struct userdata *u, const char *file_name, pa_memchunk *chunk) {
    int i;
    FILE *file = NULL;
    for (i = 0; i < file_table_size && file_table[i].name != NULL; i++) {
        if (0 == strcmp(file_name, file_table[i].name)) {
            file = file_table[i].file;
        }
    }

    if (i >= file_table_size) {
        pa_log("Can't open new files, not writing to file \"%s\"", file_name);
        return;
    }

    if (file == NULL) {
        file = file_table[i].file = fopen(file_name, "w");
        if (file == NULL) {
            pa_log("Can't open file \"%s\": %s", file_name, strerror(errno));
            return;
        }
    }

    char *p = pa_memblock_acquire(chunk->memblock);
    fwrite(p + chunk->index, 1, chunk->length, file);
    pa_memblock_release(chunk->memblock);
}

#endif
