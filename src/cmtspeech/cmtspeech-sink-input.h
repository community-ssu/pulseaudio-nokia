/*
 * This file is part of pulseaudio-nokia
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
#ifndef voice_hw_sink_input_h
#define voice_hw_sink_input_h

enum {
    PA_SINK_INPUT_MESSAGE_FLUSH_DL = PA_SINK_INPUT_MESSAGE_MAX + 1,
};

int cmtspeech_create_sink_input(struct userdata *u);
void cmtspeech_delete_sink_input(struct userdata *u);

#endif //voice_hw_sink_input_h
