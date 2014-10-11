AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = libnokia-drc.la

libnokia_drc_la_SOURCES = \
	drc.c

libnokia_drc_la_LDFLAGS = -avoid-version
libnokia_drc_la_LIBADD = $(PULSEAUDIO_LIBS)
