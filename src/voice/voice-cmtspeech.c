#include "module-voice-userdata.h"
#include "voice-cmtspeech.h"

#include <cmtspeech.h>

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
