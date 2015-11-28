AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = libnokia-xprot.la

libnokia_xprot_la_SOURCES = \
	xprot.c a_xprot.c

libnokia_xprot_la_LDFLAGS = -avoid-version
libnokia_xprot_la_LIBADD = $(PULSEAUDIO_LIBS)
