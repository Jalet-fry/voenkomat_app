QT += core widgets sql network

CONFIG += c++11

TARGET = voenkomat_app
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/DatabaseManager.cpp \
    src/MainWindow.cpp \
    src/TablesWindow.cpp \
    src/QueriesWindow.cpp \
    src/TableViewWindow.cpp \
    src/BackupManager.cpp \
    src/QueryResultWindow.cpp \
    src/RecordDialog.cpp \
    src/ConfigManager.cpp \
    src/CreateTableDialog.cpp \
    src/EditTableStructureDialog.cpp \
    src/EmbeddedQueries.cpp

HEADERS += \
    src/DatabaseManager.h \
    src/MainWindow.h \
    src/TablesWindow.h \
    src/QueriesWindow.h \
    src/TableViewWindow.h \
    src/BackupManager.h \
    src/QueryResultWindow.h \
    src/RecordDialog.h \
    src/ConfigManager.h \
    src/CreateTableDialog.h \
    src/EditTableStructureDialog.h \
    src/EmbeddedQueries.h \
    src/DbConstants.h

RESOURCES +=

win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/debug
    } else {
        DESTDIR = $$PWD/release
    }
}

RESOURCES_PATH = $$PWD/resources
include(qxlsx/QXlsx/QXlsx.pri)
