AM_LIBADD =						\
	$(DBUS_LIBS)					\
	$(PULSEAUDIO_LIBS) 				\
	$(CMTSPEECH_LIB)				\
	$(BMEIPC_LIBS)					\
	-lm

AM_CFLAGS =						\
	$(PULSEAUDIO_CFLAGS)				\
	$(DBUS_CFLAGS)					\
	$(BMEIPC_CFLAGS)				\
	-I$(top_srcdir)/src/voice			\
	-I$(top_srcdir)/src/eap				\
	-I$(top_srcdir)/src/drc				\
	-I$(top_srcdir)/src/xprot			\
	-I$(top_srcdir)/src/eq				\
	-I$(top_srcdir)/src/common

AM_LIBADD += $(top_builddir)/src/drc/libnokia-drc.la
AM_LIBADD += $(top_builddir)/src/eap/libnokia-eap.la
AM_LIBADD += $(top_builddir)/src/xprot/libnokia-xprot.la
AM_LIBADD += $(top_builddir)/src/eq/libnokia-eq.la
AM_LIBADD += $(top_builddir)/src/common/libnokia-common.la

modlibexec_LTLIBRARIES = module-nokia-voice.la

CMTSPEECH_LIB = -lcmtspeechdata

noinst_HEADERS = module-nokia-voice-symdef.h

module_nokia_voice_la_SOURCES = voice-sidetone.c	\
				voice-util.c		\
				voice-event-forwarder.c	\
				voice-cmtspeech.c	\
				voice-raw-sink.c	\
				voice-raw-source.c	\
				voice-voip-sink.c	\
				voice-voip-source.c	\
				voice-hw-sink-input.c	\
				voice-hw-source-output.c\
				voice-mainloop-handler.c\
				voice-optimized.c	\
				eq_iir.c                \
				eq_fir.c                \
				voice-aep-sink-input.c  \
				voice-cs-call-sink-input.c \
				voice-temperature.c \
				module-nokia-voice.c

module_nokia_voice_la_LDFLAGS = -module -avoid-version -Wl,-no-undefined -Wl,-z,noexecstack
module_nokia_voice_la_LIBADD = $(AM_LIBADD)
module_nokia_voice_la_CFLAGS = $(AM_CFLAGS)
