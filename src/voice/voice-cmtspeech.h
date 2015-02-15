#ifndef VOICECMTSPEECH_H
#define VOICECMTSPEECH_H

#include "dbus/dbus.h"
#include "pulsecore/dbus-shared.h"

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
  void *cmtspeech;
  pa_mutex *unk_mutex;
  pa_semaphore *cmtspeech_semaphore;
  void *cmtspeech_ctx;
  pa_mutex *cmtspeech_mutex;
  pa_rtpoll *rtpoll;
  pa_rtpoll_item *cmt_poll_item;
  pa_rtpoll_item *thread_state_poll_item;
  pa_thread *thread;
  pa_thread_mq thread_mq;
  pa_asyncq *dl_frame_queue;
  pa_bool_t call_ul;
  pa_bool_t call_dl;
  pa_bool_t call_emergency;
  int8_t first_dl_frame_received;
  int8_t record_running;
  int8_t playback_running;
  int8_t unk1_running;
  int8_t unk2_running;
  int8_t unk3_running;
};

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

#endif // VOICECMTSPEECH_H
