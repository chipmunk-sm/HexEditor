/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include "dialogsavetofile.h"
#include "ui_dialogsavetofile.h"

#include <QFile>
#include <QLabel>
#include <QMetaObject>
#include <QPaintDeviceWindow>
#include <QTextStream>
#include <thread>

#include "cmemorymappedfile.h"

//#define DEF_ENABLE_THREADING

DialogSaveToFile::DialogSaveToFile(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::DialogSaveToFile)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->label_result->hide();
    ui->pushButton_exit->hide();

    ui->progressBar->setRange(0, m_progressMax);
    ui->progressBar->setValue(0);

#ifdef DEF_ENABLE_THREADING
    m_callbackProgress = [&](int val)->void {
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, val));
        //qApp->processEvents();
    };
#else
    m_callbackProgress = [&](int val)->void {
        ui->progressBar->setValue(val);
        qApp->processEvents();
    };
#endif

}

DialogSaveToFile::~DialogSaveToFile()
{
    m_cancel = true;
    auto timeout = 100;
    while (!m_exit && timeout-- > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    delete ui;
}

void DialogSaveToFile::DumpSelectionAsText(const QString& inFile, const QString& outFile,
    int64_t top, int64_t left, int64_t bottom, int64_t right, int cols_hex)
{
    m_InFile = inFile;
    m_OutFile = outFile;
    m_top = top;
    m_left = left;
    m_bottom = bottom;
    m_right = right;
    m_cols_hex = cols_hex;
    m_exit = false;

#ifdef DEF_ENABLE_THREADING
    auto OpenProcess = [](DialogSaveToFile * pThis)
#else
    auto pThis = this;
#endif
    {
        auto result = false;
        try
        {
            result = pThis->RunDumpThread();
        }
        catch (std::exception  const& e)
        {
            pThis->m_error = QObject::tr(e.what());
        }
        catch (...)
        {
            pThis->m_error = QObject::tr("Unexpected exception");
        }

        pThis->setInfo(pThis->m_cancel ? tr("Canceled by user") : result ? tr("Successfully saved") : pThis->m_error);
        pThis->m_exit = true;
    };
#ifdef DEF_ENABLE_THREADING
    (std::thread(OpenProcess, this)).detach();
#endif

}

bool DialogSaveToFile::EditFile(const QString & inFile, const QString & outFile,
    int64_t editPos, std::vector<uint8_t> & data, int64_t deleteSize)
{
    m_InFile = inFile;
    m_OutFile = outFile;
    m_editPos = editPos;
    m_data = data;
    m_deleteSize = deleteSize;
    m_exit = false;

    auto result = false;
    try
    {
        result = EditFile();
    }
    catch (std::exception  const& e)
    {
        m_error = QObject::tr(e.what());
    }
    catch (...)
    {
        m_error = QObject::tr("Unexpected exception");
    }

    setInfo(m_cancel ? tr("Canceled by user") : result ? tr("Successfully saved") : m_error);
    m_exit = true;
    return result;
}

bool DialogSaveToFile::CopyFile(const QString & sInFile, const QString & sOutFile, QString & sError)
{

    QFile inFile(sInFile);
    QFile outFile(sOutFile);

    if (!inFile.open(QIODevice::ReadOnly))
    {
        sError = tr("Unable to open\n %1 \n %2").arg(inFile.fileName()).arg(inFile.errorString());
        return false;
    }

    if (!outFile.open(QIODevice::WriteOnly | QFile::Truncate))
    {
        sError = tr("Unable to open\n %1 \n %2").arg(outFile.fileName()).arg(outFile.errorString());
        return false;
    }

    inFile.seek(0);
    outFile.seek(0);

    m_progressInc = static_cast<double>(m_progressMax) / (inFile.size());

    if (!CopyBlock(&inFile, &outFile, inFile.size()))
    {
        sError = GetFileError(inFile, outFile);
        return false;
    }

    return true;

}

bool DialogSaveToFile::RunDumpThread()
{

    QFile inFile(m_InFile);
    QFile outFile(m_OutFile);

    std::vector<unsigned char> buffer;

    auto stride = m_right - m_left + 1;
    if (stride < 1)
        return false;

    auto height = m_bottom - m_top + 1;
    if (height < 1)
        return false;

    buffer.resize(static_cast<uint64_t>(m_cols_hex));

    if (!inFile.open(QIODevice::ReadOnly))
    {
        m_error = tr("Unable to open\n %1 \n %2").arg(m_InFile).arg(inFile.errorString());
        return false;
    }

    if (!outFile.open(QIODevice::ReadWrite | QFile::Truncate))
    {
        m_error = tr("Unable to open\n %1 \n %2").arg(m_OutFile).arg(outFile.errorString());
        return false;
    }

    m_progressInc = static_cast<double>(m_progressMax) / height;

    QTextStream stream(&outFile);

    for (int64_t rowIdx = 0; rowIdx < height; rowIdx++)
    {

        if (m_cancel)
            return false;

        m_callbackProgress(static_cast<int>(m_progressInc * (rowIdx + 1)));

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
        stream << "\r\n";
#else
        stream << "\n";
#endif

        auto position = (m_top + rowIdx) * m_cols_hex;
        if (!inFile.seek(position))
        {
            m_error = tr("Unable to seek position ") + QString::number(position) + tr("\n") + m_InFile + tr("\n") + inFile.errorString();
            return false;
        }

        auto len = inFile.read(reinterpret_cast<char*>(buffer.data()), static_cast<int64_t>(buffer.size()));
        if (len < 0)
            continue;

        if (len < static_cast<int64_t>(buffer.size()))
        {
            memset(buffer.data() + len, 0, buffer.size() - static_cast<uint64_t>(len));
        }

        for (int64_t colIdx = m_left; colIdx < stride + m_left; colIdx++)
        {
            if (colIdx >= m_cols_hex)
            {
                auto col = colIdx - m_cols_hex;
                auto charx = buffer[static_cast<uint64_t>(col)];
                if (charx <= 0x1F || charx >= 0x7F)
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
    return true;
}

bool DialogSaveToFile::EditFile()
{

    QFile inFile(m_InFile);
    QFile outFile(m_OutFile);

    if (!inFile.open(QIODevice::ReadOnly))
    {
        m_error = tr("Unable to open\n %1 \n %2").arg(m_InFile).arg(inFile.errorString());
        return false;
    }

    if (!outFile.open(QIODevice::WriteOnly | QFile::Truncate))
    {
        m_error = tr("Unable to open\n %1 \n %2").arg(m_OutFile).arg(outFile.errorString());
        return false;
    }

    inFile.seek(0);
    outFile.seek(0);

    m_progressInc = static_cast<double>(m_progressMax) / (inFile.size() + static_cast<int64_t>(m_data.size()));

    if (!CopyBlock(&inFile, &outFile, m_editPos))
    {
        m_error = GetFileError(inFile, outFile);
        return false;
    }

    if (m_cancel)
        return false;

    if (m_deleteSize < 0)
    {
        auto res = outFile.write(reinterpret_cast<const char*>(m_data.data()), static_cast<qint64>(m_data.size()));
        if (res != static_cast<qint64>(m_data.size()))
        {
            m_error = GetFileError(inFile, outFile);
            return false;
        }
    }
    else
    {
        m_editPos += m_deleteSize;
        if (!inFile.seek(inFile.pos() + m_deleteSize))
        {
            m_error = GetFileError(inFile, outFile);
            return false;
        }
    }

    if (m_cancel)
        return false;

    if (!CopyBlock(&inFile, &outFile, inFile.size() - m_editPos))
    {
        m_error = GetFileError(inFile, outFile);
        return false;
    }

    return true;

}

bool DialogSaveToFile::CopyBlock(QFile * pSrc, QFile * pDst, int64_t length)
{
    const auto largeBlockSize = 65535;
    const auto smallBlockSize = 4096;

    std::vector<char> tmpBuff;
    tmpBuff.resize(length > largeBlockSize ? largeBlockSize : smallBlockSize);

    auto nprogress = 0;
    const auto nprogressStep = 5;

    while (length > 0)
    {

        if (m_cancel)
            return false;

        auto toRead = std::min(static_cast<int64_t>(tmpBuff.size()), length);
        auto res = pSrc->read(tmpBuff.data(), toRead);
        if (res == -1)
        {
            return false;
        }

        if (pDst->write(tmpBuff.data(), res) == -1)
        {
            return false;
        }

        length -= res;

        if (!nprogress--)
        {
            nprogress = nprogressStep;
            m_callbackProgress(static_cast<int>(m_progressInc * pDst->pos()));
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

void DialogSaveToFile::setInfo(const QString & infoString)
{
#ifdef DEF_ENABLE_THREADING
    QMetaObject::invokeMethod(ui->progressBar, "hide", Qt::QueuedConnection);
    QMetaObject::invokeMethod(ui->pushButton_cancel, "hide", Qt::QueuedConnection);
    QMetaObject::invokeMethod(ui->label_result, "show", Qt::QueuedConnection);
    QMetaObject::invokeMethod(ui->pushButton_exit, "show", Qt::QueuedConnection);
    QMetaObject::invokeMethod(ui->label_result, "setText", Qt::QueuedConnection, Q_ARG(const QString&, infoString));
#else
    ui->progressBar->hide();
    ui->pushButton_cancel->hide();
    ui->label_result->show();
    ui->pushButton_exit->show();
    ui->label_result->setText(infoString);
#endif
}

QString DialogSaveToFile::GetFileError(const QFile & inFile, const QFile & outFile)
{
    return tr("Error: failed copy from \n %1 \n to \n %2 \n In[%3] \n Out[%4]")
        .arg(inFile.fileName()).arg(outFile.fileName()).arg(inFile.errorString()).arg(outFile.errorString());
}
