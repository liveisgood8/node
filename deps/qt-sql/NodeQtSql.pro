QT -= gui
QT += sql

TEMPLATE = lib
DEFINES += NODEQTSQL_LIBRARY
DEFINES += NODEQTSQL_STATIC

CONFIG += static
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    ../../src/ \
    ../v8/include/ \
    deps/ \
    $$PWD/src/

SOURCES += \
    src/bindings/nodequery.cpp \
    src/db/connection.cpp \
    src/db/connection_sources.cpp \
    src/db/oledbconnectionparser.cpp \
    src/nodeqtsql.cpp \
    src/exception.cpp \
    deps/logger/easylogging++.cc

HEADERS += \
    src/bindings/nodequery.h \
    src/db/connection.h \
    src/db/connection_sources.h \
    src/db/oledbconnectionparser.h \
    src/global.h \
    src/nodeqtsql.h \
    src/exception.h \
    deps/logger/easylogging++.h \
    deps/logger/configuration.h

Release:DESTDIR = ../../out/Release/lib
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = ../../out/Debug/lib
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
