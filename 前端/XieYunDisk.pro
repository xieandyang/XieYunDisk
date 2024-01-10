#-------------------------------------------------
#
# Project created by QtCreator 2023-11-23T17:35:14
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = XieYunDisk
TEMPLATE = app
RC_ICONS = ./images/logo.ico

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        login.cpp \
    selfwidget/titlewidget.cpp \
    common/common.cpp \
    mainwindow.cpp \
    buttongroup.cpp \
    myfilewg.cpp \
    sharelist.cpp \
    rankinglist.cpp \
    transfer.cpp \
    selfwidget/dataprogress.cpp \
    selfwidget/filepropertyinfo.cpp \
    common/des.c \
    common/logininfoinstance.cpp \
    selfwidget/mymenu.cpp \
    common/uploadlayout.cpp \
    common/uploadtask.cpp \
    common/downloadlayout.cpp \
    common/downloadtask.cpp \
    common/uploadthread.cpp

HEADERS += \
        login.h \
    selfwidget/titlewidget.h \
    common/common.h \
    mainwindow.h \
    buttongroup.h \
    myfilewg.h \
    sharelist.h \
    rankinglist.h \
    transfer.h \
    selfwidget/dataprogress.h \
    selfwidget/filepropertyinfo.h \
    common/des.h \
    common/logininfoinstance.h \
    selfwidget/mymenu.h \
    common/uploadlayout.h \
    common/uploadtask.h \
    common/downloadlayout.h \
    common/downloadtask.h \
    common/uploadthread.h

FORMS += \
        login.ui \
    selfwidget/titlewidget.ui \
    mainwindow.ui \
    buttongroup.ui \
    myfilewg.ui \
    sharelist.ui \
    rankinglist.ui \
    transfer.ui \
    selfwidget/dataprogress.ui \
    selfwidget/filepropertyinfo.ui

RESOURCES += \
    resource.qrc
