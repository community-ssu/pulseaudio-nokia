#ifndef VOICECMTSPEECH_H
#define VOICECMTSPEECH_H

#include "module-voice-userdata.h"

#include "dbus/dbus.h"
#include "pulsecore/dbus-shared.h"
#include "voice-aep-ear-ref.h"

#define CMTSPEECH_DBUS_CSCALL_CONNECT_IF    "com.nokia.csd.Call.Instance"
#define CMTSPEECH_DBUS_CSCALL_CONNECT_SIG   "AudioConnect"

#define CMTSPEECH_DBUS_CSCALL_STATUS_IF     "com.nokia.csd.Call"
#define CMTSPEECH_DBUS_CSCALL_STATUS_SIG    "ServerStatus"

#define CMTSPEECH_DBUS_PHONE_SSC_STATE_IF   "com.nokia.phone.SSC"
#define CMTSPEECH_DBUS_PHONE_SSC_STATE_SIG  "modem_state_changed_ind"

enum {
    CMTSPEECH_HANDLER_CLOSE_CONNECTION,
};

enum CMT_DL_STATE
{
    CMT_DL_INACTIVE,
    CMT_DL_ACTIVE,
    CMT_DL_DEACTIVATE
};

enum CMT_UL_STATE
{
    CMT_UL_INACTIVE,
    CMT_UL_ACTIVE,
    CMT_UL_DEACTIVATE
};

struct _voice_cmt_handler
{
    pa_msgobject parent;
    struct userdata *u;
};

typedef struct _voice_cmt_handler voice_cmt_handler;

void voice_cmt_speech_buffer_to_memchunk(struct userdata *u, cmtspeech_buffer_t *buf, pa_memchunk *chunk);
DBusHandlerResult voice_cmt_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *arg);
int voice_cmt_send_ul_frame(struct userdata *u, uint8_t *buf, size_t bytes);
int voice_init_cmtspeech(struct userdata *u);
void voice_unload_cmtspeech(struct userdata *u);

static inline pa_bool_t
voice_cmt_ul_is_active_iothread(struct userdata *u)
{
    struct cmtspeech_connection *c = &u->cmt_connection;

    while (1)
    {
        int ul_state = pa_atomic_load(&c->ul_state);

        if (ul_state == CMT_UL_ACTIVE)
            return TRUE;

        if (ul_state == CMT_UL_INACTIVE)
            break;

        if (pa_atomic_cmpxchg(&c->ul_state, CMT_UL_DEACTIVATE, CMT_UL_INACTIVE))
        {
            pa_log_debug("UL state changed from CMT_UL_DEACTIVATE to CMT_UL_INACTIVE");
            pa_memblockq_flush_read(u->ul_memblockq);
            VOICE_TIMEVAL_INVALIDATE(&c->deadline);
            voice_aep_ear_ref_loop_reset(u);
            pa_semaphore_post(c->cmtspeech_semaphore);
            break;
        }
    }

    return FALSE;
}

static inline pa_bool_t
voice_cmt_dl_is_active_iothread(struct userdata *u)
{
    struct cmtspeech_connection *c = &u->cmt_connection;

    while (1)
    {
        int dl_state = pa_atomic_load(&c->dl_state);

        if (dl_state == CMT_DL_ACTIVE)
            return TRUE;

        if (dl_state == CMT_DL_INACTIVE)
            break;

        if (pa_atomic_cmpxchg(&c->dl_state, CMT_DL_DEACTIVATE, CMT_DL_INACTIVE))
        {
          void *buf;
          int err;

          pa_log_debug("DL state changed from CMT_DL_DEACTIVATE to CMT_DL_INACTIVE");
          pa_memblockq_flush_read((pa_memblockq *)u->dl_memblockq);

          while ((buf = pa_asyncq_pop(c->dl_frame_queue, 0)))
          {
            pa_mutex_lock(c->cmtspeech_mutex);
            if ((err = cmtspeech_dl_buffer_release(c->cmtspeech, buf)))
              pa_log_error("cmtspeech_dl_buffer_release(%p) failed return value %d.", buf, err);
            pa_mutex_unlock(c->cmtspeech_mutex);
          }

          voice_aep_ear_ref_loop_reset(u);
          pa_semaphore_post(c->cmtspeech_semaphore);
          break;
        }
    }

    return FALSE;
}

#endif // VOICECMTSPEECH_H
