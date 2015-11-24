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
#include "voice-cs-call-sink-input.h"
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

static void cs_call_sink_input_process_rewind_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

static int cs_call_sink_input_pop_cb(pa_sink_input *i, size_t length, pa_memchunk *chunk) {
    struct userdata *u;
    pa_assert_fp(i);
    pa_sink_input_assert_ref(i);
    pa_assert_fp(chunk);
    u = i->userdata;
    pa_silence_memchunk_get(&u->core->silence_cache, u->core->mempool, chunk, &i->sample_spec, length);
    return 0;
}

static void cs_call_sink_input_state_change_cb(pa_sink_input *i, pa_sink_input_state_t state) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

static void cs_call_sink_input_kill_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_log_debug("Kill called");

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    pa_assert(u->cs_call_sink_input == i);

    u->cs_call_sink_input->thread_info.attached = FALSE;
    pa_sink_input_unlink(u->cs_call_sink_input);
    pa_sink_input_unref(u->cs_call_sink_input);
    u->cs_call_sink_input = NULL;
    pa_module_unload_request(u->module, TRUE);
}

static void cs_call_sink_input_attach_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_log_debug("Attach called, new master %p %s",(void *)i->sink,i->sink->name);
}

static void cs_call_sink_input_detach_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_log_debug("Detach called");
}

static void cs_call_sink_input_update_sink_latency_range_cb(pa_sink_input *i) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

static void cs_call_sink_input_update_max_request_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

static void cs_call_sink_input_update_max_rewind_cb(pa_sink_input *i, size_t nbytes) {
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);
    ENTER();
}

int voice_create_cs_call_sink_input(struct userdata *u)
{
  pa_sink_input_new_data data;
  char t[256];

  pa_assert(u);
  ENTER();

  if (u->cs_call_sink_input)
  {
    pa_log_warn("Create called but input already exists");
    return 1;
  }

  pa_sink_input_new_data_init(&data);
  snprintf(t, sizeof(t), "Cellular call (dummy)");
  pa_proplist_sets(data.proplist, PA_PROP_MEDIA_NAME, t);
  snprintf(t, sizeof(t), "phone");
  pa_proplist_sets(data.proplist, PA_PROP_MEDIA_ROLE, t);
  data.module = u->module;
  data.sink = u->voip_sink;
  data.driver = __FILE__;
  data.origin_sink = 0;
  pa_sink_input_new_data_set_sample_spec(&data, &u->voip_sink->sample_spec);
  pa_sink_input_new_data_set_channel_map(&data, &u->voip_sink->channel_map);
  pa_sink_input_new(&u->cs_call_sink_input, u->core, &data, PA_SINK_INPUT_DONT_MOVE);
  pa_sink_input_new_data_done(&data);
  if (!u->cs_call_sink_input)
  {
    pa_log_warn("Creating sink input failed");
    return -1;
  }

  u->cs_call_sink_input->pop = cs_call_sink_input_pop_cb;
  u->cs_call_sink_input->process_rewind = cs_call_sink_input_process_rewind_cb;
  u->cs_call_sink_input->update_max_rewind = cs_call_sink_input_update_max_rewind_cb;
  u->cs_call_sink_input->update_max_request = cs_call_sink_input_update_max_request_cb;
  u->cs_call_sink_input->update_sink_latency_range = cs_call_sink_input_update_sink_latency_range_cb;
  u->cs_call_sink_input->kill = cs_call_sink_input_kill_cb;
  u->cs_call_sink_input->attach = cs_call_sink_input_attach_cb;
  u->cs_call_sink_input->detach = cs_call_sink_input_detach_cb;
  u->cs_call_sink_input->state_change = cs_call_sink_input_state_change_cb;
  u->cs_call_sink_input->userdata = u;

  pa_sink_input_put(u->cs_call_sink_input);
  return 0;
}

void voice_delete_cs_call_sink_input(struct userdata *u) {
    pa_assert(u);
    ENTER();

    pa_sink_input_unlink(u->cs_call_sink_input);
    pa_sink_input_unref(u->cs_call_sink_input);
    u->cs_call_sink_input = NULL;
}

