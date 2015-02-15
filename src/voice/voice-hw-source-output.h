/*
 * This file is part of pulseaudio-meego
 *
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
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
#ifndef voice_hw_source_output_h
#define voice_hw_source_output_h

#include "module-voice-userdata.h"
#include <pulse/sample.h>
#include <pulse/channelmap.h>
#include <pulsecore/source.h>
#include <pulsecore/source-output.h>

int voice_init_hw_source_output(struct userdata *u);
void voice_reinit_hw_source_output(struct userdata *u);

#endif //voice_hw_source_output_h
