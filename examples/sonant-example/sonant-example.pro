QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

#DEFINES += AUDIO_DEBUG
DEFINES += DUMMY

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -L/usr/lib -lSDL2 -lsndfile
LIBS += -L/usr/local/lib -lwhisper

INCLUDEPATH += \
        .. \
        ../..

HEADERS += \
        ../../sonantmanager_p.h \
        ../../sonantworker.h \
        ../common.h \
        ../../sonantmanager.h

SOURCES += \
        ../../sonantmanager.cpp \
        ../../sonantworker.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
