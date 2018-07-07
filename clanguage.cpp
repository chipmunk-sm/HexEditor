/* Copyright (C) 2018 chipmunk-sm <dannico@linuxmail.org> */


#include "clanguage.h"

#include <QTranslator>
#include <QLibraryInfo>
#include <QFileInfo>
#include <QCoreApplication>
#include <QLocale>
#include <iostream>

#if 0
#   include <QDebug>
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

#define ADDLANG(langPrefix, langName) m_langNamesList.insert(#langPrefix, #langName)

CLanguage::CLanguage()
{
    DEBUGTRACE();
/*
    ADDLANG(af,Afrikaans);
    ADDLANG(sq,Albanian);
    ADDLANG(ar,Arabic);
    ADDLANG(eu,Basque);
    ADDLANG(be,Belarusian);
    ADDLANG(bs,Bosnian);
    ADDLANG(bg,Bulgarian);
    ADDLANG(ca,Catalan);
    ADDLANG(hr,Croatian);
    ADDLANG(zh_cn,Chinese (Simplified));
    ADDLANG(zh_tw,Chinese (Traditional));
    ADDLANG(cs,Czech);
    ADDLANG(da,Danish);
    ADDLANG(nl,Dutch);
    ADDLANG(en,English);
    ADDLANG(en_us,English (US));
    ADDLANG(et,Estonian);
    ADDLANG(fa,Farsi);
    ADDLANG(fil,Filipino);
    ADDLANG(fi,Finnish);
    ADDLANG(fr,French);
    ADDLANG(fr_ca,French (Canada));
    ADDLANG(ga,Gaelic);
    ADDLANG(gl,Gallego);
    ADDLANG(ka,Georgian);
    ADDLANG(de,German);
    ADDLANG(de_du,German (Personal));
    ADDLANG(el,Greek);
    ADDLANG(gu,Gujarati);
    ADDLANG(he,Hebrew);
    ADDLANG(hi,Hindi);
    ADDLANG(hu,Hungarian);
    ADDLANG(is,Icelandic);
    ADDLANG(id,Indonesian);
    ADDLANG(it,Italian);
    ADDLANG(ja,Japanese);
    ADDLANG(kn,Kannada);
    ADDLANG(km,Khmer);
    ADDLANG(ko,Korean);
    ADDLANG(lo,Lao);
    ADDLANG(lt,Lithuanian);
    ADDLANG(lv,Latvian);
    ADDLANG(ml,Malayalam);
    ADDLANG(ms,Malaysian);
    ADDLANG(mi_tn,Maori (Ngai Tahu));
    ADDLANG(mi_wwow,Maori (Waikoto Uni));
    ADDLANG(mn,Mongolian);
    ADDLANG(no,Norwegian);
    ADDLANG(no_gr,Norwegian (Primary));
    ADDLANG(nn,Nynorsk);
    ADDLANG(pl,Polish);
    ADDLANG(pt,Portuguese);
    ADDLANG(pt_br,Portuguese (Brazil));
    ADDLANG(ro,Romanian);
    ADDLANG(ru,Russian);
    ADDLANG(sm,Samoan);
    ADDLANG(sr,Serbian);
    ADDLANG(sk,Slovak);
    ADDLANG(sl,Slovenian);
    ADDLANG(so,Somali);
    ADDLANG(es,Spanish (International));
    ADDLANG(sv,Swedish);
    ADDLANG(tl,Tagalog);
    ADDLANG(ta,Tamil);
    ADDLANG(th,Thai);
    ADDLANG(to,Tongan);
    ADDLANG(tr,Turkish);
    ADDLANG(uk,Ukrainian);
    ADDLANG(vi,Vietnamese);

    //qDebug() << m_langNamesList;
*/
    LoadTranslations(QDir(":/translations"));

}

void CLanguage::LoadTranslations(const QDir &dir)
{
    DEBUGTRACE();
    auto fileNames = dir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);
    foreach (const QString &str, fileNames)
    {
        auto path = dir.filePath(str);
        auto langName = ExtractLanguageName(path);

       // qDebug() << langName << "\t" << str << "\t" << path;

        if(langName.length() < 1)
            continue;

        if(m_langList.find(langName) != m_langList.end())
            continue;

        m_langList.insert(langName, path);
        m_langNames.append(langName);
    }

}

void CLanguage::SetLang(const QString &langName)
{
    DEBUGTRACE();

    auto it= m_langList.find(langName);
    if(it == m_langList.end())
        return;

    if (!m_translator.load(it.value()))
        return;

    QCoreApplication::instance()->installTranslator(&m_translator);
}

void CLanguage::SetLangByLocale()
{
    DEBUGTRACE();

    //xx_XX
    auto localeName = QLocale::system().name();
    if(SetLangByLocale(localeName))
        return;

    //xx
    auto list = localeName.split(QRegExp("(_|-)"), QString::SkipEmptyParts);
    foreach (auto tmp, list)
    {
        if(SetLangByLocale(tmp))
            return;
        break;
    }

}

bool CLanguage::SetLangByLocale(QString localeName)
{
    DEBUGTRACE();

    localeName = QString("language_%1.qm").arg(localeName);

    foreach (auto value, m_langList)
    {
        if(value.contains(localeName, Qt::CaseInsensitive))
        {
            if (!m_translator.load(value))
                return false;

            QCoreApplication::instance()->installTranslator(&m_translator);
            return true;
        }
    }

    return false;

}

const QStringList CLanguage::GetListLangNames()
{
    DEBUGTRACE();
    return m_langNames;
}

QString CLanguage::ExtractLanguageName(const QString &fileName)
{
    DEBUGTRACE();

    auto writeLanguageName = QObject::tr("English");// !!! Change the "English" to the current language name in the file !!!

    Q_UNUSED(writeLanguageName);

    QTranslator translator;
    translator.load(fileName);
    return translator.translate("QObject", "English");
}




