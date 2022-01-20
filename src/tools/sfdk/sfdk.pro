QTC_LIB_DEPENDS += \
 sfdk \
 ssh \
 utils

CONFIG += qtc_human_user_tool

include(../../qtcreatortool.pri)

QT += dbus network xmlpatterns qml
QT -= gui

*-g++*:QMAKE_CXXFLAGS += -Wall -Werror

DEFINES += QT_NO_URL_CAST_FROM_STRING

HEADERS = \
    cmake.h \
    command.h \
    commandlineparser.h \
    configuration.h \
    dbus.h \
    debugger.h \
    dispatch.h \
    sdkmanager.h \
    remoteprocess.h \
    script.h \
    session.h \
    sfdkconstants.h \
    sfdkglobal.h \
    task.h \
    textutils.h \

SOURCES = \
    cmake.cpp \
    command.cpp \
    commandlineparser.cpp \
    configuration.cpp \
    dbus.cpp \
    debugger.cpp \
    dispatch.cpp \
    main.cpp \
    remoteprocess.cpp \
    script.cpp \
    sdkmanager.cpp \
    session.cpp \
    sfdkglobal.cpp \
    task.cpp \
    textutils.cpp \

win32 {
    HEADERS += 3rdparty/iscygpty.h
    SOURCES += 3rdparty/iscygpty.c
}
