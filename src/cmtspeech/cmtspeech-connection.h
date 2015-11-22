/*
 * This file is part of pulseaudio-nokia
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

#ifndef cmtspeech_connection_h
#define cmtspeech_connection_h

#include "module-nokia-cmtspeech.h"
#include "cmtspeech-mainloop-handler.h"

#define CMTSPEECH_DBUS_CSCALL_CONNECT_IF    "com.nokia.csd.Call.Instance"
#define CMTSPEECH_DBUS_CSCALL_CONNECT_SIG   "AudioConnect"

#define CMTSPEECH_DBUS_CSCALL_STATUS_IF     "com.nokia.csd.Call"
#define CMTSPEECH_DBUS_CSCALL_STATUS_SIG    "ServerStatus"

#define CMTSPEECH_DBUS_PHONE_SSC_STATE_IF   "com.nokia.phone.SSC"
#define CMTSPEECH_DBUS_PHONE_SSC_STATE_SIG  "modem_state_changed_ind"

#include <cmtspeech_msgs.h>

typedef cmtspeech_buffer_t cmtspeech_dl_buf_t;

int cmtspeech_connection_init(struct userdata *u);
void cmtspeech_connection_unload(struct userdata *u);

int cmtspeech_send_ul_frame(struct userdata *u, uint8_t *buf, size_t bytes);

int cmtspeech_buffer_to_memchunk(struct userdata *u, cmtspeech_dl_buf_t *buf, pa_memchunk *chunk);

DBusHandlerResult cmtspeech_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *arg);

#endif /* cmtspeech_connection_h */
