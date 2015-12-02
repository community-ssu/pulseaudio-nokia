AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = libnokia-eq.la

libnokia_eq_la_SOURCES = \
	iir.c

libnokia_eq_la_LDFLAGS = -avoid-version
libnokia_eq_la_LIBADD = $(PULSEAUDIO_LIBS)
