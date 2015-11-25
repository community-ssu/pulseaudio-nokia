#include "module-voice-userdata.h"
#include "voice-cmtspeech.h"

#include <cmtspeech.h>

/* TODO: Get rid of this and use asyncmsgq instead. */
enum cmt_speech_thread_state {
    CMT_UNINITIALIZED = 0,
    CMT_STARTING,
    CMT_RUNNING,
    CMT_ASK_QUIT,
    CMT_QUIT
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

PA_DECLARE_CLASS(voice_cmt_handler);
static PA_DEFINE_CHECK_TYPE(voice_cmt_handler, pa_msgobject);

/* This should be only used for memblock free cb - cmtspeech_free_cb - below
   and it is initialized in cmtspeech_connection_init(). */
static struct userdata *userdata = NULL;

static uint ul_frame_count = 0;

static void cmt_handler_free(pa_object *o)
{
    pa_log_info("Free called");
    pa_xfree(o);
}

static int voice_cmtspeech_to_pa_prio(int cmtspprio)
{
    if (cmtspprio == CMTSPEECH_TRACE_ERROR)
       return PA_LOG_ERROR;

    if (cmtspprio == CMTSPEECH_TRACE_INFO)
       return PA_LOG_INFO;

    return PA_LOG_DEBUG;
}

struct cmtspeech_buffer_t;

void voice_cmt_speech_buffer_to_memchunk(struct userdata *u, cmtspeech_buffer_t *buf, pa_memchunk *chunk)
{
    //todo address 0x0000FBD4
}

static void cmtspeech_free_cb(void *p)
{
    //todo address 0x0000FC40
}

static void close_cmtspeech_on_error(struct userdata *u)
{
    //todo address 0x000102FC
}

void voice_cmt_send_ul_frame(struct userdata *u, const void *buffer, size_t size)
{
    //todo address 0x00010614
}

static int cmt_handler_process_msg(pa_msgobject *o, int code, void *data,
                                   int64_t offset, pa_memchunk *chunk)
{
    voice_cmt_handler *h = (voice_cmt_handler *)o;
    struct userdata *u;

    pa_assert(u = h->u);

    if (code)
    {
        pa_log_error("Unknown message code %d", code);

        return -1;
    }

    pa_log_debug("VOICE_CMT_HANDLER_CLOSE_CONNECTION");
    close_cmtspeech_on_error(u);

    return 0;
}

static void thread_func(void *udata)
{
    //todo address 0x000108F0
}

static void voice_cmtspeech_trace_handler(int priority, const char *message,
                                          va_list args)
{
    pa_log_levelv_meta(voice_cmtspeech_to_pa_prio(priority),
                      "libcmtspeechdata",
                      0,
                      NULL,
                      message,
                      args);
}

/* This is called form pulseaudio main thread. */
DBusHandlerResult voice_cmt_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *arg)
{
    DBusMessageIter args;
    int type;
    struct userdata *u = arg;
    struct cmtspeech_connection *c = &u->cmt_connection;

    pa_assert(u);

    DBusError dbus_error;
    dbus_error_init(&dbus_error);

    if (dbus_message_is_signal(msg, CMTSPEECH_DBUS_CSCALL_CONNECT_IF, CMTSPEECH_DBUS_CSCALL_CONNECT_SIG)) {
        dbus_bool_t ulflag, dlflag, emergencyflag;

        dbus_message_get_args(msg, &dbus_error,
                              DBUS_TYPE_BOOLEAN, &ulflag,
                              DBUS_TYPE_BOOLEAN, &dlflag,
                              DBUS_TYPE_BOOLEAN, &emergencyflag,
                              DBUS_TYPE_INVALID);

        if (dbus_error_is_set(&dbus_error) != TRUE) {
            pa_log_debug("received AudioConnect with params %d, %d, %d", ulflag, dlflag, emergencyflag);

            c->call_ul = (ulflag == TRUE ? TRUE : FALSE);
            c->call_dl = (dlflag == TRUE ? TRUE : FALSE);
            c->call_emergency = (emergencyflag == TRUE ? TRUE : FALSE);

            /* note: very rarely taken code path */
            pa_mutex_lock(c->cmtspeech_mutex);
            if (c->cmtspeech)
                cmtspeech_state_change_call_connect(c->cmtspeech, dlflag == TRUE);
            pa_mutex_unlock(c->cmtspeech_mutex);

        } else
            pa_log_error("received %s with invalid parameters", CMTSPEECH_DBUS_CSCALL_CONNECT_SIG);

        return DBUS_HANDLER_RESULT_HANDLED;

    } else if (dbus_message_is_signal(msg, CMTSPEECH_DBUS_CSCALL_STATUS_IF, CMTSPEECH_DBUS_CSCALL_STATUS_SIG)) {
        pa_log_debug("Received ServerStatus");

        if (dbus_message_iter_init(msg, &args) == TRUE) {
            type = dbus_message_iter_get_arg_type(&args);
            dbus_bool_t val;
            if (type == DBUS_TYPE_BOOLEAN) {
                dbus_message_iter_get_basic(&args, &val);
            }

            pa_log_debug("Set ServerStatus to %d.", val == TRUE);

            /* note: very rarely taken code path */
            pa_mutex_lock(c->cmtspeech_mutex);
            if (c->cmtspeech)
                cmtspeech_state_change_call_status(c->cmtspeech, val == TRUE);
            pa_mutex_unlock(c->cmtspeech_mutex);

        } else
            pa_log_error("received %s with invalid parameters", CMTSPEECH_DBUS_CSCALL_STATUS_SIG);

        return DBUS_HANDLER_RESULT_HANDLED;

    } else if (dbus_message_is_signal(msg, CMTSPEECH_DBUS_PHONE_SSC_STATE_IF, CMTSPEECH_DBUS_PHONE_SSC_STATE_SIG)) {
        const char* modemstate = NULL;

        dbus_message_get_args(msg, &dbus_error,
                              DBUS_TYPE_STRING, &modemstate,
                              DBUS_TYPE_INVALID);

        if (dbus_error_is_set(&dbus_error) != TRUE) {
            pa_log_debug("modem state change: %s", modemstate);

            if (strcmp(modemstate, "initialize") == 0) {
                int lstate;

                /* If modem state changes to "initialize" (it has just
                 * booted), during a call, this means a fatal error has
                 * occured and we should close the cmtspeech instance
                 * immediately.
                 *
                 * The library should ideally notify us of this situation,
                 * the current implementation (at least libcmtspeech 1.5.2
                 * and older) cannot detect all reset conditions reliably,
                 * so this additional check is needed.
                 */
                if (c->cmtspeech) {
                    /* locking note: very rarely taken code path */
                    pa_mutex_lock(c->cmtspeech_mutex);
                    lstate = cmtspeech_protocol_state(c->cmtspeech);
                    pa_mutex_unlock(c->cmtspeech_mutex);

                    if (lstate != CMTSPEECH_STATE_DISCONNECTED) {
                        pa_log_error("modem reset during call, closing library instance.");

                        /* Syncronous pa_asyncmsgq_send() can not be used here, because handling of
                           CMTSPEECH_HANDLER_CLOSE_CONNECTION is using the message queue too. */
                        pa_asyncmsgq_post(c->thread_mq.inq, c->cmt_handler,
                                          CMTSPEECH_HANDLER_CLOSE_CONNECTION, NULL, 0, NULL, NULL);
                    }
                }
            }
        }
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Main thread */
void voice_unload_cmtspeech(struct userdata *u)
{
    struct cmtspeech_connection *c = &u->cmt_connection;

    pa_assert(u);

    switch (pa_atomic_load(&c->thread_state)) {
        default:
            pa_log_error("Undefined thread_state value: %d", pa_atomic_load(&c->thread_state));
            // fall trough
        case CMT_UNINITIALIZED:
            pa_log_debug("No CMT connection to unload");
            return;
        case CMT_STARTING:
            while (pa_atomic_load(&c->thread_state) == CMT_STARTING) {
                pa_log_debug("CMT connection not up yet, waiting...");
                usleep(200000);
            }
            // fall trough
        case CMT_RUNNING:
            pa_assert_se(pa_atomic_cmpxchg(&c->thread_state, CMT_RUNNING, CMT_ASK_QUIT));
            pa_fdsem_post(c->thread_state_change);
            // fall trough
        case CMT_ASK_QUIT:
            while (pa_atomic_load(&c->thread_state) == CMT_ASK_QUIT) {
                pa_log_debug("Waiting for CMT connection thread to quit...");
                usleep(200000);
            }
            pa_log_debug("cmtspeech thread has ended");
            // fall trough
        case CMT_QUIT:
            break;
    }

    pa_atomic_store(&c->thread_state, CMT_UNINITIALIZED);
    if (c->cmt_handler) {
        c->cmt_handler->parent.free((pa_object *)c->cmt_handler);
        c->cmt_handler = NULL;
    }
    if (c->thread_state_change) {
        pa_fdsem_free(c->thread_state_change);
        c->thread_state_change = NULL;
    }
    pa_atomic_store(&c->ul_state,0);
    pa_atomic_store(&c->dl_state,0);
    if (c->cmtspeech_semaphore)
    {
        pa_semaphore_free(c->cmtspeech_semaphore);
        c->cmtspeech_semaphore = 0;
    }
    pa_rtpoll_free(c->rtpoll);
    c->rtpoll = NULL;
    pa_thread_mq_done(&c->thread_mq);

    if (c->cmtspeech) {
        pa_log_error("CMT speech connection up when shutting down");
    }
    pa_asyncq_free(c->dl_frame_queue, NULL);
    pa_mutex_free(c->ul_timing_mutex);
    pa_mutex_free(c->cmtspeech_mutex);
    pa_log_debug("CMT connection unloaded");
}

static voice_cmt_handler *voice_cmt_handler_new(pa_mainloop_api *m, int shared)
{
    voice_cmt_handler *h;

    pa_assert(h = pa_msgobject_new(voice_cmt_handler));
    h->parent.parent.free = cmt_handler_free;
    h->parent.process_msg = cmt_handler_process_msg;

    return h;
}

int voice_init_cmtspeech(struct userdata *u)
{
    voice_cmt_handler *h;
    struct cmtspeech_connection *c = &u->cmt_connection;

    userdata = u;

    h = voice_cmt_handler_new(u->core->mainloop, FALSE);
    h->u = u;

    c->cmt_handler = (pa_msgobject *)h;
    pa_atomic_store(&c->thread_state, 1);
    pa_atomic_store(&c->ul_state, CMT_DL_INACTIVE);
    pa_atomic_store(&c->dl_state, CMT_UL_INACTIVE);
    c->thread_state_change = pa_fdsem_new();
    c->cmtspeech = NULL;
    c->cmtspeech_semaphore = pa_semaphore_new(0);
    c->cmtspeech_mutex = pa_mutex_new(FALSE, FALSE);
    c->cmt_poll_item = NULL;
    c->rtpoll = pa_rtpoll_new();
    pa_thread_mq_init(&c->thread_mq, u->core->mainloop, c->rtpoll);
    c->deadline.tv_usec = -1;
    c->deadline.tv_sec = 0;
    c->ul_timing_mutex = pa_mutex_new(FALSE, FALSE);
    c->dl_frame_queue = pa_asyncq_new(4);

    cmtspeech_init();
    cmtspeech_trace_toggle(CMTSPEECH_TRACE_ERROR, TRUE);
    cmtspeech_trace_toggle(CMTSPEECH_TRACE_INFO, TRUE);
    cmtspeech_trace_toggle(CMTSPEECH_TRACE_STATE_CHANGE, TRUE);
    cmtspeech_trace_toggle(CMTSPEECH_TRACE_IO, TRUE);
    cmtspeech_trace_toggle(CMTSPEECH_TRACE_DEBUG, TRUE);
    cmtspeech_set_trace_handler(voice_cmtspeech_trace_handler);

    c->call_ul = FALSE;
    c->call_dl = FALSE;
    c->call_emergency = FALSE;
    c->first_dl_frame_received = FALSE;
    c->record_running = FALSE;
    c->playback_running = FALSE;

    c->thread = pa_thread_new(thread_func, u);

    if (!c->thread)
    {
      pa_log_error("Failed to create thread.");
      pa_atomic_store(&c->thread_state, 4);
      voice_unload_cmtspeech(u);

      return -1;
    }

    return 0;
}
