/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

#include "clanguage.h"
#include "versionhelper.h"

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(hexeditor);

    QApplication application(argc, argv);

    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setOrganizationName("chipmunk-sm");
    QCoreApplication::setApplicationName("ChipmunkHexEditor");
    QCoreApplication::setApplicationVersion(PRODUCTVERSIONSTRING);
    //    QString translatorFileName = QLatin1String("qt_");
    //    translatorFileName += QLocale::system().name();
    //    QTranslator *translator = new QTranslator(&app);
    //    if (translator->load(translatorFileName, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    //        app.installTranslator(translator);


    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption source("source", "Open source file --source=\"file path name\"", "file");
    parser.addOption(source);

    QCommandLineOption hex("hex", "HEX string to search --hex=\"50..fe.ff\"", "hex");
    parser.addOption(hex);

    parser.process(application);

    MainWindow window;

    QString sourceFile;
    QString hexString;

    auto sourceSet = parser.isSet(source);
    auto hexSet = parser.isSet(hex);

    if (sourceSet)
        sourceFile = parser.value(source);

    if (hexSet)
        hexString = parser.value(hex);

    if (sourceSet || hexSet)
        window.RunFromCmd(sourceFile, hexString);

    window.show();

    return application.exec();
}
