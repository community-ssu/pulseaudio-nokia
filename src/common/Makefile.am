AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = libnokia-common.la

libnokia_common_la_SOURCES = \
	proplist-file.c \
	optimized.c optimized.h \
	memory.c memory.h

#	aupdate.c aupdate.h \
#	parameter-hook.c parameter-hook.h \
#	algorithm-hook.c algorithm-hook.h \
#	proplist-nokia.h

libnokia_common_la_LDFLAGS = -avoid-version
libnokia_common_la_LIBADD = $(PULSEAUDIO_LIBS)

#if X86
#libnokia_common_la_SOURCES += \
#	src-8-to-48.c \
#	src-48-to-8.c
#else
#libnokia_common_la_LIBADD += \
#	src-8-to-48-neon.o \
#	src-48-to-8-neon.o
#%-neon.o: src_8_48_neon.a
#	ar x src_8_48_neon.a
#endif

check_PROGRAMS = tests
tests_SOURCES = tests.c
tests_LDADD = liboptimized.a

libnokia_commondir = /var/lib/@PACKAGE@
libnokia_common_DATA  = data/bta2dp.parameters \
	data/ihfandtv-out.parameters \
	data/btmono-hfp.parameters \
	data/ihf.parameters \
	data/btmono-hsp.parameters \
	data/lineout.parameters \
	data/fmtx.parameters \
	data/ihfandfmtx.parameters \
	data/tvout.parameters \
	data/hp.parameters \
	data/ihfandheadset.parameters \
	data/hs.parameters \
	data/ihfandlineout.parameters

EXTRA_DIST = $(libnokia_common_DATA)

install-data-hook:
	ln -s ihf.parameters $(DESTDIR)$(libnokia_commondir)/ihfandbtmono-hfp.parameters
	ln -s ihf.parameters $(DESTDIR)$(libnokia_commondir)/ihfandbtmono-hsp.parameters
	ln -s ihf.parameters $(DESTDIR)$(libnokia_commondir)/ihfandbtmono.parameters
