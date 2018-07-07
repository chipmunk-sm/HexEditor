/* Copyright (C) 2018 chipmunk-sm <dannico@linuxmail.org> */

#include "mainwindow.h"
#include <QApplication>

#include "clanguage.h"
#include "versionhelper.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(hexeditor);

    QApplication a(argc, argv);

    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setOrganizationName("chipmunk-sm");
    QCoreApplication::setApplicationName("ChipmunkHexEditor");
    QCoreApplication::setApplicationVersion(PRODUCTVERSIONSTRING);
    //    QString translatorFileName = QLatin1String("qt_");
    //    translatorFileName += QLocale::system().name();
    //    QTranslator *translator = new QTranslator(&app);
    //    if (translator->load(translatorFileName, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    //        app.installTranslator(translator);

    CLanguage        m_lang;
    m_lang.SetLangByLocale();

    MainWindow w;
    w.show();

    return a.exec();
}
