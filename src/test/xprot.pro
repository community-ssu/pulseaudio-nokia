#-------------------------------------------------
#
# Project created by QtCreator 2014-10-18T14:30:03
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = xprot_test
CONFIG   += console
CONFIG   -= app_bundle
CONFIG +=link_pkgconfig

TEMPLATE = app

PKGCONFIG = pulsecore

SOURCES += xprot_test.cpp \
    ../xprot/xprot.c

test.path = /opt/xprot_test/bin
test.files = test.raw

LIBS += ../xprot/.libs/a_xprot.o

maemo5 {
    target.path = /opt/xprot_test/bin
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
