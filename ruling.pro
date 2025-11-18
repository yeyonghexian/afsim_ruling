QT += core gui widgets
CONFIG += c++17 console
CONFIG -= app_bundle
TEMPLATE = app

#QT       += core gui

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#CONFIG += c++11

#DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    environmentgridwidget.cpp \
    adjudicationengine.cpp \
    manualadjudicationdialog.cpp \
    taskmanagerdialog.cpp \
    rulemodelmanagerdialog.cpp

HEADERS += \
    mainwindow.h \
    environmentgridwidget.h \
    models.h \
    adjudicationengine.h \
    manualadjudicationdialog.h \
    taskmanagerdialog.h \
    rulemodelmanagerdialog.h

#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target
