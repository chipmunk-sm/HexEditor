/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include "cpropertyview.h"

#include <QMessageBox>
#include <QStandardItemModel>
#include <sstream>
#include <iomanip>


#define GEN_LIST_PARAM\
    MACRO_PROP(int8_t)\
    MACRO_PROP(uint8_t)\
    MACRO_PROP(int16_t)\
    MACRO_PROP(uint16_t)\
    MACRO_PROP(int32_t)\
    MACRO_PROP(uint32_t)\
    MACRO_PROP(int64_t)\
    MACRO_PROP(uint64_t)\
    /*MACRO_PROP(__int128_t)*/\
    /*MACRO_PROP(__uint128_t)*/\
    MACRO_PROP(float)\
    MACRO_PROP(double)\
    MACRO_PROP(long double)

CPropertyView::CPropertyView(QTreeView* propertyView)
{
    m_propertyView = propertyView;
    propertyView->setItemDelegate(new CPropertyViewDelegate(propertyView));
    Init();
}

CPropertyView::~CPropertyView()
{
    if (m_file.isOpen())
        m_file.close();
}

void CPropertyView::Init()
{

    // prepare read buffer

#undef  MACRO_PROP
#define MACRO_PROP(XIDNAME) \
    if( m_buffer_len < sizeof(XIDNAME))\
        m_buffer_len = sizeof(XIDNAME);

    GEN_LIST_PARAM;

#undef  MACRO_PROP

    m_buffer_len = std::max(m_buffer_len, m_string_len);
    m_buffer_len = std::max(m_buffer_len, static_cast<uint32_t>(sizeof(UUID)));

    m_buffer.resize(m_buffer_len * 2, 0);

    // column

    QStringList cols;
    cols << QObject::tr("Type Name")
        << QObject::tr("Size")
        << QObject::tr("Value");

    m_value_column = cols.size() - 1;

    auto pModel = new QStandardItemModel(0, cols.size(), m_propertyView);
    pModel->setHorizontalHeaderLabels(cols);
    m_propertyView->setModel(pModel);

    // rows

    auto rows = QList<QStringList>()

#undef  MACRO_PROP
#define MACRO_PROP(XIDNAME) << (QStringList() << #XIDNAME <<  std::to_string(sizeof(XIDNAME)).c_str())
        GEN_LIST_PARAM
        GEN_LIST_PARAM;
#undef  MACRO_PROP

    rows.append(QStringList() << "UUID" << std::to_string(sizeof(UUID)).c_str());

    rows.append(QStringList() << "Latin1" << (std::string("Max ") + std::to_string(m_buffer_len)).c_str());
    rows.append(QStringList() << "Utf8" << (std::string("Max ") + std::to_string(m_buffer_len)).c_str());
    rows.append(QStringList() << "Utf16" << (std::string("Max ") + std::to_string(m_buffer_len)).c_str());
    rows.append(QStringList() << "Ucs4" << (std::string("Max ") + std::to_string(m_buffer_len)).c_str());

    for(const auto & row : rows)
    {
        QList<QStandardItem*> items;
        for(const QString& text : row)
            items.append(new QStandardItem(text));

        pModel->appendRow(items);
    }
}

void CPropertyView::OpenFile(const QString& path)
{
    if (m_file.isOpen())
        m_file.close();

    m_file.setFileName(path);

    if (!m_file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(m_propertyView, QObject::tr("Open"), m_file.errorString(), QMessageBox::Ok);
        return;
    }
}

void CPropertyView::Close()
{
    if (m_file.isOpen())
        m_file.close();
    DecodeValue(nullptr, 0);
}

QFile* CPropertyView::GetFileHandler()
{
    return &m_file;
}

void CPropertyView::SetDisplayText(bool displayText)
{
    m_displayText = displayText;
    //    if(!displayText)
    //        DecodeValue(nullptr, 0);
}

void CPropertyView::DecodeValue(int64_t pos)
{
    if (m_file.isOpen() && pos >= 0 && m_file.seek(pos))
    {
        auto len = m_file.read(reinterpret_cast<char*>(m_buffer.data()), m_buffer_len);
        if (len > 0)
        {

            if(len < m_buffer_len)
                memset(m_buffer.data() + len, 0, m_buffer_len - static_cast<uint32_t>(len));

            DecodeValue(reinterpret_cast<char*>(m_buffer.data()), static_cast<uint32_t>(len));

            return;
        }
    }

    DecodeValue(nullptr, 0);
}

void CPropertyView::DecodeValue(char* pBuffer, unsigned int bufferSize)
{
    auto pModel = m_propertyView->model();
    auto index = 0;

    const auto hexPrefixEnd = 8;

#undef  MACRO_PROP

#define MACRO_PROP(XIDNAME)\
    try {\
    if(bufferSize >= sizeof(XIDNAME)){\
    std::stringstream ss;\
    if(index < 2){\
    ss << "0x" << std::setw(sizeof(XIDNAME) * 2) << std::setfill('0') << std::uppercase << std::hex << static_cast<uint32_t>(*reinterpret_cast<uint8_t*>(pBuffer));\
}else{\
    ss << (index < hexPrefixEnd ? "0x" : "") << std::setw(sizeof(XIDNAME) * (index < hexPrefixEnd ? 2 : 0)) << std::setfill('0') << std::uppercase  << std::hex << *reinterpret_cast<XIDNAME*>(pBuffer);\
}\
    pModel->setData(pModel->index(index, m_value_column), ss.str().c_str());\
}else{\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
} catch (...) {\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
    index++;


    GEN_LIST_PARAM;

#undef  MACRO_PROP

#define MACRO_PROP(XIDNAME)\
    try {\
    if(bufferSize >= sizeof(XIDNAME)){\
    std::stringstream ss;\
    ss << std::to_string(*reinterpret_cast<XIDNAME*>(pBuffer));\
    pModel->setData(pModel->index(index, m_value_column), ss.str().c_str());\
}else{\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
} catch (...) {\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
    index++;

    GEN_LIST_PARAM;

#undef  MACRO_PROP

    QString str;

    if (bufferSize >= sizeof(UUID))
    {
        try
        {
            str = UuidToString(reinterpret_cast<UUID*>(pBuffer)).c_str();
        }
        catch (...)
        {
            str = "-";
        }
    }
    else
    {
        str = "-";
    }

    pModel->setData(pModel->index(index++, m_value_column), str);


    if (!m_displayText)
    {
        pModel->setData(pModel->index(index++, m_value_column), "");
        pModel->setData(pModel->index(index++, m_value_column), "");
        pModel->setData(pModel->index(index++, m_value_column), "");
        pModel->setData(pModel->index(index++, m_value_column), "");
    }
    else
    {
        try
        {
            str = QString::fromLatin1(pBuffer);
        }
        catch (...)
        {
            str = "-";
        }

        pModel->setData(pModel->index(index++, m_value_column), str);

        try
        {
            str = QString::fromUtf8(pBuffer);
        }
        catch (...)
        {
            str = "-";
        }

        pModel->setData(pModel->index(index++, m_value_column), str);

        if (bufferSize >= sizeof(ushort))
        {
            try
            {
                str = QString::fromUtf16(reinterpret_cast<ushort*>(pBuffer));
            }
            catch (...)
            {
                str = "-";
            }
        }
        else
        {
            str = "-";
        }

        pModel->setData(pModel->index(index++, m_value_column), str);

        if (bufferSize >= sizeof(uint))
        {
            try
            {
                str = QString::fromUcs4(reinterpret_cast<uint*>(pBuffer));
            }
            catch (...)
            {
                str = "-";
            }
        }
        else
        {
            str = "-";
        }

        pModel->setData(pModel->index(index++, m_value_column), str);
    }
}

std::string CPropertyView::UuidToString(const UUID* pId)
{
    char buff[256];
    sprintf(buff, "{ 0x%08x, 0x%04x, 0x%04x, {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x} }",
        pId->Data1, pId->Data2, pId->Data3,
        pId->Data4[0], pId->Data4[1], pId->Data4[2], pId->Data4[3],
        pId->Data4[4], pId->Data4[5], pId->Data4[6], pId->Data4[7]);
    return std::string(buff);
}

