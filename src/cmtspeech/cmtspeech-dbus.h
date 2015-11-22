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

#ifndef cmtspeech_dbus_h
#define cmtspeech_dbus_h

#include "module-nokia-cmtspeech.h"

int cmtspeech_dbus_init(struct userdata *u, const char *dbus_type);
void cmtspeech_dbus_unload(struct userdata *u);

#endif // cmtspeech_dbus_h
