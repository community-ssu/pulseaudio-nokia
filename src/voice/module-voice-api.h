/*
 * This file is part of pulseaudio-nokia
 *
 * Copyright (C) 2010 Nokia Corporation. All rights reserved.
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
#ifndef module_voice_api_h
#define module_voice_api_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/sink.h>
#include <pulsecore/source.h>

#define VOICE_SOURCE_FRAMESIZE (20000) /* us */
#define VOICE_SINK_FRAMESIZE (10000) /* us */

#define VOICE_API_VERSION "0.1"

#define VOICE_HOOK_HW_SINK_PROCESS              "x-nokia.voice.hw_sink_process"
#define VOICE_HOOK_NARROWBAND_EAR_EQU_MONO      "x-nokia.voice.narrowband_ear_equ_mono"
#define VOICE_HOOK_NARROWBAND_MIC_EQ_MONO       "x-nokia.voice.narrowband_mic_eq_mono"
#define VOICE_HOOK_WIDEBAND_MIC_EQ_MONO         "x-nokia.voice.wideband_mic_eq_mono"
#define VOICE_HOOK_XPROT_MONO                   "x-nokia.voice.xport_mono"
#define VOICE_HOOK_VOLUME                       "x-nokia.voice.volume"
#define VOICE_HOOK_CALL_VOLUME                  "x-nokia.voice.call_volume"
#define VOICE_HOOK_CALL_BEGIN                   "x-nokia.voice.call_begin"
#define VOICE_HOOK_CALL_END                     "x-nokia.voice.call_end"
#define VOICE_HOOK_AEP_DOWNLINK                 "x-nokia.voice.aep_downlink"
#define VOICE_HOOK_AEP_UPLINK                   "x-nokia.voice.aep_uplink"

typedef struct {
    pa_memchunk *chunk;
    pa_memchunk *rchunk;
} aep_uplink;

typedef struct {
    pa_memchunk *chunk;
    int spc_flags;
    pa_bool_t cmt;
} aep_downlink;

enum {
    /* TODO: Print out BIG warning if in wrong buffer mode when this message is received */
    VOICE_SOURCE_SET_UL_DEADLINE = PA_SOURCE_MESSAGE_MAX + 100,
};

enum {
    VOICE_SINK_GET_SIDE_INFO_QUEUE_PTR = PA_SINK_MESSAGE_MAX + 100,
};


#define PA_PROP_SINK_API_EXTENSION_PROPERTY_NAME "sink.api-extension.nokia.voice"
#define PA_PROP_SINK_API_EXTENSION_PROPERTY_VALUE VOICE_API_VERSION

#define PA_PROP_SOURCE_API_EXTENSION_PROPERTY_NAME "source.api-extension.nokia.voice"
#define PA_PROP_SOURCE_API_EXTENSION_PROPERTY_VALUE VOICE_API_VERSION

/* Because pa_queue does not like NULL pointers, this flag is added to every
 * set of flags to make the pa_queue entries non null. */
#define VOICE_SIDEINFO_FLAG_BOGUS (0x8000)
/* TODO: Create voice API for SPC flags. Voice module should not use cmtspeech headers. */

#endif /* module_voice_api_h */
