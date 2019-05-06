/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CPROPERTYVIEW_H
#define CPROPERTYVIEW_H

#include <QFile>
#include <QItemDelegate>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTreeView>


class CPropertyViewDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    CPropertyViewDelegate(QObject* parent = nullptr) : QItemDelegate(parent)
    {
    }

    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        return index.column() == 2 ? QItemDelegate::createEditor(parent, option, index) : nullptr;
    }

    virtual void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const override
    {
    }

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();
        painter->setPen(QColor(Qt::lightGray));
        painter->drawRect(option.rect);
        painter->restore();
        QItemDelegate::paint(painter, option, index);
    }
};

class CPropertyView
{
public:

    CPropertyView(QTreeView* propertyView);
    ~CPropertyView();
    void DecodeValue(int64_t pos);
    void OpenFile(const QString& path);
    void Close();
    QFile* GetFileHandler();
    void SetDisplayText(bool displayText);
private:
    void Init();
    void DecodeValue(char* pBuffer, unsigned int bufferSize);
    QTreeView* m_propertyView = nullptr;
    int         m_value_column = 0;
    QFile       m_file;
    mutable std::vector<unsigned char> m_buffer;
    const uint32_t m_string_len = 256;
    bool m_displayText = true;
};

#endif // CPROPERTYVIEW_H
