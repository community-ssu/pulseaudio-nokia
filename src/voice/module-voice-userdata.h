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
#ifndef module_voice_userdata_h
#define module_voice_userdata_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <pulsecore/modargs.h>
#include <pulsecore/sink-input.h>
#include <pulsecore/source-output.h>
#include <pulsecore/module.h>
#include <pulsecore/thread.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/semaphore.h>
#include <pulsecore/fdsem.h>
#include <pulsecore/dbus-shared.h>

#include <dbus/dbus.h>

#ifdef HAVE_LIBCMTSPEECHDATA
#include <cmtspeech.h>
#endif

#include "src/common/src-48-to-8.h"
#include "src/common/src-8-to-48.h"

#include <voice-buffer.h>
#include "xprot.h"
#include "eq_iir.h"
#include "eq_fir.h"

/* This is a copy/paste from module-alsa-sink-volume.c, keep it up to date!*/
/* String with single integer defining which mixer
 * tuning table is used. Currently only two different tables
 * can be defined.
 */
#define PROP_MIXER_TUNING_MODE "x-maemo.alsa_sink.mixer_tuning_mode"
#define PROP_MIXER_TUNING_PRI (0)
#define PROP_MIXER_TUNING_ALT (1)

#define PROP_MIXER_TUNING_PRI_S "0"
#define PROP_MIXER_TUNING_ALT_S "1"

struct voice_aep_ear_ref
{
  int loop_padding_usec;
  pa_atomic_t loop_state;
  volatile struct timeval loop_tstamp;
  pa_asyncq *loop_asyncq;
  pa_memblockq *loop_memblockq;
};

struct cmtspeech_dbus_conn
{
  DBusBusType dbus_type;
  pa_dbus_connection *dbus_conn;
  char *dbus_match_rules[32];
};

struct cmtspeech_connection
{
  pa_msgobject *cmt_handler;
  pa_atomic_t thread_state;
  pa_fdsem *thread_state_change;
  pa_atomic_t ul_state;
  pa_atomic_t dl_state;
  pa_semaphore *cmtspeech_semaphore;
  void *cmtspeech;
  pa_mutex *cmtspeech_mutex;
  pa_rtpoll *rtpoll;
  pa_rtpoll_item *cmt_poll_item;
  pa_rtpoll_item *thread_state_poll_item;
  pa_thread *thread;
  pa_thread_mq thread_mq;
  pa_mutex *ul_timing_mutex;
  struct timeval deadline;
  pa_asyncq *dl_frame_queue;
  pa_bool_t call_ul;
  pa_bool_t call_dl;
  pa_bool_t call_emergency;
  pa_bool_t first_dl_frame_received;
  pa_bool_t record_running;
  pa_bool_t playback_running;
};

/* TODO: Classify each member according to which thread they are used from */
struct userdata
{
  pa_core *core;
  pa_module *module;
  pa_modargs *modargs;
  pa_msgobject *mainloop_handler;
  int ul_timing_advance;
  pa_channel_map mono_map;
  pa_channel_map stereo_map;
  pa_sample_spec hw_sample_spec;
  pa_sample_spec hw_mono_sample_spec;
  pa_sample_spec aep_sample_spec;
  pa_channel_map aep_channel_map;
  size_t aep_fragment_size;
  size_t aep_hw_fragment_size;
  size_t hw_fragment_size;
  size_t hw_fragment_size_max;
  size_t hw_mono_fragment_size;
  size_t aep_hw_mono_fragment_size;
  size_t voice_ul_fragment_size;
  pa_memchunk aep_silence_memchunk;
  pa_atomic_ptr_t memchunk_pool;
  pa_sink *master_sink;
  pa_source *master_source;
  pa_sink *raw_sink;
  pa_sink *voip_sink;
  pa_sink_input *hw_sink_input;
  pa_hook_slot *hw_sink_input_move_fail_slot;
  pa_hook_slot *hw_sink_input_move_finish_slot;
  pa_atomic_t mixer_state;
  pa_volume_t alt_mixer_compensation;
  void *sink_temp_buff;
  size_t sink_temp_buff_len;
  pa_memblockq *unused_memblockq;
  pa_sink_input *aep_sink_input;
  pa_source *raw_source;
  pa_source *voip_source;
  pa_source_output *hw_source_output;
  pa_hook_slot *hw_source_output_move_fail_slot;
  pa_memblockq *hw_source_memblockq;
  pa_memblockq *ul_memblockq;
  pa_sink_input *cs_call_sink_input;
  int16_t linear_q15_master_volume_R;
  int16_t linear_q15_master_volume_L;
  int steps; //the below unknowns hold aep volume step information and are referenced by voice_parse_aep_steps and voice_pa_vol_to_aep_step
  int field_248;
  int field_24C;
  int field_250;
  int field_254;
  int field_258;
  int field_25C;
  int field_260;
  int field_264;
  int field_268;
  int field_26C;
  int field_270;
  int field_274;
  int field_278;
  int field_27C;
  int field_280;
  int field_284;
  int field_288;
  int field_28C;
  int field_290;
  int field_294;
  int field_298;
  int field_29C;
  int field_2A0;
  int field_2A4;
  int field_2A8;
  int field_2AC;
  int field_2B0;
  int field_2B4;
  int field_2B8;
  int field_2BC;
  int field_2C0;
  int field_2C4;
  pa_queue *dl_sideinfo_queue;
  pa_bool_t field_2CC;
  char gap_2CD[3];
  src_48_to_8 *hw_source_to_aep_resampler;
  src_8_to_48 *aep_to_hw_sink_resampler;
  src_48_to_8 *ear_to_aep_resampler;
  src_48_to_8 *raw_sink_to_hw8khz_sink_resampler;
  src_8_to_48 *hw8khz_source_to_raw_source_resampler;
  struct voice_aep_ear_ref ear_ref;
  struct cmtspeech_dbus_conn cmt_dbus_conn;
  struct cmtspeech_connection cmt_connection;
  char gap_3DE[2];
  struct iir_eq *wb_mic_iir_eq;
  void *wb_ear_iir_eq; //should be fir_eq *
  struct iir_eq *nb_mic_iir_eq;
  struct iir_eq *nb_ear_iir_eq;
  xprot* xprot;
  pa_bool_t input_task_active:1;
  pa_bool_t xprot_watchdog:1;
  char gap_3f5[3];
  int ambient_temp;
  pa_bool_t sidetone_enable:1;
  pa_bool_t aep_enable:1;
  pa_bool_t btmono_unk:1;
  pa_bool_t nrec_enable:1;
  pa_bool_t wb_meq_enable:1;
  pa_bool_t wb_eeq_enable:1;
  pa_bool_t nb_meq_enable:1;
  pa_bool_t nb_eeq_enable:1;
  pa_bool_t xprot_enable:1;
  char gap_3FE[2];
  pa_hook_slot *sink_proplist_changed_slot;
  pa_hook_slot *source_proplist_changed_slot;
  pa_subscription *sink_subscription;
  void *trace_func; //unknown format
  unsigned int hash; //set to the result of pa_idxset_hash_func, not sure what its hashing
  pa_bool_t field_414;
  char gap_415[3];
};

#endif /* module_voice_userdata_h */
