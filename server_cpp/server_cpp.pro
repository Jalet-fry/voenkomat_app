QT += core network sql httpserver

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = voenkomat_server
TEMPLATE = app

SOURCES += main.cpp \
    DatabaseManager.cpp \
    RestServer.cpp

HEADERS += \
    DatabaseManager.h \
    RestServer.h

# Путь к библиотекам PostgreSQL (настройте под свою систему, если нужно)
 INCLUDEPATH += "C:/Program Files/PostgreSQL/16/include"
# LIBS += -L"C:/Program Files/PostgreSQL/16/lib" -lpq
