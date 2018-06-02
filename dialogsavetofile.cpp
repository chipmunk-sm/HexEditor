/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "dialogsavetofile.h"
#include "ui_dialogsavetofile.h"

#include <QFile>
#include <QLabel>
#include <QMetaObject>
#include <QPaintDeviceWindow>
#include <QTextStream>
#include <thread>

DialogSaveToFile::DialogSaveToFile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSaveToFile)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->label_result->setVisible(false);
    ui->pushButton_exit->setVisible(false);

    ui->progressBar->setRange(0, m_progressMax);
    ui->progressBar->setValue(0);

}

DialogSaveToFile::~DialogSaveToFile()
{
    m_cancel = true;
    auto timeout = 100;
    while(!m_exit && timeout-- > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    delete ui;
}

void DialogSaveToFile::DumpSelectionAsText(const QString &inFile, const QString &outFile,
                                           int64_t top, int64_t left, int64_t bottom, int64_t right, int cols_hex)
{
    m_InFile = inFile;
    m_OutFile = outFile;
    m_top = top;
    m_left = left;
    m_bottom = bottom;
    m_right = right;
    m_cols_hex = cols_hex;

    auto OpenProcess = [](DialogSaveToFile* pThis)
    {
        try
        {
            pThis->RunDumpThread();

            pThis->ui->progressBar->hide();
            pThis->ui->pushButton_cancel->hide();
            pThis->ui->label_result->show();
            pThis->ui->pushButton_exit->show();

            if(pThis->m_error.isEmpty())
                pThis->ui->label_result->setText(tr("Successfully saved"));
            else
                pThis->ui->label_result->setText(pThis->m_error);
        }
        catch(std::exception  const&e)
        {
            pThis->m_error = QObject::tr(e.what());
        }
        catch(...)
        {
            pThis->m_error = QObject::tr("Unexpected exception");
        }
        pThis->m_exit = true;
    };
    (std::thread(OpenProcess, this)).detach();
}

bool DialogSaveToFile::EditBytes(const QString &inFile, const QString &outFile,
                                 int64_t pos, std::vector<uint8_t> &data, int64_t deleteSize)
{
    m_InFile = inFile;
    m_OutFile = outFile;
    m_pos = pos;
    m_data = data;
    m_deleteSize = deleteSize;

    auto result = EditBytes();

    ui->progressBar->hide();
    ui->pushButton_cancel->hide();
    ui->label_result->show();
    ui->pushButton_exit->show();

    if(m_cancel)
        ui->label_result->setText(tr("Cancelled by user"));
    else if(result)
        ui->label_result->setText(tr("Successfully saved"));
    else
        ui->label_result->setText(m_error);

    return result;
}

void DialogSaveToFile::setError(const QString &errString)
{
    ui->progressBar->hide();
    ui->pushButton_cancel->hide();
    ui->label_result->show();
    ui->pushButton_exit->show();
    ui->label_result->setText(errString);
}

void DialogSaveToFile::RunDumpThread()
{

    QFile outFile(m_OutFile);
    QFile inFile(m_InFile);

    std::vector<unsigned char> buffer;

    auto stride = m_right - m_left + 1;
    if(stride < 1)
        return;

    auto height = m_bottom - m_top + 1;
    if(height < 1)
        return;

    buffer.resize(static_cast<uint64_t>(stride));

    if (!outFile.open(QIODevice::ReadWrite|QFile::Truncate))
    {
        m_error = tr("Unable to open In file ") + m_OutFile + tr("\n") + outFile.errorString();
        return;
    }

    if (!inFile.open(QIODevice::ReadOnly))
    {
        m_error = tr("Unable to open Out file ") + m_InFile + tr("\n") + outFile.errorString();
        return;
    }

    m_progressInc = static_cast<double>(m_progressMax) / height;

    QTextStream stream(&outFile);

    for(int64_t rowIdx = 0; rowIdx < height; rowIdx++)
    {

        QMetaObject::invokeMethod(ui->progressBar, "setValue",
                                  Qt::QueuedConnection, Q_ARG(int, static_cast<int>(m_progressInc * (rowIdx + 1))));

        if(m_cancel)
        {
            m_error = tr("Cancelled by user");
            return;
        }

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
        stream << "\r\n";
#else
        stream << "\n";
#endif

        auto position = (m_top + rowIdx) * m_cols_hex;
        if(!inFile.seek(position))
        {
            m_error = tr("Unable to seek position ") + QString::number(position) + tr("\n") + m_InFile + tr("\n") + inFile.errorString();
            return;
        }

        auto len = inFile.read(reinterpret_cast<char*>(buffer.data()), stride );
        if(len < 0)
            continue;

        if(len < static_cast<int64_t>(buffer.size()))
        {
            memset(buffer.data() + len, 0, buffer.size() - static_cast<uint64_t>(len));
        }

        for(int64_t colIdx = 0; colIdx < stride; colIdx++)
        {
            if(colIdx >= m_cols_hex)
            {
                auto col = colIdx - m_cols_hex;
                auto charx = buffer[static_cast<uint64_t>(col)];
                if(charx <= 0x1f)
                    charx = '.';
                stream << QChar(charx);
            }
            else
            {
                stream << QString("%1").arg(buffer[static_cast<uint32_t>(colIdx)], 2, 16, QLatin1Char('0')).toUpper()
                        << " ";
            }
        }
    }

    inFile.close();
    outFile.close();

}

bool DialogSaveToFile::EditBytes()
{

    QFile outFile(m_OutFile);
    QFile inFile(m_InFile);

    if (!outFile.open(QIODevice::WriteOnly|QFile::Truncate))
    {
        m_error = tr("Unable to open source file ") + m_OutFile + tr("\n") + outFile.errorString();
        return false;
    }

    if (!inFile.open(QIODevice::ReadOnly))
    {
        m_error = tr("Unable to create temp file ") + m_InFile + tr("\n") + outFile.errorString();
        return false;
    }

    inFile.seek(0);
    outFile.seek(0);

    m_progressInc = static_cast<double>(m_progressMax) / (inFile.size() + static_cast<int64_t>(m_data.size()));

    if (!InsertBytes(&inFile, &outFile, m_pos))
        return false;

    if (m_cancel)
        return false;

    if (m_deleteSize < 0)
    {
        auto res = outFile.write(reinterpret_cast<const char *>(m_data.data()), static_cast<qint64>(m_data.size()));
        if(res != static_cast<qint64>(m_data.size()))
        {
            return false;
        }
    }
    else
    {
        m_pos += m_deleteSize;
        inFile.seek(inFile.pos() + m_deleteSize);
    }

    if (m_cancel)
        return false;

    if (!InsertBytes(&inFile, &outFile, inFile.size() - m_pos))
        return false;

    return true;

}

bool DialogSaveToFile::InsertBytes(QFile *pSrc, QFile *pDst, int64_t length)
{
    const auto largeBlockSize = 65535;
    const auto smallBlockSize = 4096;

    std::vector<char> tmpBuff;
    tmpBuff.resize(length > largeBlockSize ? largeBlockSize : smallBlockSize);

    int nprogress = 0;

    while(length > 0)
    {

        if (m_cancel)
            return false;

        auto toRead = std::min(static_cast<int64_t>(tmpBuff.size()), length);
        auto res = pSrc->read(tmpBuff.data(), toRead);
        if(res == -1)
        {
            return false;
        }

        if(pDst->write(tmpBuff.data(), res) == -1)
        {
            return false;
        }

        length -= res;

        if(!nprogress--)
        {
            nprogress = 3;
            ui->progressBar->setValue(static_cast<int>(m_progressInc * pDst->pos()));
            qApp->processEvents();
        }
    }

    return true;
}

void DialogSaveToFile::on_pushButton_cancel_clicked()
{
    m_cancel = true;
}

void DialogSaveToFile::on_pushButton_exit_clicked()
{
    close();
}
