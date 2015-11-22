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

/* This should be only used for memblock free cb - cmtspeech_free_cb - below
   and it is initialized in cmtspeech_connection_init(). */
static struct userdata *userdata = NULL;

static uint ul_frame_count = 0;

static void cmt_handler_free(pa_object *o)
{
    pa_log_info("Free called");
    pa_xfree(o);
}

static int priv_cmtspeech_to_pa_prio(int cmtspprio)
{
    if (cmtspprio == 0)
       return PA_LOG_ERROR;

    if (cmtspprio == 3)
       return PA_LOG_INFO;

    return PA_LOG_DEBUG;
}

static void voice_cmt_trace_handler(int priority, const char 
*message, va_list args)
{
    pa_log_levelv_meta(priv_cmtspeech_to_pa_prio(priority),
                      "libcmtspeechdata",
                      0,
                      NULL,
                      message,
                      args);
}

static int add_dbus_match(struct cmtspeech_dbus_conn *e, DBusConnection *dbusconn, char *match)
{
    DBusError error;
    uint i;

    dbus_error_init(&error);

    for(i = 0; i < PA_ELEMENTSOF(e->dbus_match_rules) && e->dbus_match_rules[i] != NULL; i++);

    pa_return_val_if_fail (i < PA_ELEMENTSOF(e->dbus_match_rules), -1);

    dbus_bus_add_match(dbusconn, match, &error);
    if (dbus_error_is_set(&error)) {
        pa_log_error("Unable to add dbus match:\"%s\": %s: %s", match, error.name, error.message);
        return -1;;
    }

    e->dbus_match_rules[i] = pa_xstrdup(match);
    pa_log_debug("added dbus match \"%s\"", match);

    return 0;
}

/* This is called form pulseaudio main thread. */
static DBusHandlerResult voice_cmt_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *arg)
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

int voice_init_event_forwarder(struct userdata *u, const char *dbus_type)
{
    struct cmtspeech_dbus_conn *e = &u->cmt_dbus_conn;
    DBusConnection          *dbusconn;
    DBusError                error;
    char                     rule[512];

    if (0 == strcasecmp(dbus_type, "system"))
  e->dbus_type = DBUS_BUS_SYSTEM;
    else
  e->dbus_type = DBUS_BUS_SESSION;

#define STRBUSTYPE(DBusType) ((DBusType) == DBUS_BUS_SYSTEM ? "system" : "session")

    pa_log_info("DBus connection to %s bus.", STRBUSTYPE(e->dbus_type));

    dbus_error_init(&error);
    e->dbus_conn = pa_dbus_bus_get(u->core, e->dbus_type, &error);

    if (e->dbus_conn == NULL || dbus_error_is_set(&error)) {
        pa_log_error("Failed to get %s Bus: %s: %s", STRBUSTYPE(e->dbus_type), error.name, error.message);
        goto fail;
    }

    memset(&e->dbus_match_rules, 0, sizeof(e->dbus_match_rules));
    dbusconn = pa_dbus_connection_get(e->dbus_conn);

    if (!dbus_connection_add_filter(dbusconn, voice_cmt_dbus_filter, u, NULL)) {
        pa_log("failed to add filter function");
        goto fail;
    }

    snprintf(rule, sizeof(rule), "type='signal',interface='%s'", CMTSPEECH_DBUS_CSCALL_CONNECT_IF);
    if (add_dbus_match(e, dbusconn, rule)) {
  goto fail;
    }

    snprintf(rule, sizeof(rule), "type='signal',interface='%s'", CMTSPEECH_DBUS_CSCALL_STATUS_IF);
    if (add_dbus_match(e, dbusconn, rule)) {
  goto fail;
    }

    snprintf(rule, sizeof(rule), "type='signal',interface='%s'", CMTSPEECH_DBUS_PHONE_SSC_STATE_IF);
    if (add_dbus_match(e, dbusconn, rule)) {
  goto fail;
    }

    return 0;

 fail:
    voice_unload_event_forwarder(u);
    dbus_error_free(&error);
    return -1;
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

