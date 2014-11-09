#-------------------------------------------------
#
# Project created by QtCreator 2014-10-18T14:30:03
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = eap
CONFIG   += console
CONFIG   -= app_bundle
CONFIG +=link_pkgconfig

TEMPLATE = app

PKGCONFIG = pulsecore

SOURCES += test_eap.cpp \
    ../drc/drc.c \
    ../eap/eap_wfir_two_bands_int32.c \
    ../eap/eap_wfir_three_bands_int32.c \
    ../eap/eap_wfir_four_bands_int32.c \
    ../eap/eap_wfir_five_bands_int32.c \
    ../eap/eap_wfir_dummy_int32.c \
    ../eap/eap_qmf_stereo_int32.c \
    ../eap/eap_multiband_drc_int32.c \
    ../eap/eap_multiband_drc_control_int32.c \
    ../eap/eap_multiband_drc_control.c \
    ../eap/eap_memory.c \
    ../eap/eap_mdrc_delays_and_gains_int32.c \
    ../eap/eap_limiter_int32.c \
    ../eap/eap_average_amplitude_int32.c \
    ../eap/eap_att_rel_filter_int32.c \
    ../eap/eap_amplitude_to_gain_int32.c

INCLUDEPATH += ../src/eap

test.path = /opt/test_eap/bin
test.files = test.raw

maemo5 {
    target.path = /opt/test_eap/bin
    INSTALLS += target test
}

OTHER_FILES += \
    qtc_packaging/debian_fremantle/rules \
    qtc_packaging/debian_fremantle/README \
    qtc_packaging/debian_fremantle/copyright \
    qtc_packaging/debian_fremantle/control \
    qtc_packaging/debian_fremantle/compat \
    qtc_packaging/debian_fremantle/changelog

QMAKE_CXXFLAGS = -O3 -g -Wall -mfpu=neon -mtune=cortex-a8 -DPACKAGE=pulseaudio -fomit-frame-pointer
QMAKE_CFLAGS = -O3 -g -Wall -mfpu=neon -mtune=cortex-a8 -DPACKAGE=pulseaudio -fomit-frame-pointer
