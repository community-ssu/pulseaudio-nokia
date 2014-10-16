AM_CFLAGS = \
	$(PULSEAUDIO_CFLAGS)

modlibexec_LTLIBRARIES = libnokia-eap.la

libnokia_eap_la_SOURCES = \
	eap_att_rel_filter_int32.c \
	eap_memory.c \
	eap_wfir_dummy_int32.c \
	eap_wfir_two_bands_int32.c \
	eap_wfir_three_bands_int32.c \
	eap_wfir_four_bands_int32.c \
	eap_wfir_five_bands_int32.c \
	eap_qmf_stereo_int32.c \
	eap_multiband_drc_control_int32.c \
	eap_mdrc_delays_and_gains_int32.c \
	eap_limiter_int32.c \
	eap_average_amplitude_int32.c \
	eap_multiband_drc_int32.c

libnokia_eap_la_LDFLAGS = -avoid-version
libnokia_eap_la_LIBADD = $(PULSEAUDIO_LIBS)
