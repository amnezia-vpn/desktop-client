TARGET   = AmneziaVPN-service
TEMPLATE = app
CONFIG   += console qt no_batch
QT += core network remoteobjects

HEADERS = \
        ../../client/utils.h \
        ../../ipc/ipc.h \
        ../../ipc/ipcserver.h \
        ../../ipc/ipcserverprocess.h \
        localserver.h \
        log.h \
        router.h \
        systemservice.h

SOURCES = \
        ../../client/utils.cpp \
        ../../ipc/ipcserver.cpp \
        ../../ipc/ipcserverprocess.cpp \
        localserver.cpp \
        log.cpp \
        main.cpp \
        router.cpp \
        systemservice.cpp

win32 {
HEADERS += \
        tapcontroller_win.h \
        router_win.h

SOURCES += \
        tapcontroller_win.cpp \
        router_win.cpp

LIBS += \
        -luser32 \
        -lrasapi32 \
        -lshlwapi \
        -liphlpapi \
        -lws2_32 \
        -liphlpapi \
        -lgdi32 \
        -lAdvapi32 \
        -lKernel32
}

macx {
HEADERS += \
    router_mac.h \
    helper_route_mac.h

SOURCES += \
    router_mac.cpp \
    helper_route_mac.c
}

linux {
HEADERS += \
    router_linux.h

SOURCES += \
    router_linux.cpp
}

include(../src/qtservice.pri)

#CONFIG(release, debug|release) {
#    DESTDIR = $$PWD/../../../AmneziaVPN-build/server/release
#    MOC_DIR = $$DESTDIR
#    OBJECTS_DIR = $$DESTDIR
#    RCC_DIR = $$DESTDIR
#}

INCLUDEPATH += "$$PWD/../../client"

REPC_SOURCE += ../../ipc/ipcinterface.rep
