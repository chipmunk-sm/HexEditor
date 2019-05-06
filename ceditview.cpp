/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include "cconfigdialog.h"
#include "ceditview.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QRadioButton>
#include <cctype>

#include "defines.h"

CEditView::CEditView(QObject *parent)
    : QObject(parent)
{
}

void CEditView::SetOperation(int64_t pos, CEditEvent event, int byitesToDelete, const std::vector<uint8_t> &byteArray)
{
    m_event.event = event;
    if(event == CEditEvent::CEditEventDelete)
    {
        m_event.deleteSize = byitesToDelete;
        m_event.data.clear();
    }
    else
    {
        m_event.deleteSize = -1;
        m_event.data = byteArray;
    }
    m_event.pos = pos;
    m_event.valid = true;
}

void CEditView::Clear()
{
    m_event.clear();
}

bool CEditView::Apply(QFile* pFileA, QFile* pFileB, DialogSaveToFile *infoDialog)
{

    if(!m_event.valid || m_event.pos < 0 || !pFileA->isOpen())
    {
        infoDialog->setInfo(QObject::tr("Nothing to do"));
        return false;
    }

    if((m_event.event == CEditEventOverwrite || m_event.event == CEditEventInsert) && m_event.data.size() < 1)
    {
        infoDialog->setInfo(QObject::tr("Nothing to do"));
        return false;
    }

    if(m_event.event == CEditEventDelete && m_event.deleteSize < 1)
    {
        infoDialog->setInfo(QObject::tr("Nothing to do"));
        return false;
    }

    auto sourceSize = pFileA->size();
    if(m_event.pos > sourceSize)
    {
        infoDialog->setInfo(QObject::tr("Error: seek position exceeds file size"));
        return false;
    }

    if(m_event.event == CEditEventDelete && (m_event.pos + m_event.deleteSize) > sourceSize)
    {
        infoDialog->setInfo(QObject::tr("Error: request out of range"));
        return false;
    }

    if(m_event.event == CEditEventOverwrite)
    {

        auto endRange = (m_event.pos + static_cast<int64_t>(m_event.data.size()));
        if(endRange > sourceSize)
        {
            infoDialog->setInfo(QObject::tr("Error: data size out of range"));
            return false;
        }

		QDir dir;

		auto srcPathName = pFileA->fileName();
		if (CConfigDialog::LoadChklBox(nullptr, CONFIG_BACKUPONSAVE, CONFIG_BACKUPONSAVE_DEF))
		{
			auto incr = 0;
			QString bacPathName;

			do {
				bacPathName = pFileA->fileName() + "." + QString::number(incr++) + ".old";
			} while (dir.exists(bacPathName));

            QString sError;
            if (!infoDialog->CopyFile(srcPathName, bacPathName, sError))
			{
                infoDialog->setInfo(sError);
				return false;
			}
		}

        QFile srcFile(srcPathName);

        if(!srcFile.open(QIODevice::ReadWrite))
        {
            infoDialog->setInfo(QObject::tr("Failed open file\n") + srcFile.errorString());
            return false;
        }

        if(!srcFile.seek(m_event.pos))
        {
            infoDialog->setInfo(QObject::tr("Failed on seek position in file"));
            return false;
        }

        auto res = srcFile.write(reinterpret_cast<const char *>(m_event.data.data()), static_cast<qint64>(m_event.data.size()));
        if(res == -1)
        {
            infoDialog->setInfo(srcFile.errorString());
            return false;
        }

        if(res != static_cast<qint64>(m_event.data.size()))
        {
            infoDialog->setInfo(QObject::tr("Failed write to file"));
            return false;
        }

        m_event.valid = false;

        infoDialog->setInfo(QObject::tr("Successfully saved"));

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

        if(infoDialog->EditFile(srcPathName, tmpPathName, m_event.pos, m_event.data, m_event.deleteSize))
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
            auto charx = m_event.data[static_cast<uint64_t>(pos - m_event.pos)];
            if(charx <= 0x1F || charx >= 0x7F)
                charx = '.';
            return QChar(charx);
        }
        else
        {
            return QString("%1").arg(m_event.data[static_cast<uint64_t>(pos - m_event.pos)], 2, 16, QLatin1Char('0')).toUpper();
        }
    }

    return nullptr;
}

CEditEvent CEditView::GetCellStatus(int64_t pos)
{
    if(m_event.valid)
    {

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

