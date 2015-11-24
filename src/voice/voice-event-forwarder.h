#ifndef VOICEEVENTFORWARDER_H
#define VOICEEVENTFORWARDER_H

int voice_init_event_forwarder(struct userdata *u, const char *dbus_type);
void voice_unload_event_forwarder(struct userdata *u);

#endif // VOICEEVENTFORWARDER_H
