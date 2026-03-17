QT += core network sql httpserver

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = voenkomat_server
TEMPLATE = app

SOURCES += main.cpp \
    DatabaseManager.cpp \
    RestServer.cpp \
    Converter.cpp \
    DataGenerator.cpp

HEADERS += \
    DatabaseManager.h \
    RestServer.h \
    Converter.h \
    DataGenerator.h

# Путь к библиотекам PostgreSQL
INCLUDEPATH += "C:/Program Files/PostgreSQL/16/include"
# LIBS += -L"C:/Program Files/PostgreSQL/16/lib" -lpq
