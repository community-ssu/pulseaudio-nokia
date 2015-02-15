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

#ifndef _VOICE_MAINLOOP_HANDLER_H_
#define _VOICE_MAINLOOP_HANDLER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/msgobject.h>
#include "module-voice-userdata.h"

typedef struct voice_mainloop_handler {
    pa_msgobject parent;
    struct userdata *u;
} voice_mainloop_handler;

PA_DECLARE_CLASS(voice_mainloop_handler);
#define VOICE_MAINLOOP_HANDLER(o) voice_mainloop_handler_cast(o)

enum {
    VOICE_MAINLOOP_HANDLER_EXECUTE,
    VOICE_MAINLOOP_HANDLER_MESSAGE_MAX
};

typedef struct voice_mainloop_handler_execute {
    int (*execute)(struct userdata *u, void *parameter);
    void *parameter;
} voice_mainloop_handler_execute;

pa_msgobject *voice_mainloop_handler_new(struct userdata *u);

#endif
