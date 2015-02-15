AM_LIBADD =						\
	$(DBUS_LIBS)					\
	$(PULSEAUDIO_LIBS) 				\
	$(CMTSPEECH_LIB)				\
	-lm

AM_CFLAGS =						\
	$(PULSEAUDIO_CFLAGS)				\
	$(DBUS_CFLAGS)					\
	-I$(top_srcdir)/src/voice			\
	-I$(top_srcdir)/src/eap				\
	-I$(top_srcdir)/src/drc				\
	-I$(top_srcdir)/src/cmtspeech			\
	-I$(top_srcdir)/src/xprot			\
	-I$(top_srcdir)/src/common

AM_LIBADD += $(top_builddir)/src/drc/libnokia-drc.la
AM_LIBADD += $(top_builddir)/src/eap/libnokia-eap.la
AM_LIBADD += $(top_builddir)/src/xprot/libnokia-xprot.la
AM_LIBADD += $(top_builddir)/src/common/libnokia-common.la

modlibexec_LTLIBRARIES = module-nokia-voice.la

CMTSPEECH_LIB = -lcmtspeechdata

noinst_HEADERS = module-nokia-voice-symdef.h

module_nokia_voice_la_SOURCES = module-nokia-voice.c	\
				voice-sidetone.c	\
				voice-util.c		\
				voice-event-forwarder.c	\
				voice-cmtspeech.c

module_nokia_voice_la_LDFLAGS = -module -avoid-version -Wl,-no-undefined -Wl,-z,noexecstack
module_nokia_voice_la_LIBADD = $(AM_LIBADD)
module_nokia_voice_la_CFLAGS = $(AM_CFLAGS)
