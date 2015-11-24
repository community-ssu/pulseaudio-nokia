#ifndef VOICECMTSPEECH_H
#define VOICECMTSPEECH_H

#include "module-voice-userdata.h"

#include "dbus/dbus.h"
#include "pulsecore/dbus-shared.h"

#define CMTSPEECH_DBUS_CSCALL_CONNECT_IF    "com.nokia.csd.Call.Instance"
#define CMTSPEECH_DBUS_CSCALL_CONNECT_SIG   "AudioConnect"

#define CMTSPEECH_DBUS_CSCALL_STATUS_IF     "com.nokia.csd.Call"
#define CMTSPEECH_DBUS_CSCALL_STATUS_SIG    "ServerStatus"

#define CMTSPEECH_DBUS_PHONE_SSC_STATE_IF   "com.nokia.phone.SSC"
#define CMTSPEECH_DBUS_PHONE_SSC_STATE_SIG  "modem_state_changed_ind"

enum {
    CMTSPEECH_HANDLER_CLOSE_CONNECTION,
};

void voice_cmt_speech_buffer_to_memchunk(struct userdata *u, cmtspeech_buffer_t *buf, pa_memchunk *chunk);
DBusHandlerResult voice_cmt_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *arg);
void voice_cmt_send_ul_frame(struct userdata *u, const void *buffer, size_t size);
int voice_init_cmtspeech(struct userdata *u);
void voice_unload_cmtspeech(struct userdata *u);
#endif // VOICECMTSPEECH_H
