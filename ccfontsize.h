/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

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
    explicit CCFontSize(QObject* parent = nullptr);

    bool Init(QSlider* slider, QFontComboBox* pFont, QWidget* pObj);
    void LoadConfig();
    void SetUpdateCallback(std::function<void(void)> callbackUpdate);
    void Reset();

private slots:
    void SetFontSize(int fontIndex);
    void SetFontFamily(const QFont& f);

private:
    int                 m_fontSize = 0;
    int                 m_defaultFontSize = 0;
    QString             m_defaultFontFamily;

    QWidget* m_qwidget = nullptr;
    QSlider* m_slider = nullptr;
    QFontComboBox* m_pFont = nullptr;
    QList<int>          m_fontSizeList;
    std::function<void(void)> m_callbackUpdate;

};

#endif // CCFONTSIZE_H
