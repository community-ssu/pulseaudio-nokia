AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = module-nokia-algorithm-drc.la

module_nokia_algorithm_drc_la_SOURCES = \
	drc-module.c \
	module-nokia-algorithm-drc.h

#	aupdate.c aupdate.h \
#	parameter-hook.c parameter-hook.h \
#	algorithm-hook.c algorithm-hook.h \
#	proplist-nokia.h

module_nokia_algorithm_drc_la_LDFLAGS = -avoid-version
module_nokia_algorithm_drc_la_LIBADD = $(PULSEAUDIO_LIBS)

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
