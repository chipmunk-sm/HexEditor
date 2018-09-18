#/* Copyright (C) 2018 chipmunk-sm <dannico@linuxmail.org> */


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
    ceditview.cpp \
    csearch.cpp \
    cmemorymappedfile.cpp \
    clanguage.cpp

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
    versionhelper.h \
    csearch.h \
    cmemorymappedfile.h \
    clanguage.h

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
    version.ps1 \
    installer.ps1

win32 {
  RC_FILE     += hexeditor.rc
  OTHER_FILES += hexeditor.rc
}

# lupdate -no-obsolete -verbose -pro *.pro
# cd translations
# linguist *.ts
# cd ..
# lrelease -removeidentical -compress *.pro


#prepare for spell checker
#
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_en.ts > language_en.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_cs.ts > language_cs.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_de.ts > language_de.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_fr.ts > language_fr.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_ja.ts > language_ja.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_pl.ts > language_pl.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_ru.ts > language_ru.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_sl.ts > language_sl.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_zh_CN.ts > language_zh_CN.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_zh_TW.ts > language_zh_TW.txt

TRANSLATIONS += \
    translations/language_en.ts \
    translations/language_cs.ts \
    translations/language_de.ts \
    translations/language_fr.ts \
    translations/language_ja.ts \
    translations/language_pl.ts \
    translations/language_ru.ts \
    translations/language_sl.ts \
    translations/language_zh_CN.ts \
    translations/language_zh_TW.ts

unix:!macx {
    isEmpty(PREFIX) {
        PREFIX=/usr
    }

    target.path = $${PREFIX}/bin/

    desktopentry.path = $${PREFIX}/share/applications/
    desktopentry.files = data/$${TARGET}.desktop

    INSTALLS += target desktopentry
}

#af		Afrikaans
#sq		Albanian
#ar		Arabic
#eu		Basque
#be		Belarusian
#bs		Bosnian
#bg		Bulgarian
#ca		Catalan
#hr		Croatian
#zh_cn		Chinese (Simplified)
#zh_tw		Chinese (Traditional)
#cs		Czech
#da		Danish
#nl		Dutch
#en		English
#en_us		English (US)
#et		Estonian
#fa		Farsi
#fil		Filipino
#fi		Finnish
#fr		French
#fr_ca		French (Canada)
#ga		Gaelic
#gl		Gallego
#ka		Georgian
#de		German
#de_du		German (Personal)
#el		Greek
#gu		Gujarati
#he		Hebrew
#hi		Hindi
#hu		Hungarian
#is		Icelandic
#id		Indonesian
#it		Italian
#ja		Japanese
#kn		Kannada
#km		Khmer
#ko		Korean
#lo		Lao
#lt		Lithuanian
#lv		Latvian
#ml		Malayalam
#ms		Malaysian
#mi_tn		Maori (Ngai Tahu)
#mi_wwow	Maori (Waikoto Uni)
#mn		Mongolian
#no		Norwegian
#no_gr		Norwegian (Primary)
#nn		Nynorsk
#pl		Polish
#pt		Portuguese
#pt_br		Portuguese (Brazil)
#ro		Romanian
#ru		Russian
#sm		Samoan
#sr		Serbian
#sk		Slovak
#sl		Slovenian
#so		Somali
#es		Spanish (International)
#sv		Swedish
#tl		Tagalog
#ta		Tamil
#th		Thai
#to		Tongan
#tr		Turkish
#uk		Ukrainian
#vi		Vietnamese
