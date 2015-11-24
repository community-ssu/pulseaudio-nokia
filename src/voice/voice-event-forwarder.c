#include "module-voice-userdata.h"
#include "voice-event-forwarder.h"
#include "voice-cmtspeech.h"

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
