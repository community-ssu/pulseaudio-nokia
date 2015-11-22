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

DBusHandlerResult voice_cmt_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *arg);

int voice_init_event_forwarder(struct userdata *u, const char *dbus_type);
void voice_unload_cmtspeech(struct userdata *u);

#endif // VOICECMTSPEECH_H
