#-------------------------------------------------
#
# Project created by QtCreator 2012-05-10T10:48:40
#
#-------------------------------------------------
TEMPLATE = app

QT      += core gui
TARGET   = qspy_gui
DEFINES += QT_NO_STATEMACHINE

INCLUDEPATH = . \
    INCLUDEPATH += ../include

HEADERS += \
    ../include/hal.h \
    ../include/qs_copy.h \
    ../include/qspy.h \
    qspy_app.h \
    qspy_thread.h \
    qspy_gui.h

SOURCES += \
    ../source/getopt.c \
    ../source/qspy.c \
    qspy_app.cpp \
    qspy_gui.cpp \
    main.cpp \
    qspy_thread.cpp

win32 {
    SOURCES += \
        win32/com.c \
        win32/tcp.c
} else {
    SOURCES += \
        posix/com.c \
        posix/tcp.c
}

RESOURCES = qspy_gui.qrc
