/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CCFONTSIZE_H
#define CCFONTSIZE_H

#include <QSlider>
#include <QFontComboBox>
#include <functional>

#define DEFCFG_FONTSIZE   "base/fontSize"
#define DEFCFG_FONTFAMILY "base/FONTFAMILY"


class CCFontSize : public QObject
{
    Q_OBJECT

public:
    explicit CCFontSize(QObject *parent = nullptr);

    bool Init(QSlider *slider, QFontComboBox *pFont, QWidget *pObj);
    void LoadConfig();
    void SetUpdateCallback(std::function<void(void)> callbackUpdate);
    void Reset();

private slots:
    void SetFontSize(int fontIndex);
    void SetFontFamily(const QFont &f);

private:
    int                 m_fontSize;
    int                 m_defaultFontSize;
    QString             m_defaultFontFamily;

    QWidget             *m_qwidget;
    QSlider             *m_slider;
    QFontComboBox       *m_pFont;
    QList<int>          m_fontSizeList;
    std::function<void(void)> m_callbackUpdate;

};

#endif // CCFONTSIZE_H
