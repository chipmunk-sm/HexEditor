/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CLANGUAGE_H
#define CLANGUAGE_H

#include <QStringList>
#include <QMap>
#include <QMainWindow>
#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <QMap>
#include <QComboBox>
#include <functional>

class CLanguage : public QObject
{
    Q_OBJECT

public:

    explicit CLanguage(QObject* parent = nullptr);
    ~CLanguage() override;
    bool SetLang(const QString& langName);
    void SetLangByLocale();
    bool SetLangByLocale(QString localeName);
    const QStringList GetListLangNames();
    void LoadTranslations(const QDir& dir);
    QString ExtractLanguageName(const QString& fileName);
    void InitCombo(QComboBox* comboBox_language, std::function<void()> callbackUpdate);
    void SetLangByConfig();
private:

    QStringList            m_langNames;
    QMap<QString, QString> m_langList;
    QTranslator            m_translator;
    //QMap<QString, QString> m_langNamesList;
    std::function<void(void)> m_callbackUpdate;


private slots:
    void comboCurrentIndexChanged(const QString& val);

};

#endif // CLANGUAGE_H
