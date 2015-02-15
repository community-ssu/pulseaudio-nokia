#include "module-voice-userdata.h"
#include "voice-event-forwarder.h"

static void clear_dbus_matches(struct cmtspeech_dbus_conn *e, DBusConnection *dbusconn)
{
    int i;

    for(i = PA_ELEMENTSOF(e->dbus_match_rules) - 1; i >= 0; i--)
    {
        if (e->dbus_match_rules[i] == NULL)
            continue;

        dbus_bus_remove_match(dbusconn, e->dbus_match_rules[i], NULL);
        pa_log_debug("removed dbus match \"%s\"", e->dbus_match_rules[i]);
        pa_xfree(e->dbus_match_rules[i]);
        e->dbus_match_rules[i] = NULL;
    }
}

void voice_unload_event_forwarder(struct userdata *u)
{
    DBusConnection *dbusconn;
    struct cmtspeech_dbus_conn *e = &u->cmt_dbus_conn;

    if (!e->dbus_conn)
        return;

    dbusconn = pa_dbus_connection_get(e->dbus_conn);

    dbus_connection_remove_filter(dbusconn, voice_cmt_dbus_filter, u);

    clear_dbus_matches(e, dbusconn);

    pa_dbus_connection_unref(e->dbus_conn);
}
