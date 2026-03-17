TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    voenkomat_app.pro \
    server_cpp/server_cpp.pro

# Чтобы сервер не зависел от того, откуда его запускают, можно добавить зависимости
# voenkomat_app.depends = server_cpp/server_cpp.pro
