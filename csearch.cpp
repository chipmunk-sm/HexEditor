/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "csearch.h"

#include <QFile>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QApplication>

CSearch::CSearch(QObject *parent)
    : QObject(parent)
{
}

void CSearch::SetControl(QStandardItemModel *resultControl, QProgressBar *progressBar)
{
    m_resultControl = resultControl;
    m_progressBar = progressBar;
}

void CSearch::Search(const char *searchBuffer, int32_t searchBufferLength, const QString &searchFile)
{
	Clear();

    if (!searchBufferLength)
    {
        QMessageBox::critical(nullptr, QObject::tr("Open"), tr("Nothing to search for"), QMessageBox::Ok);
        return;
    }

    if(m_mappedFile.OpenMapped(searchFile.toStdWString(), CMemoryMappedFile::CacheAccess_Sequential))
    {
        ParseFile(searchBuffer, searchBufferLength);
        return;
    }

    m_file.setFileName(searchFile);

    if (!m_file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, QObject::tr("Open"), m_file.errorString(), QMessageBox::Ok);
        return;
    }

    m_file.seek(0);

    ParseFile(searchBuffer, searchBufferLength);

}

void CSearch::ParseFile(const char *searchBuffer, int32_t searchBufferLength)
{

    auto sysPageSize = CMemoryMappedFile::GetSysPageSize();
    auto useMapped = !m_file.isOpen();

    if(searchBufferLength == 0 || searchBufferLength > sysPageSize)
    {
        if(useMapped)
        {
            m_mappedFile.Close();
        }
        else
        {
            m_file.close();
        }
        return;
    }

    std::vector<char> m_buffer;

    if(!useMapped)
        m_buffer.resize(static_cast<uint64_t>(sysPageSize));

    auto dataPtr = m_buffer.data();

    std::vector<FoundedResult> tmpResult;
    bool tmpPartialSearchExist = false;
    int64_t pos = 0;
    int64_t pageSize;
    int64_t fileSize;
    if(useMapped)
    {
        fileSize = m_mappedFile.GetFileSize();
    }
    else
    {
        fileSize = m_file.size();
    }

    while(!m_abort)
    {
        if(useMapped)
        {
            if(m_mappedFile.GetFileSize() < pos)
                break;

            dataPtr = m_mappedFile.GetDataPtr(pos, &pageSize);
            if(dataPtr == nullptr || pageSize < 1)
                break;
        }
        else
        {
            pageSize = m_file.read(reinterpret_cast<char*>(m_buffer.data()), static_cast<int64_t>(m_buffer.size()));
            if( pageSize < 1)
                break;
        }

        auto progress1 = 100.0 * static_cast<double>(pos) / static_cast<double>(fileSize);
        auto progress = static_cast<int>(progress1);
        QMetaObject::invokeMethod(m_progressBar, "setValue",
                                  Qt::QueuedConnection, Q_ARG(int, progress));

        qApp->processEvents();

        int64_t endPos = pageSize + pos;

        if(tmpPartialSearchExist)
        {
            for(auto &res : tmpResult)
            {

                int64_t searchIndex = 0;

                do{

                    if(searchIndex + res.len == searchBufferLength)
                    {
                        AddResult(res.pos, searchBufferLength);
                        pos += searchIndex - 1;
                        dataPtr += searchIndex - 1;
                        break;
                    }

                    if(dataPtr[searchIndex] != searchBuffer[searchIndex + res.len])
                        break;

                    searchIndex++;

                }while(true);

            }

            tmpResult.clear();
            tmpPartialSearchExist = false;
        }

        for(; pos < endPos; pos++)
        {
            if(*dataPtr++ != searchBuffer[0])
                continue;

            int64_t searchIndex = 1;

            do{

                if(searchIndex == searchBufferLength)
                {
                    AddResult(pos, searchBufferLength);
                    pos += searchIndex - 1;
                    dataPtr += searchIndex - 1;
                    break;
                }

                if(pos + searchIndex >= endPos)
                {
                    tmpResult.emplace_back(pos, searchIndex);
                    tmpPartialSearchExist = true;
                    break;
                }

                if(dataPtr[searchIndex - 1] != searchBuffer[searchIndex])
                    break;

                searchIndex++;

            }while(true);

        }
    }

    if(useMapped)
    {
        m_mappedFile.Close();
    }
    else
    {
        m_file.close();
    }

    QMetaObject::invokeMethod(m_progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 100));

}

void CSearch::AddResult(int64_t pos, int64_t len)
{
    m_result.emplace_back(pos, len);

    QList<QStandardItem*> listitems;
    listitems << new QStandardItem(tr("%1 (HEX %2)").arg(pos, 8, 10, QLatin1Char('0')).arg(pos, 8, 16, QLatin1Char('0')).toUpper());

//    QMetaObject::invokeMethod(m_resultControl, "appendRow", Qt::QueuedConnection,
//                              Q_ARG(const QList<QStandardItem*>&, listitems));

    m_resultControl->appendRow(listitems);

}

bool CSearch::GetCellStatus(int64_t pos)
{
    foreach(auto &item, m_result)
    {
        if(pos >= item.pos && pos < item.pos + item.len )
            return true;
    }
    return false;
}

void CSearch::Abort()
{
    m_abort = true;
}

void CSearch::Clear()
{
	m_progressBar->setValue(0);
	m_resultControl->removeRows(0, m_resultControl->rowCount());
	m_result.clear();
	m_abort = false;
}



