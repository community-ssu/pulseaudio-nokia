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

#ifndef _CMTSPEECH_MAINLOOP_HANDLER_H_
#define _CMTSPEECH_MAINLOOP_HANDLER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/msgobject.h>
#include "module-nokia-cmtspeech.h"

typedef struct cmtspeech_mainloop_handler {
    pa_msgobject parent;
    struct userdata *u;
} cmtspeech_mainloop_handler;

PA_DECLARE_CLASS(cmtspeech_mainloop_handler);
#define CMTSPEECH_MAINLOOP_HANDLER(o) cmtspeech_mainloop_handler_cast(o)

enum {
    CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_CONNECT,
    CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_DISCONNECT,
    CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_CONNECT,
    CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_DISCONNECT,
    CMTSPEECH_MAINLOOP_HANDLER_MESSAGE_MAX
};

pa_msgobject *cmtspeech_mainloop_handler_new(struct userdata *u);

#endif
