QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

#DEFINES += USING_SDL2
#DEFINES += USING_SNDFILE
#DEFINES += SAVE_TO_FILE

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    $${PWD}/..

SOURCES += \
        source.cpp

LIBS += -L/usr/lib/x86_64-linux-gnu -lsndfile -lSDL2
LIBS += -L/usr/local/lib -lwhisper

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    $${PWD}/../common.h
