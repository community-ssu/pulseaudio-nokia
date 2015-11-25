#include "module-voice-userdata.h"
#include "voice-cmtspeech.h"

#include <cmtspeech.h>
#include <poll.h>

#define ONDEBUG_TOKENS(a)

typedef cmtspeech_buffer_t cmtspeech_dl_buf_t;

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

/* FIXME - some members seems to be missing here */
enum {
    CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_CONNECT,
    CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_CONNECT,
    CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_DISCONNECT = 3,
    CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_DISCONNECT = 4,
    CMTSPEECH_MAINLOOP_HANDLER_MESSAGE_MAX
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

static void voice_cmt_dl_deactivate(struct userdata *u)
{
    struct cmtspeech_connection *c = &u->cmt_connection;

    pa_log_debug("voice_cmt_dl_deactivate");

    do
    {
      for (;;)
      {
          int dl_state = pa_atomic_load(&c->dl_state);

          if (dl_state == CMT_DL_ACTIVE)
              break;

          if (dl_state == CMT_DL_DEACTIVATE || dl_state == CMT_DL_INACTIVE)
              goto out;
      }
    }
    while(!pa_atomic_cmpxchg(&c->dl_state, CMT_DL_ACTIVE, CMT_DL_DEACTIVATE));

    pa_log_debug("DL state changed from CMT_DL_ACTIVE to CMT_DL_DEACTIVATE");
    pa_semaphore_wait(c->cmtspeech_semaphore);
    pa_asyncmsgq_post(c->thread_mq.outq, u->mainloop_handler,
                      CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_DISCONNECT, 0, 0, 0, 0);

out:
    c->playback_running = FALSE;
}

static void voice_cmt_ul_deactivate(struct userdata *u)
{
    struct cmtspeech_connection *c = &u->cmt_connection;

    pa_log_debug("voice_cmt_ul_deactivate");

    do
    {
      for (;;)
      {
          int ul_state = pa_atomic_load(&c->ul_state);

          if (ul_state == CMT_UL_ACTIVE)
              break;

          if (ul_state == CMT_UL_DEACTIVATE || ul_state == CMT_UL_INACTIVE)
              goto out;
      }
    }
    while(!pa_atomic_cmpxchg(&c->ul_state, CMT_UL_ACTIVE, CMT_UL_DEACTIVATE));

    pa_log_debug("DL state changed from CMT_UL_ACTIVE to CMT_UL_DEACTIVATE");
    pa_semaphore_wait(c->cmtspeech_semaphore);
    pa_asyncmsgq_post(c->thread_mq.outq, u->mainloop_handler,
                      CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_DISCONNECT, 0, 0, 0, 0);

out:
    c->record_running = FALSE;
}

static void reset_call_stream_states(struct userdata *u)
{
    if (u->cmt_connection.playback_running)
        voice_cmt_dl_deactivate(u);

    if ( u->cmt_connection.record_running )
        voice_cmt_ul_deactivate(u);
}

static void close_cmtspeech_on_error(struct userdata *u)
{
    void *buf;

    pa_log_error("closing the modem instance");

    reset_call_stream_states(u);
    pa_mutex_lock(u->cmt_connection.cmtspeech_mutex);

    while ((buf = pa_asyncq_pop(u->cmt_connection.dl_frame_queue, 0)))
    {
        if (cmtspeech_dl_buffer_release(u->cmt_connection.cmtspeech, buf))
            pa_log_error("Freeing cmtspeech buffer failed!");
    }

    if (cmtspeech_close(u->cmt_connection.cmtspeech))
        pa_log_error("cmtspeech_close() failed");

    u->cmt_connection.cmtspeech = NULL;
    pa_mutex_unlock(u->cmt_connection.cmtspeech_mutex);
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

/* cmtspeech thread */
static int check_cmtspeech_connection(struct cmtspeech_connection *c) {
    static uint counter = 0;

    if (c->cmtspeech)
        return 0;

    /* locking note: not on the hot path */

    pa_mutex_lock(c->cmtspeech_mutex);

    c->cmtspeech = cmtspeech_open();

    pa_mutex_unlock(c->cmtspeech_mutex);

    if (!c->cmtspeech) {
        if (counter++ < 5)
            pa_log_error("cmtspeech_open() failed");
        return -1;
    } else if (counter > 0) {
        pa_log_error("cmtspeech_open() OK");
        counter = 0;
    }
    return 0;
}

/* cmtspeech thread */
static void update_uplink_frame_timing(struct userdata *u,
                                       cmtspeech_event_t *cmtevent)
{
    suseconds_t usec;
    time_t sec;
    int deadline_us;

    pa_log_debug("msec= %d usec=%d rtclock=%d.%09ld",
                 (int)cmtevent->msg.timing_config_ntf.msec,
                 (int)cmtevent->msg.timing_config_ntf.usec,
                 (int)cmtevent->msg.timing_config_ntf.tstamp.tv_sec,
                 cmtevent->msg.timing_config_ntf.tstamp.tv_nsec);

    deadline_us = 1000 * (cmtevent->msg.timing_config_ntf.msec % 20) +
            cmtevent->msg.timing_config_ntf.usec;
    usec = deadline_us + cmtevent->msg.timing_config_ntf.tstamp.tv_nsec / 1000;
    sec = usec / 1000000 + cmtevent->msg.timing_config_ntf.tstamp.tv_sec;
    usec %= 1000000;

    pa_log_debug("deadline %d.%06d (usec from now %d)", (int)sec,
                 (int)(usec), deadline_us);

    pa_mutex_lock(u->cmt_connection.ul_timing_mutex);
    u->cmt_connection.deadline.tv_usec = usec;
    u->cmt_connection.deadline.tv_sec = sec;
    pa_mutex_unlock(u->cmt_connection.ul_timing_mutex);
}

/* cmtspeech thread */
static inline
int push_cmtspeech_buffer_to_dl_queue(struct userdata *u, cmtspeech_dl_buf_t *buf) {
    pa_assert_fp(u);
    pa_assert_fp(buf);

    if (pa_asyncq_push(u->cmt_connection.dl_frame_queue, (void *)buf, FALSE)) {
        int ret;
        struct cmtspeech_connection *c = &u->cmt_connection;

        pa_log_error("Failed to push dl frame to asyncq");
        pa_mutex_lock(c->cmtspeech_mutex);
        if ((ret = cmtspeech_dl_buffer_release(u->cmt_connection.cmtspeech, buf)))
            pa_log_error("cmtspeech_dl_buffer_release(%p) failed return value %d.", (void *)buf, ret);
        pa_mutex_unlock(c->cmtspeech_mutex);
        return -1;
    }

    ONDEBUG_TOKENS(fprintf(stderr, "D"));
    return 0;
}

/* cmtspeech thread */
static int mainloop_cmtspeech(struct userdata *u) {
    int retsockets = 0;
    struct cmtspeech_connection *c = &u->cmt_connection;
    struct pollfd *pollfd;

    pa_assert(u);

    if (!c->cmt_poll_item)
        return 0;

    pollfd = pa_rtpoll_item_get_pollfd(c->cmt_poll_item, NULL);
    if (pollfd->revents & POLLIN) {
        cmtspeech_t *cmtspeech;
        int flags = 0, i = CMTSPEECH_CTRL_LEN;
        int res;

        /* locking note: hot path lock */
        pa_mutex_lock(c->cmtspeech_mutex);

        cmtspeech = c->cmtspeech;

        res = cmtspeech_check_pending(cmtspeech, &flags);
        if (res >= 0)
            retsockets = 1;

        pa_mutex_unlock(c->cmtspeech_mutex);

        if (res > 0) {
            if (flags & CMTSPEECH_EVENT_CONTROL) {
                cmtspeech_event_t cmtevent;

                /* locking note: this path is taken only very rarely */
                pa_mutex_lock(c->cmtspeech_mutex);

                i = cmtspeech_read_event(cmtspeech, &cmtevent);

                pa_mutex_unlock(c->cmtspeech_mutex);

                pa_log_debug("read cmtspeech event: state %d -> %d (type %d, ret %d).",
                             cmtevent.prev_state, cmtevent.state, cmtevent.msg_type, i);

                if (i != 0) {
                    pa_log_error("ERROR: unable to read event.");

                } else if (cmtevent.prev_state == CMTSPEECH_STATE_DISCONNECTED &&
                           cmtevent.state == CMTSPEECH_STATE_CONNECTED) {
                    pa_log_debug("call starting.");
                    reset_call_stream_states(u);

                } else if (cmtevent.prev_state == CMTSPEECH_STATE_CONNECTED &&
                           cmtevent.state == CMTSPEECH_STATE_ACTIVE_DL &&
                           cmtevent.msg_type == CMTSPEECH_SPEECH_CONFIG_REQ) {
                    pa_log_notice("speech start: srate=%u, format=%u, stream=%u",
                                  cmtevent.msg.speech_config_req.sample_rate,
                                  cmtevent.msg.speech_config_req.data_format,
                                  cmtevent.msg.speech_config_req.speech_data_stream);

                     /* Ul is turned on when timing information is received */

                    pa_log_debug("enabling DL");
                    pa_asyncmsgq_post(pa_thread_mq_get()->outq, u->mainloop_handler,
                                      CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_CONNECT, NULL, 0, NULL, NULL);
                    c->playback_running = true;

                     // start waiting for first dl frame
                     c->first_dl_frame_received = false;
                } else if (cmtevent.prev_state == CMTSPEECH_STATE_ACTIVE_DLUL &&
                           cmtevent.state == CMTSPEECH_STATE_ACTIVE_DL &&
                           cmtevent.msg_type == CMTSPEECH_SPEECH_CONFIG_REQ) {

                    pa_log_notice("speech update: srate=%u, format=%u, stream=%u",
                                  cmtevent.msg.speech_config_req.sample_rate,
                                  cmtevent.msg.speech_config_req.data_format,
                                  cmtevent.msg.speech_config_req.speech_data_stream);

                } else if (cmtevent.prev_state == CMTSPEECH_STATE_ACTIVE_DL &&
                           cmtevent.state == CMTSPEECH_STATE_ACTIVE_DLUL) {
                    pa_log_debug("enabling UL");

                    pa_asyncmsgq_post(pa_thread_mq_get()->outq, u->mainloop_handler,
                                    CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_CONNECT, NULL, 0, NULL, NULL);
                    c->record_running = true;

                } else if (cmtevent.state == CMTSPEECH_STATE_ACTIVE_DLUL &&
                           cmtevent.msg_type == CMTSPEECH_TIMING_CONFIG_NTF) {
                    update_uplink_frame_timing(u, &cmtevent);
                    pa_log_debug("updated UL timing params");

                } else if ((cmtevent.prev_state == CMTSPEECH_STATE_ACTIVE_DL ||
                            cmtevent.prev_state == CMTSPEECH_STATE_ACTIVE_DLUL) &&
                           cmtevent.state == CMTSPEECH_STATE_CONNECTED) {
                    pa_log_notice("speech stop: stream=%u",
                                  cmtevent.msg.speech_config_req.speech_data_stream);
                    pa_asyncmsgq_post(pa_thread_mq_get()->outq, u->mainloop_handler,
                                      CMTSPEECH_MAINLOOP_HANDLER_CMT_DL_DISCONNECT, NULL, 0, NULL, NULL);
                    c->playback_running = FALSE;
                    pa_asyncmsgq_post(pa_thread_mq_get()->outq, u->mainloop_handler,
                                      CMTSPEECH_MAINLOOP_HANDLER_CMT_UL_DISCONNECT, NULL, 0, NULL, NULL);
                    c->record_running = FALSE;
                    ul_frame_count = 0;

                } else if (cmtevent.prev_state == CMTSPEECH_STATE_CONNECTED &&
                         cmtevent.state == CMTSPEECH_STATE_DISCONNECTED) {
                    pa_log_debug("call terminated.");
                    reset_call_stream_states(u);

                } else {
                    pa_log_error("Unrecognized cmtspeech event: state %d -> %d (type %d, ret %d).",
                                 cmtevent.prev_state, cmtevent.state, cmtevent.msg_type, i);
                    if (cmtevent.state == CMTSPEECH_STATE_DISCONNECTED)
                        reset_call_stream_states(u);
                }
            }

            /* step: check for SSI data events */
            if (flags & CMTSPEECH_EVENT_DL_DATA) {
                cmtspeech_buffer_t *buf;
                static int counter = 0;
                bool cmtspeech_active = false;

                counter++;
                if (counter < 10)
                    pa_log_debug("SSI: DL frame available, read %d bytes.", i);

                /* locking note: another hot path lock */
                pa_mutex_lock(c->cmtspeech_mutex);
                cmtspeech_active = cmtspeech_is_active(c->cmtspeech);
                i = cmtspeech_dl_buffer_acquire(cmtspeech, &buf);
                pa_mutex_unlock(c->cmtspeech_mutex);

                if (i < 0) {
                    pa_log_error("Invalid DL frame received, cmtspeech_dl_buffer_acquire returned %d", i);
                } else {
                    if (counter < 10 )
                        pa_log_debug("DL (audio len %d) frame's first bytes %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                                     buf->count - CMTSPEECH_DATA_HEADER_LEN,
                                     buf->data[0], buf->data[1], buf->data[1], buf->data[3],
                                     buf->data[4], buf->data[5], buf->data[6], buf->data[7]);

                    if (c->playback_running) {
                        if (c->first_dl_frame_received != true) {
                            c->first_dl_frame_received = true;
                            pa_log_debug("DL frame received, turn DL routing on...");
                        }
                        (void)push_cmtspeech_buffer_to_dl_queue(u, buf);

                    } else if (cmtspeech_active != true) {
                        pa_log_debug("DL frame received before ACTIVE_DL state, dropping...");
                    }
                }
            }
        }
    }

    return retsockets;
}

/* cmtspeech thread */
static void pollfd_update(struct cmtspeech_connection *c) {
    if (c->cmt_poll_item) {
        pa_rtpoll_item_free(c->cmt_poll_item);
        c->cmt_poll_item = NULL;
    }
    if (c->cmtspeech) {
        pa_rtpoll_item *i = pa_rtpoll_item_new(c->rtpoll, PA_RTPOLL_NEVER, 1);
        struct pollfd *pollfd = pa_rtpoll_item_get_pollfd(i, NULL);
        /* locking note: a hot path lock */
        pa_mutex_lock(c->cmtspeech_mutex);
        pollfd->fd = cmtspeech_descriptor(c->cmtspeech);
        pa_mutex_unlock(c->cmtspeech_mutex);
        pollfd->events = POLLIN;
        pollfd->revents = 0;

        c->cmt_poll_item = i;

    } else {
        pa_log_debug("No cmtspeech connection");
    }

    if (c->thread_state_poll_item) {
        pa_rtpoll_item_free(c->thread_state_poll_item);
        c->thread_state_poll_item = NULL;
    }

    c->thread_state_poll_item = pa_rtpoll_item_new_fdsem(c->rtpoll, PA_RTPOLL_NORMAL, c->thread_state_change);
}

/* cmtspeech thread */
static void thread_func(void *udata)
{
    struct userdata *u = udata;
    struct cmtspeech_connection *c = &u->cmt_connection;

    pa_assert(u);

    pa_log_debug("cmtspeech thread starting up");

    if (u->core->realtime_scheduling)
        pa_make_realtime(u->core->realtime_priority - 1);

    pa_thread_mq_install(&c->thread_mq);

    c->cmtspeech = cmtspeech_open();

    pa_assert_se(pa_atomic_cmpxchg(&c->thread_state, CMT_STARTING,
                                   CMT_RUNNING));

    while(1)
    {
        int ret;

        if (check_cmtspeech_connection(c))
            pa_rtpoll_set_timer_relative(c->rtpoll, 500000);

        pollfd_update(c);

        if (0 > (ret = pa_rtpoll_run(c->rtpoll, TRUE)))
        {
            pa_log_error("running rtpoll failed (%d) (fd %d)", ret,
                         cmtspeech_descriptor(c->cmtspeech));
            close_cmtspeech_on_error(u);
        }

        if (pa_atomic_load(&c->thread_state) == CMT_ASK_QUIT)
        {
            pa_log_debug("cmtspeech thread quiting");
            goto finish;
        }

        /* note: cmtspeech can be closed in DBus thread */
        if (c->cmtspeech == NULL)
        {
            pa_log_notice("closing and reopening cmtspeech device");
            continue;
        }

        if (0 > mainloop_cmtspeech(u))
            break;
    }

finish:
    close_cmtspeech_on_error(u);

    pa_assert_se(pa_atomic_cmpxchg(&c->thread_state, CMT_ASK_QUIT, CMT_QUIT));

    pa_log_debug("cmtspeech thread ended");
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
