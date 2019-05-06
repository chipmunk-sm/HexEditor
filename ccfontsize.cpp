/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include <QtWidgets>

#include "ccfontsize.h"

CCFontSize::CCFontSize(QObject *parent)
    : QObject(parent)
    , m_fontSize(-1)
    , m_qwidget(nullptr)
{

}

bool CCFontSize::Init(QSlider *slider, QFontComboBox *pFont, QWidget *pObj)
{

    if(m_fontSize != -1)
        return false;

    m_qwidget = pObj;
    m_slider = slider;
    m_pFont = pFont;

    m_defaultFontSize = m_qwidget->font().pointSize();
    m_defaultFontFamily = m_qwidget->font().family();

    LoadConfig();

    if(slider)
        QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(SetFontSize(int)));
    if(pFont)
        QObject::connect(pFont,  SIGNAL(currentFontChanged(QFont)), this, SLOT(SetFontFamily(QFont)));

    return true;
}

void CCFontSize::LoadConfig()
{

    m_slider->blockSignals(true);
    m_slider->setSingleStep(1);
    m_slider->setPageStep(1);

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    auto initialFontSize = settings.value(QString(DEFCFG_FONTSIZE), m_qwidget->font().pointSize());
    auto fontSize = initialFontSize.toInt();

    m_fontSizeList = QFontDatabase::standardSizes();
    m_slider->setRange(0, m_fontSizeList.length() - 1);

    auto index = m_fontSizeList.indexOf(fontSize);
    if(index < 0)
    {
        for(QList<int>::iterator it(m_fontSizeList.begin()); it != m_fontSizeList.end(); ++it)
        {
            if(*it < fontSize)
                continue;
            m_fontSizeList.insert(it,fontSize);
            break;
        }
        index = m_fontSizeList.indexOf(fontSize);
    }

    if(index >= 0)
        m_slider->setValue(index);

    if(index >= 0)
        m_fontSize = fontSize;

    if(index >= 0)
        SetFontSize(index);

    m_slider->blockSignals(false);

    auto loadedFont = settings.value(DEFCFG_FONTFAMILY).toString();
    if(!loadedFont.isEmpty() && m_qwidget)
    {
        QFont newFont(m_qwidget->font());
        newFont.setFamily(loadedFont);
        m_qwidget->setFont(newFont);

        if( m_pFont )
            m_pFont->setCurrentFont(newFont);
    }

}

void CCFontSize::SetFontSize(int fontIndex)
{

    if(m_fontSize < 0 || m_qwidget == nullptr)
        return;

    if( fontIndex >= m_fontSizeList.length() )
        fontIndex  = m_fontSizeList.length() - 1;

    m_fontSize = m_fontSizeList[fontIndex];

    auto font = m_qwidget->font();

    font.setPointSize(m_fontSize);

    m_qwidget->setFont(font);

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue(QString(DEFCFG_FONTSIZE), m_fontSize);

    if(m_callbackUpdate != nullptr)
        m_callbackUpdate();
}

void CCFontSize::SetUpdateCallback(std::function<void ()> callbackUpdate)
{
    m_callbackUpdate = callbackUpdate;
}

void CCFontSize::Reset()
{
    {
        QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
        settings.setValue(DEFCFG_FONTSIZE,   m_defaultFontSize);
        if(!m_defaultFontFamily.isEmpty())
            settings.setValue(DEFCFG_FONTFAMILY, m_defaultFontFamily);
    }
    LoadConfig();
}

void CCFontSize::SetFontFamily(const QFont &f)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue(DEFCFG_FONTFAMILY, f.family());

    if(!m_qwidget)
        return;

    QFont newFont(m_qwidget->font());
    newFont.setFamily(f.family());
    m_qwidget->setFont(newFont);

    if(m_callbackUpdate != nullptr)
        m_callbackUpdate();
}

