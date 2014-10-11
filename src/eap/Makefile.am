AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = libnokia-eap.la

libnokia_eap_la_SOURCES = \
	eap_memory.c

libnokia_eap_la_LDFLAGS = -avoid-version
libnokia_eap_la_LIBADD = $(PULSEAUDIO_LIBS)
