/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "cconfigdialog.h"
#include "ceditview.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QRadioButton>
#include <cctype>

#include "defines.h"

CEditView::CEditView(QLabel *labelOperationInfo,
                     QPlainTextEdit *textEdit,
                     QSpinBox* spinBox_bytesCountToDelete,
                     QButtonGroup *buttonGroup_EditInputType,
                     QButtonGroup *buttonGroup_EditOverwrite)

    : QObject(labelOperationInfo)
    , m_pLabelOperationInfo(labelOperationInfo)
    , m_pQTextEdit(textEdit)
    , m_pSpinBox_bytesCountToDelete(spinBox_bytesCountToDelete)
    , m_buttonGroup_EditInputType(buttonGroup_EditInputType)
    , m_buttonGroup_EditOverwrite(buttonGroup_EditOverwrite)
{

    connect(buttonGroup_EditInputType,  SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(validateInput()));
    connect(buttonGroup_EditOverwrite,  SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(validateInput()));
    connect(textEdit,                   SIGNAL(textChanged()),                    this, SLOT(validateInput()));
    connect(spinBox_bytesCountToDelete, SIGNAL(valueChanged(int)),                this, SLOT(validateInput()));
    //connect(textEdit,                   SIGNAL(cursorPositionChanged()),          this, SLOT(validateInput()));

    validateInput();
}

void CEditView::setUpdateCallback(std::function<void (void)> callbackUpdate)
{
    m_callbackUpdate = callbackUpdate;
}

void CEditView::setGetIndexCallback(std::function<int64_t (void)> callbackGetIndex)
{
    m_callbackGetIndex = callbackGetIndex;
}

void CEditView::Clear()
{
    m_pQTextEdit->clear();
    m_event.clear();
    m_pLabelOperationInfo->clear();
}

bool CEditView::Apply(QFile* pFileA, QFile* pFileB, DialogSaveToFile *infoDialog)
{
    auto errPos = validateInput();
    if(errPos != -1)
    {
        auto tmpCursor = m_pQTextEdit->textCursor();
        tmpCursor.setPosition(errPos);
        m_pQTextEdit->setTextCursor(tmpCursor);
        infoDialog->setError(QObject::tr("Unable convert to hexadecimal"));
        m_pQTextEdit->setFocus();
        return false;
    }

    if(!m_event.valid || m_event.pos < 0 || !pFileA->isOpen())
    {
        infoDialog->setError(QObject::tr("Nothing to do"));
        return false;
    }

    if((m_event.event == CEditEventOverwrite || m_event.event == CEditEventInsert) && m_event.data.size() < 1)
    {
        infoDialog->setError(QObject::tr("Nothing to do"));
        return false;
    }

    if(m_event.event == CEditEventDelete && m_event.deleteSize < 1)
    {
        infoDialog->setError(QObject::tr("Nothing to do"));
        return false;
    }

    auto sourceSize = pFileA->size();
    if(m_event.pos > sourceSize)
    {
        infoDialog->setError(QObject::tr("Error: seek position exceeds file size"));
        return false;
    }

    if(m_event.event == CEditEventDelete && (m_event.pos + m_event.deleteSize) > sourceSize)
    {
        infoDialog->setError(QObject::tr("Error delete: request out of range"));
        return false;
    }

    if(m_event.event == CEditEventOverwrite)
    {

        auto endRange = (m_event.pos + static_cast<int64_t>(m_event.data.size()));
        if(endRange > sourceSize)
        {
            infoDialog->setError(QObject::tr("Error: data size out of range"));
            return false;
        }

        QFile srcFile(pFileA->fileName());

        if(!srcFile.open(QIODevice::ReadWrite))
        {
            infoDialog->setError(QObject::tr("Failed open file\n") + srcFile.errorString());
            return false;
        }

        if(!srcFile.seek(m_event.pos))
        {
            infoDialog->setError(QObject::tr("Failed on seek position in file"));
            return false;
        }

        auto res = srcFile.write(reinterpret_cast<const char *>(m_event.data.data()), static_cast<qint64>(m_event.data.size()));
        if(res == -1)
        {
            infoDialog->setError(srcFile.errorString());
            return false;
        }

        if(res != static_cast<qint64>(m_event.data.size()))
        {
            infoDialog->setError(QObject::tr("Failed write to file"));
            return false;
        }

        m_event.valid = false;

        infoDialog->setError(QObject::tr("Successfully saved"));

        return true;
    }

    if(m_event.event == CEditEventInsert || m_event.event == CEditEventDelete)
    {

        QDir dir;

        auto srcPathName = pFileA->fileName();

        QString tmpPathName;
        QString bacPathName;

        auto incr = 0;
        do{
            tmpPathName = srcPathName + "." + QString::number(incr++) + ".tmp";
        }while(dir.exists(tmpPathName));

        if(m_event.event == CEditEventInsert)
            m_event.deleteSize = -1;

        if(infoDialog->EditBytes(srcPathName, tmpPathName, m_event.pos, m_event.data, m_event.deleteSize))
        {
            pFileA->close();
            pFileB->close();

            if(CConfigDialog::LoadChklBox(nullptr, CONFIG_BACKUPONSAVE, CONFIG_BACKUPONSAVE_DEF))
            {
                incr = 0;
                do{
                    bacPathName = srcPathName + "." + QString::number(incr++) + ".old";
                }while(dir.exists(bacPathName));

                QFile::rename(srcPathName, bacPathName);

            }
            else
            {
                QFile::remove(srcPathName);
            }

            QFile::rename(tmpPathName, srcPathName);

            pFileA->open(QIODevice::ReadOnly);
            pFileB->open(QIODevice::ReadOnly);

            m_event.valid = false;

            return true;
        }
        else
        {
            if(dir.exists(tmpPathName))
                dir.remove(tmpPathName);

            return false;
        }
    }


    return false;
}

int CEditView::HighlightError(const QString &src)
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    auto errorIndex = 0;
    auto firstErrorPosition = -1;
    for (auto ind = 0; ind < src.length(); ind++)
    {
        if(src[ind].isSpace() || isxdigit(src[ind].toLatin1()))
            continue;

        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(m_pQTextEdit->palette().background());
        selection.format.setProperty(QTextFormat::FullWidthSelection, false);
        selection.cursor = m_pQTextEdit->textCursor();
        selection.cursor.clearSelection();
        selection.cursor.setPosition(ind);
        selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);

        extraSelections.append(selection);

        if(firstErrorPosition < 0)
            firstErrorPosition = ind;

        errorIndex++;
        if(errorIndex > 100)
            break;
    }

    m_pQTextEdit->setExtraSelections(extraSelections);

    return firstErrorPosition;
}

void CEditView::ClearHighlightChar()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_pQTextEdit->setExtraSelections(extraSelections);
}


uint32_t HexChartoInt(uint32_t x)
{
    uint32_t const xval = x;
    if(xval < 65)
        return xval - 48;
    if(xval < 97)
        return xval - (65 - 10);
    return xval - (97 - 10);
}


void CEditView::ConvertHexText(QString &src, std::vector<uint8_t> &data)
{

    data.clear();

    uint32_t tmp[2] = {0};
    auto tmpInd = 0;

    for (auto ind = 0; ind < src.length(); ind++)
    {
        if(src[ind].isSpace())
            continue;
        auto val = src[ind].toLatin1();
        if(!isxdigit(val))
            continue;

        tmp[tmpInd++] = static_cast<uint32_t>(val);
        if(tmpInd == 2)
        {
            data.push_back(static_cast<uint8_t>(HexChartoInt(tmp[0]) << 4 | HexChartoInt(tmp[1])));
            tmpInd = 0;
        }
    }

}

int CEditView::validateInput()
{

    auto isText      = m_buttonGroup_EditInputType->checkedButton()->objectName().compare("radioButton_TEXT") == 0;
    auto isOverwrite = m_buttonGroup_EditOverwrite->checkedButton()->objectName().compare("radioButton_overwrite") == 0;
    auto isDelete    = m_buttonGroup_EditOverwrite->checkedButton()->objectName().compare("radioButton_delete") == 0;

    auto  firstErrorPosition = -1;

    m_event.event =  isOverwrite ? CEditEventOverwrite : isDelete ? CEditEventDelete : CEditEventInsert;

    m_pQTextEdit->setEnabled(!isDelete);
    m_pSpinBox_bytesCountToDelete->setEnabled(isDelete);

    if(m_callbackGetIndex == nullptr)
        return -1;

    auto pos = m_callbackGetIndex();
    if(pos < 0)
        return -1;

    m_event.pos = pos;

    if(isDelete)
    {
        m_event.deleteSize = m_pSpinBox_bytesCountToDelete->value();
        //m_event.data.clear();
    }
    else
    {
        m_event.deleteSize = -1;
        if(isText)
        {
            ClearHighlightChar();
            auto src = m_pQTextEdit->toPlainText();
            const QByteArray &val = src.toLatin1();
            m_event.data.assign(val.begin(),val.end());
        }
        else
        {
            auto src = m_pQTextEdit->toPlainText();
            firstErrorPosition = HighlightError(src);
            ConvertHexText(src, m_event.data);
        }
    }

    m_event.valid = true;

    m_pLabelOperationInfo->setText(m_event.GetInfo());

    m_callbackUpdate();

    return firstErrorPosition;
}

QString CEditView::GetEditStatus(int64_t row, int col, int cols_hex)
{
    if(!m_event.valid || m_event.event == CEditEventInsert || m_event.event == CEditEventDelete)
        return nullptr;

    auto pos = row * cols_hex;
    pos += col;

    if(col >= cols_hex)
        pos -= cols_hex;

    if(m_event.pos <= pos && pos < (m_event.pos + static_cast<int64_t>(m_event.data.size())))
    {
        if(col >= cols_hex)
        {
            col -= cols_hex;
            auto charx = m_event.data[pos - m_event.pos];
            if(charx <= 0x1f)
                charx = '.';
            return QChar(charx);
        }
        else
        {
            return QString("%1").arg(m_event.data[pos - m_event.pos], 2, 16, QLatin1Char('0')).toUpper();
        }
    }

    return nullptr;
}

CEditEvent CEditView::GetCellStatus(int64_t row, int col, int cols_hex)
{
    if(m_event.valid)
    {
        auto pos = row * cols_hex;
        pos += col;

        if(col >= cols_hex)
            pos -= cols_hex;

        if(m_event.event == CEditEventDelete)
        {
            if(m_event.pos <= pos && pos < (m_event.pos + static_cast<int64_t>(m_event.deleteSize)))
                return m_event.event;
        }
        else if(m_event.event == CEditEventOverwrite)
        {
            if(m_event.pos <= pos && pos < (m_event.pos + static_cast<int64_t>(m_event.data.size())))
                return m_event.event;
        }
        else if(m_event.event == CEditEventInsert)
        {
            if(m_event.pos == pos)
                return m_event.event;
        }

    }

    return CEditEvent::CEditEventNone;
}

