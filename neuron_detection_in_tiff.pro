TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    tomo_tiff.cpp

INCLUDEPATH += /usr/local/include/
LIBS += -L/usr/local/lib/ -ltiff

HEADERS += \
    tomo_tiff.h

QMAKE_MAC_SDK = macosx10.10
