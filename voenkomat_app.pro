QT += core widgets sql

CONFIG += c++11

TARGET = voenkomat_app
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/DatabaseManager.cpp \
    src/MainWindow.cpp \
    src/TablesWindow.cpp \
    src/QueriesWindow.cpp \
    src/TableEditWindow.cpp \
    src/TableViewWindow.cpp \
    src/BackupManager.cpp \
    src/QueryResultWindow.cpp \
    src/TableAdditionWindow.cpp \
    src/RecordDialog.cpp \
    src/ConfigManager.cpp

HEADERS += \
    src/DatabaseManager.h \
    src/MainWindow.h \
    src/TablesWindow.h \
    src/QueriesWindow.h \
    src/TableEditWindow.h \
    src/TableViewWindow.h \
    src/BackupManager.h \
    src/QueryResultWindow.h \
    src/TableAdditionWindow.h \
    src/RecordDialog.h \
    src/ConfigManager.h

RESOURCES +=

# Определение выходной директории
win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/debug
    } else {
        DESTDIR = $$PWD/release
    }
}

# Путь к ресурсам (относительно директории проекта)
# Приложение будет искать ресурсы в нескольких местах:
# 1. В директории приложения (для релизных сборок)
# 2. В директории проекта (для разработки)
RESOURCES_PATH = $$PWD/resources

