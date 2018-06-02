QT       += core gui

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hexeditor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    cconfigdialog.cpp \
    ccfontsize.cpp \
    cpropertyview.cpp \
    chexviewmodel.cpp \
    chexviewcustom.cpp \
    chexviewselectionmodel.cpp \
    chexviewverticalheader.cpp \
    dialogsavetofile.cpp \
    ceditview.cpp

HEADERS += \
    mainwindow.h \
    defines.h \
    cconfigdialog.h \
    ccfontsize.h \
    cpropertyview.h \
    chexviewmodel.h \
    chexviewcustom.h \
    chexviewselectionmodel.h \
    chexviewverticalheader.h \
    dialogsavetofile.h \
    ceditview.h \
    ver.h \
    versionhelper.h

FORMS += \
        mainwindow.ui \
    cconfigdialog.ui \
    dialogsavetofile.ui

RESOURCES += \
    hexeditor.qrc

DISTFILES += \
    README.md \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/hexeditor.install \
    debian/changelog \
    debian/rules \
    debian/source/format \
    data/hexeditor.desktop \
    ISSUE_TEMPLATE.md \
    LICENSE \
    appveyor.yml \
    version.ps1

win32 {
  RC_FILE     += hexeditor.rc
  OTHER_FILES += hexeditor.rc
}
