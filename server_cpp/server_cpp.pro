QT += core network sql httpserver

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = voenkomat_server
TEMPLATE = app

SOURCES += main.cpp \
    DatabaseManager.cpp \
    NoSQLManager.cpp \
    RestServer.cpp \
    Converter.cpp \
    DataGenerator.cpp

HEADERS += \
    DatabaseManager.h \
    NoSQLManager.h \
    RestServer.h \
    Converter.h \
    DataGenerator.h

# Путь к библиотекам PostgreSQL
INCLUDEPATH += "C:/Program Files/PostgreSQL/16/include"
# LIBS += -L"C:/Program Files/PostgreSQL/16/lib" -lpq

# --- NOSQL CONFIGURATION (PURE QT IMPLEMENTATION) ---
# Мы используем встроенную сериализацию Qt для создания бинарных .db файлов.
# Это позволяет избежать проблем с архитектурами (32/64 бит) и упрощает переносимость.
# -----------------------------------------------------
