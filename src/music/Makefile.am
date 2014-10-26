AM_LIBADD =
AM_CFLAGS =						\
	$(PULSEAUDIO_CFLAGS)				\
	-I$(top_srcdir)/src/music			\
	-I$(top_srcdir)/src/eap				\
	-I$(top_srcdir)/src/drc				\
	-I$(top_srcdir)/src/common

AM_LIBADD += $(top_builddir)/src/common/libnokia-common.la
AM_LIBADD += $(top_builddir)/src/drc/libnokia-drc.la
AM_LIBADD += $(top_builddir)/src/eap/libnokia-eap.la

modlibexec_LTLIBRARIES = module-nokia-music.la

noinst_HEADERS = module-nokia-music-symdef.h

module_nokia_music_la_SOURCES = module-nokia-music.c

module_nokia_music_la_LDFLAGS = -module -avoid-version -Wl,-no-undefined -Wl,-z,noexecstack
module_nokia_music_la_LIBADD = $(AM_LIBADD) $(PULSEAUDIO_LIBS) -lm $(top_builddir)/src/common/libnokia-common.la
module_nokia_music_la_CFLAGS = $(AM_CFLAGS)
