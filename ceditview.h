/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CEDITVIEW_H
#define CEDITVIEW_H

#include "dialogsavetofile.h"

#include <QListView>
#include <QAbstractListModel>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QPlainTextEdit>
#include <QFile>
#include <QLabel>
#include <QSpinBox>

enum CEditEvent
{
    CEditEventNone,
    CEditEventOverwrite,
    CEditEventInsert,
    CEditEventDelete
};

struct CEditItem
{
    bool                 valid      = false;
    CEditEvent           event      = CEditEvent::CEditEventNone;
    int64_t              pos        = -1;
    int64_t              deleteSize = -1;

    std::vector<uint8_t> data;

    QString GetInfo() const
    {

        if(!valid)
            return "";

        QString str;
        switch (event)
        {
            case CEditEvent::CEditEventOverwrite:
                str += "Overwrite " + QString::number(data.size()) + " byte(s) From ";
            break;
            case CEditEvent::CEditEventInsert:
                str += "Insert "    + QString::number(data.size()) + " byte(s) Before ";
            break;
            case CEditEvent::CEditEventDelete:
                str += "Delete "    + QString::number(deleteSize)  + " byte(s) From ";
            break;
            case CEditEvent::CEditEventNone: return "Error";
        }

        str += QString("%1 (HEX: %2)").arg(pos, 1, 10, QLatin1Char('0')).arg(pos, 1, 16, QLatin1Char('0')).toUpper();

        return str;
    }

    void clear()
    {
        valid = false;
        event = CEditEvent::CEditEventNone;
        pos = -1;
        deleteSize = -1;
        data.clear();
    }

};

class CEditView : public QObject
{

    Q_OBJECT

public:
    explicit CEditView(QLabel *labelOperationInfo,
                       QPlainTextEdit *textEdit,
                       QSpinBox *spinBox_bytesCountToDelete,
                       QButtonGroup *buttonGroup_EditInputType,
                       QButtonGroup *buttonGroup_EditOverwrite);

    void Clear();
    bool Apply(QFile *pFile1, QFile *pFile2, DialogSaveToFile *infoDialog);
    CEditEvent GetCellStatus(int64_t row, int col, int cols_hex);
    QString GetEditStatus(int64_t row, int col, int cols_hex);
    void setUpdateCallback(std::function<void(void)> callbackUpdate);
    void setGetIndexCallback(std::function<int64_t(void)> callbackGetIndex);

public slots:
    int validateInput();

private:
    int HighlightError(const QString &src);
    void ClearHighlightChar();
    void ConvertHexText(QString &src, std::vector<uint8_t> &data);

private:
    QLabel         *m_pLabelOperationInfo;
    QPlainTextEdit *m_pQTextEdit;
    QSpinBox       *m_pSpinBox_bytesCountToDelete;

    CEditItem       m_event;

    QButtonGroup *m_buttonGroup_EditInputType;
    QButtonGroup *m_buttonGroup_EditOverwrite;

    std::function<void(void)>    m_callbackUpdate;
    std::function<int64_t(void)> m_callbackGetIndex;

};

#endif // CEDITVIEW_H
