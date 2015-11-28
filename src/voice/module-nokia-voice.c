#include "module-voice-userdata.h"
#include "module-nokia-voice-symdef.h"
#include "proplist-nokia.h"
#include "voice-sidetone.h"
#include "voice-util.h"
#include "voice-aep-ear-ref.h"
#include "voice-raw-sink.h"
#include "voice-raw-source.h"
#include "voice-convert.h"
#include "voice-mainloop-handler.h"
#include "voice-voip-sink.h"
#include "voice-voip-source.h"
#include "voice-hw-sink-input.h"
#include "voice-hw-source-output.h"
#include "voice-cmtspeech.h"
#include "voice-event-forwarder.h"
#include "voice-aep-sink-input.h"

#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>

PA_MODULE_AUTHOR("Jyri Sarha");
PA_MODULE_DESCRIPTION("Nokia voice module");
PA_MODULE_USAGE("voice_sink_name=<name for the voice sink> "
                "voice_source_name=<name for the voice source> "
                "master_sink=<sink to connect to> "
                "master_source=<source to connect to> "
                "raw_sink=<name for raw sink> "
                "raw_source=<name for raw source> "
                "dbus_type=<system|session> "
                "max_hw_frag_size=<maximum fragment size of master sink and source in usecs>");

PA_MODULE_VERSION(PACKAGE_VERSION);

static const char* const valid_modargs[] =
{
  "voice_sink_name",
  "voice_source_name",
  "master_sink",
  "master_source",
  "raw_sink_name",
  "raw_source_name",
  "dbus_type"
  "max_hw_frag_size",
  NULL,
};

static pa_hook_result_t sink_proplist_changed_cb(pa_core *c,pa_sink *s,struct userdata *u)
{
  if (u->master_sink == s)
  {
    voice_sink_proplist_update(u,s);
  }
  return PA_HOOK_OK;
}

static pa_hook_result_t source_proplist_changed_cb(pa_core *c,pa_source *s,struct userdata *u)
{
  if (u->master_source == s)
  {
    voice_update_parameters(u);
  }
  return PA_HOOK_OK;
}

void sink_subscribe_cb(pa_core *c,pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  //todo address 0x0000E4FC
}

int pa__init(pa_module *m)
{
  pa_modargs *ma;
  const char *master_sink_name;
  const char *master_source_name;
  const char *max_hw_frag_size_str;
  const char *aep_runtime;
  pa_source *master_source;
  struct userdata *u;
  pa_proplist *p;
  pa_sink *master_sink;
  const char *raw_sink_name;
  const char *raw_source_name;
  const char *voice_sink_name;
  const char *voice_source_name;
  const char *dbus_type;
  int max_hw_frag_size = 3840;

  pa_assert(m);

  if (!(ma = pa_modargs_new(m->argument, valid_modargs)))
  {
      pa_log_error("Failed to parse module arguments");
      goto fail;
  }

  voice_turn_sidetone_down();
  master_sink_name = pa_modargs_get_value(ma, "master_sink", NULL);
  master_source_name = pa_modargs_get_value(ma, "master_source", NULL);
  raw_sink_name = pa_modargs_get_value(ma, "raw_sink_name", "sink.voice.raw");
  raw_source_name = pa_modargs_get_value(ma, "raw_source_name",
                                         "source.voice.raw");
  voice_sink_name = pa_modargs_get_value(ma, "voice_sink_name", "sink.voice");
  voice_source_name = pa_modargs_get_value(ma, "voice_source_name",
                                           "source.voice");
  dbus_type = pa_modargs_get_value(ma, "dbus_type", "session");
  max_hw_frag_size_str = pa_modargs_get_value(ma, "max_hw_frag_size", "3840");
  aep_runtime = pa_modargs_get_value(ma, "aep_runtime",
                                     "bbaid1n-wr0-h9a22b--dbxpb--");
  voice_set_aep_runtime_switch(aep_runtime);
  pa_log_debug("Got arguments: master_sink=\"%s\" master_source=\"%s\" raw_sink_name=\"%s\" raw_source_name=\"%s\" dbus_type=\"%s\" max_hw_frag_size=\"%s\". ",
               master_sink_name, master_source_name, raw_sink_name,
               raw_source_name, dbus_type, max_hw_frag_size_str);

  if (!(master_sink = pa_namereg_get(m->core, master_sink_name, PA_NAMEREG_SINK)))
  {
    pa_log("Master sink \"%s\" not found", master_sink_name);
    goto fail;
  }

  if (!(master_source = pa_namereg_get(m->core, master_source_name, PA_NAMEREG_SOURCE)))
  {
    pa_log( "Master source \"%s\" not found", master_source_name);
    goto fail;
  }

  if (master_sink->sample_spec.format != master_source->sample_spec.format &&
      master_sink->sample_spec.rate != master_source->sample_spec.rate &&
      master_sink->sample_spec.channels != master_source->sample_spec.channels)
  {
    pa_log("Master source and sink must have same sample spec");
    goto fail;
  }

  if (pa_atoi(max_hw_frag_size_str, &max_hw_frag_size) < 0 ||
      max_hw_frag_size < 960 ||
      max_hw_frag_size > 128*960)
  {
    pa_log("Bad value for max_hw_frag_size: %s", max_hw_frag_size_str);
    goto fail;
  }

  m->userdata = u = pa_xnew0(struct userdata, 1);
  u->core = m->core;
  u->module = m;
  u->modargs = ma;
  u->master_sink = master_sink;
  u->master_source = master_source;
  u->mainloop_handler = voice_mainloop_handler_new(u);;
  u->ul_timing_advance = 500;  // = 500 micro seconds, seems to be a good default value

  pa_channel_map_init_mono(&u->mono_map);
  pa_channel_map_init_stereo(&u->stereo_map);

  u->hw_sample_spec.format = PA_SAMPLE_S16NE;
  u->hw_sample_spec.rate = SAMPLE_RATE_HW_HZ;
  u->hw_sample_spec.channels = 2;

  u->hw_mono_sample_spec.format = PA_SAMPLE_S16NE;
  u->hw_mono_sample_spec.rate = SAMPLE_RATE_HW_HZ;
  u->hw_mono_sample_spec.channels = 1;

  u->aep_sample_spec.format = PA_SAMPLE_S16NE;
  u->aep_sample_spec.rate = SAMPLE_RATE_AEP_HZ;
  u->aep_sample_spec.channels = 1;
  pa_channel_map_init_mono(&u->aep_channel_map);

  // The result is rounded down incorrectly thus +1
  u->aep_fragment_size = pa_usec_to_bytes(PERIOD_AEP_USECS+1, &u->aep_sample_spec);
  u->aep_hw_fragment_size = pa_usec_to_bytes(PERIOD_AEP_USECS+1, &u->hw_sample_spec);
  u->hw_fragment_size = pa_usec_to_bytes(PERIOD_MASTER_USECS+1, &u->hw_sample_spec);
  u->hw_fragment_size_max = max_hw_frag_size;
  if (0 != (u->hw_fragment_size_max % u->hw_fragment_size))
      u->hw_fragment_size_max += u->hw_fragment_size - (u->hw_fragment_size_max % u->hw_fragment_size);
  u->aep_hw_mono_fragment_size = pa_usec_to_bytes(PERIOD_AEP_USECS+1, &u->hw_mono_sample_spec);
  u->hw_mono_fragment_size = pa_usec_to_bytes(PERIOD_MASTER_USECS+1, &u->hw_mono_sample_spec);

  u->voice_ul_fragment_size = pa_usec_to_bytes(PERIOD_CMT_USECS+1, &u->aep_sample_spec);

  pa_silence_memchunk_get(&u->core->silence_cache,
                          u->core->mempool,
                          &u->aep_silence_memchunk,
                          &u->aep_sample_spec,
                          u->aep_fragment_size);
  voice_memchunk_pool_load(u);

  if (voice_init_raw_sink(u, raw_sink_name))
    goto fail;

  pa_sink_put(u->raw_sink);

  if (voice_init_voip_sink(u, voice_sink_name))
    goto fail;

  pa_sink_put(u->voip_sink);

  if (voice_init_aep_sink_input(u))
    goto fail;

  pa_atomic_store(&u->mixer_state, PROP_MIXER_TUNING_PRI);
  u->alt_mixer_compensation = PA_VOLUME_NORM;

  if (voice_init_hw_sink_input(u))
    goto fail;

  u->sink_temp_buff = pa_xmalloc(2 * u->hw_fragment_size_max);
  u->sink_temp_buff_len = 2 * u->hw_fragment_size_max;

  u->dl_memblockq =
          pa_memblockq_new(0, 2 * u->voice_ul_fragment_size, 0,
                           pa_frame_size(&u->aep_sample_spec), 0, 0, 0, NULL);

  if (voice_init_raw_source(u, raw_source_name))
    goto fail;

  pa_source_put(u->raw_source);

  if (voice_init_voip_source(u, voice_source_name))
    goto fail;

  pa_source_put(u->voip_source);

  if (voice_init_hw_source_output(u))
    goto fail;

  u->hw_source_memblockq =
      pa_memblockq_new(0, 2 * u->hw_fragment_size_max, 0,
                       pa_frame_size(&u->hw_sample_spec), 0, 0, 0, NULL);

  u->ul_memblockq =
      pa_memblockq_new(0, 2 * u->voice_ul_fragment_size, 0,
                       pa_frame_size(&u->aep_sample_spec), 0, 0, 0, NULL);

  u->cs_call_sink_input = 0;
  u->dl_sideinfo_queue = pa_queue_new();

  u->linear_q15_master_volume_L = INT16_MAX;
  u->linear_q15_master_volume_R = INT16_MAX;
  u->field_2CC = 0;

  voice_aep_ear_ref_init(u);

  if (voice_convert_init(u))
    goto fail;

  if (voice_init_event_forwarder(u, dbus_type) || voice_init_cmtspeech(u))
    goto fail;

  if (!(u->wb_mic_iir_eq = iir_eq_new(u->hw_fragment_size / 2,
                              master_source->sample_spec.channels)))
      goto fail;

  if (!(u->nb_mic_iir_eq = iir_eq_new( u->aep_fragment_size / 2, 1)))
    goto fail;

  if (!(u->wb_ear_iir_eq = fir_eq_new(master_sink->sample_spec.rate,
                                      master_sink->sample_spec.channels)))
    goto fail;

  if (!(u->nb_ear_iir_eq = iir_eq_new(u->aep_fragment_size / 2, 1)))
    goto fail;

  u->input_task_active = FALSE;
  u->xprot_watchdog = TRUE;
  u->ambient_temp = 30;
  if (!(u->xprot = xprot_new()))
    goto fail;

  u->aep_enable = FALSE;
  u->wb_meq_enable = FALSE;
  u->wb_eeq_enable = FALSE;
  u->nb_meq_enable = FALSE;
  u->nb_eeq_enable = FALSE;
  u->xprot_enable = FALSE;
  u->field_414 = 0;  

  u->sink_proplist_changed_slot =
      pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_PROPLIST_CHANGED],
                              0, (pa_hook_cb_t)sink_proplist_changed_cb, u);;

  u->source_proplist_changed_slot =
      pa_hook_connect( &m->core->hooks[PA_CORE_HOOK_SOURCE_PROPLIST_CHANGED], 0,
              (pa_hook_cb_t)source_proplist_changed_cb, u);
  u->hash = 0;

  p = pa_proplist_new();
  pa_proplist_sets(p, PA_NOKIA_PROP_AUDIO_MODE, "ihf");
  pa_proplist_sets(p, PA_NOKIA_PROP_AUDIO_ACCESSORY_HWID, "");

  pa_sink_update_proplist( master_sink, PA_UPDATE_REPLACE, p);

  pa_proplist_free(p);

  pa_source_output_put(u->hw_source_output);
  pa_sink_input_put(u->hw_sink_input);
  pa_sink_input_put(u->aep_sink_input);

  u->sink_subscription = pa_subscription_new(
        m->core,
        PA_SUBSCRIPTION_MASK_SINK,
        sink_subscribe_cb,
        u);
  return 0;

fail:
  if (ma)
    pa_modargs_free(ma);

  pa__done(m);

  return -1;
}


void pa__done(pa_module *m)
{
  struct userdata *u;
  pa_modargs *ma;

  u = m->userdata;
  if (u)
  {
    voice_shutdown_aep();
    voice_enable_sidetone(u, 0);
    voice_clear_up(u);
    fir_eq_free(u->wb_ear_iir_eq);
    iir_eq_free(u->nb_ear_iir_eq);
    iir_eq_free(u->wb_mic_iir_eq);
    iir_eq_free(u->nb_mic_iir_eq);
    xprot_free(u->xprot);
    ma = (pa_modargs *)u->modargs;

    if (ma)
      pa_modargs_free(ma);

    pa_xfree(u);
  }
}
