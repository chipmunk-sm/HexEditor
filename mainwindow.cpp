/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QException>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QTextCodec>
#include <chrono>

#include "cconfigdialog.h"
#include "chexviewselectionmodel.h"
#include "csearch.h"
#include "defines.h"
#include "dialogsavetofile.h"

#if 0
#   include <QDebug>
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    DEBUGTRACE();
    m_ui->setupUi(this);

    m_pcpropertyview = new CPropertyView(m_ui->propertyView);
    m_pcsearch = new CSearch(this);
    m_pceditview = new CEditView(this);
    m_pchexview = new CHexViewModel(m_ui->hexView,
                                    m_ui->verticalScrollBarHexView,
                                    m_ui->label_info,
                                    m_pceditview,
                                    m_ui->lineEdit_goto,
                                    m_pcsearch);

    setWindowIcon(QPixmap(":/data/hexeditor_logo.png"));

    m_windowTitle = windowTitle();

    connect(this, &MainWindow::callCloseConfig,       this, &MainWindow::CloseConfig);
    connect(this, &MainWindow::callUpdateConfig,      this, &MainWindow::UpdateConfig);

    QWidget *horizontalLineWidget = new QWidget(this);
    horizontalLineWidget->setFixedHeight(1);
    horizontalLineWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    horizontalLineWidget->setStyleSheet(QString("background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 Whitesmoke, stop:1 Darkslategray);"));

    m_ui->dockWidget_control->setTitleBarWidget(horizontalLineWidget);

    m_ui->treeView_searchResult->setModel(new QStandardItemModel(0, 1, this));
    m_ui->treeView_searchResult->model()->setHeaderData(0, Qt::Horizontal, tr("Position"));

    connect(m_ui->treeView_searchResult->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::searchSelectionModelChanged);

    m_pcsearch->SetControl(qobject_cast<QStandardItemModel*>(m_ui->treeView_searchResult->model()), m_ui->progressBar_search);
    m_ui->progressBar_search->setVisible(false);
    m_ui->pushButton_abortSearch->setVisible(false);

    connect(m_ui->buttonGroup_EditOverwrite,  SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(on_textDataEditor_textChanged()));
    connect(static_cast<CHexViewSelectionModel*>(m_ui->hexView->selectionModel()), &CHexViewSelectionModel::selectionChangedEx,
            this, &MainWindow::SelectionChange);

    CollectCodecs();

//    auto buttonStyle = "QPushButton{border:none;}";
//    m_ui->pushButton_update_position->setStyleSheet(buttonStyle);
    m_ui->pushButton_update_position->setText("");

}

MainWindow::~MainWindow()
{
    DEBUGTRACE();
    delete m_pchexview;
    delete m_pceditview;
    delete m_pcsearch;
    delete m_pcpropertyview;
    delete m_ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    DEBUGTRACE();
    QMainWindow::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
            m_ui->retranslateUi(this);
        break;
        case QEvent::FontChange:
            m_pchexview->UpdateTable(false);
        break;
        default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *  /*event*/)
{
    DEBUGTRACE();
    try
    {
		m_pcsearch->Abort();

        QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
        settings.setValue(DEFCFG_MAINWINDOWGEOM, saveGeometry());
        settings.setValue(DEFCFG_MAINWINDOWSTATE, saveState());
        //settings.setValue(DEFCFG_MAINWINDOWSPLITS, m_ui->splitter->saveState());
        settings.setValue(DEFCFG_PROPRTYTABS, m_ui->propertyView->header()->saveState());
        settings.setValue(DEFCFG_DISPLAYTEXT, m_ui->checkBox_displayDecodedText->isChecked());
    }
    catch(...)
    {
        QMessageBox::critical(this, windowTitle(), "Failed on closeEvent", QMessageBox::Ok);
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    DEBUGTRACE();

    QMainWindow::showEvent( event );

    try
    {
        if(!m_ccfontsize.Init(m_ui->horizontalSlider_Zoom, nullptr, this))
            return;
        {
            QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
            m_ui->horizontalSlider_Zoom->setVisible(!settings.value(DEFCFG_CFG_HIDE_ZOOM, true).toBool());
            this->restoreGeometry(settings.value(DEFCFG_MAINWINDOWGEOM,"").toByteArray());
            this->restoreState(settings.value(DEFCFG_MAINWINDOWSTATE).toByteArray());
            //m_ui->splitter->restoreState(settings.value(DEFCFG_MAINWINDOWSPLITS,"").toByteArray());

            auto displayTextState = settings.value(DEFCFG_DISPLAYTEXT,true).toBool();
            m_ui->checkBox_displayDecodedText->setChecked(displayTextState);
            m_pcpropertyview->SetDisplayText(displayTextState);

            m_ui->propertyView->header()->restoreState(settings.value(DEFCFG_PROPRTYTABS,"").toByteArray());
            QList<QDockWidget*> docks = this->findChildren<QDockWidget*>();
            for(int i = 0; i < docks.size(); i++)
            {
                //docks.at(i)->raise();
                docks.at(i)->setFloating(false);
                docks.at(i)->show();
            }
        }
    }
    catch(...)
    {
        QMessageBox::critical(this, windowTitle(), "Failed on showEvent", QMessageBox::Ok);
    }

    m_pchexview->UpdateTable(false);
}

void MainWindow::on_pushButtonOpen_clicked()
{
    DEBUGTRACE();
    auto docsLocation = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    auto docPath = (docsLocation.isEmpty()) ? "~/" : QString("%1").arg(docsLocation.first());

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    docPath = settings.value(DEFCFG_CFG_OPENDIR, docPath).toString();

    QStringList filters;
    filters << "ALL (*)";

    QFileDialog dialog(this, QObject::tr("Open"), docPath);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilters(filters);

    if (dialog.exec() == QDialog::Accepted)
    {
        try
        {
            settings.setValue(DEFCFG_CFG_OPENDIR, dialog.directory().absolutePath());

            m_PathFilename = dialog.selectedFiles()[0];
            QFileInfo fileinfo(m_PathFilename);
            m_filename = fileinfo.baseName();
            setWindowTitle(m_windowTitle + " [" + m_PathFilename + "]") ;
            if(m_pchexview->OpenFile(m_PathFilename))
            {
                m_pcpropertyview->Close();
                m_pcpropertyview->OpenFile(m_PathFilename);
            }
            else
            {
                m_pcpropertyview->Close();
            }
        }
        catch(QException  const&e)
        {
            QMessageBox::critical(this, QObject::tr("Open"), QObject::tr(e.what()), QMessageBox::Ok);
        }
        catch(std::exception  const&e)
        {
            QMessageBox::critical(this, QObject::tr("Open"), QObject::tr(e.what()), QMessageBox::Ok);
        }
        catch(...)
        {
            QMessageBox::critical(this, QObject::tr("Open"), QObject::tr("Unexpected exception"), QMessageBox::Ok);
        }

    }
}

void MainWindow::on_pushButton_config_clicked()
{
    DEBUGTRACE();
    m_ui->pushButton_config->setEnabled(false);
    auto cfg = new CConfigDialog([&](void)->void{emit MainWindow::callUpdateConfig();}, [&](void)->void{emit MainWindow::callCloseConfig();});
    cfg->setWindowTitle(m_windowTitle + " "+ tr("Settings"));
    auto flags = cfg->windowFlags() & (~Qt::WindowContextHelpButtonHint);
    cfg->setWindowFlags(flags);
    cfg->setWindowIcon(windowIcon());
    cfg->show();
}

void MainWindow::UpdateConfig()
{
    DEBUGTRACE();
    m_ui->hexView->setShowGrid(            CConfigDialog::LoadChklBox(nullptr, CONFIG_SHOWGRID, CONFIG_SHOWGRID_DEF));
    m_ui->hexView->setAlternatingRowColors(CConfigDialog::LoadChklBox(nullptr, CONFIG_ROWCOLORS, CONFIG_ROWCOLORS_DEF));
    m_ccfontsize.LoadConfig();
    m_pchexview->UpdateColorConfig();
}

void MainWindow::CloseConfig()
{
    DEBUGTRACE();
    m_ui->pushButton_config->setEnabled(true);
}

void MainWindow::on_pushButton_SaveSelected_clicked()
{
    DEBUGTRACE();

    auto selsection = static_cast<CHexViewSelectionModel*>(m_ui->hexView->selectionModel());
    CHexViewSelectionModelItem itemFirst;
    CHexViewSelectionModelItem itemSecond;
    if(!selsection->GetSelectedEx(&itemFirst, &itemSecond))
        return;

    int64_t top = 0;
    int64_t left = 0;
    int64_t bottom = 0;
    int64_t right = 0;

    if(itemFirst.column < itemSecond.column)
    {
        left = itemFirst.column;
        right = itemSecond.column;
    }
    else
    {
        left = itemSecond.column;
        right = itemFirst.column;
    }

    if(itemFirst.row < itemSecond.row)
    {
        top = itemFirst.row;
        bottom = itemSecond.row;
    }
    else
    {
        top = itemSecond.row;
        bottom = itemFirst.row;
    }

    auto itemsCount = (bottom - top + 1) * (right - left + 1);
    if(itemsCount < 1)
        return;

    auto docsLocation = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    auto docPath = (docsLocation.isEmpty()) ? "~/" : QString("%1").arg(docsLocation.first());

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    docPath = settings.value(DEFCFG_CFG_SAVEDIR, docPath).toString();

    QStringList filters;
    filters << "ALL (*)";

    QFileDialog dialog(this, QObject::tr("Save as..."), docPath);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDefaultSuffix("txt");
    dialog.setNameFilters(filters);
    dialog.selectFile(m_filename);

    QString outFile;
    if (dialog.exec() == QDialog::Accepted)
    {
        try
        {
            settings.setValue(DEFCFG_CFG_SAVEDIR, dialog.directory().absolutePath());
            outFile = dialog.selectedFiles().first();
            if(outFile.compare(m_PathFilename,Qt::CaseInsensitive) == 0)
            {
                QMessageBox::critical(this, QObject::tr("Save as..."), QObject::tr("Access denied\n") + m_PathFilename, QMessageBox::Ok);
            }
            else
            {
                auto infoDialog = new DialogSaveToFile(this);
                infoDialog->setWindowTitle(tr("Save to ") + m_PathFilename);
                infoDialog->setFont(font());
                infoDialog->show();
                infoDialog->raise();
                infoDialog->activateWindow();
                infoDialog->DumpSelectionAsText(m_PathFilename, outFile, top, left, bottom, right, m_pchexview->GetColHex());
            }
        }
        catch(QException  const&e)
        {
            QMessageBox::critical(this, QObject::tr("Save as..."), QObject::tr(e.what()), QMessageBox::Ok);
        }
        catch(std::exception  const&e)
        {
            QMessageBox::critical(this, QObject::tr("Save as..."), QObject::tr(e.what()), QMessageBox::Ok);
        }
        catch(...)
        {
            QMessageBox::critical(this, QObject::tr("Save as..."), QObject::tr("Unexpected exception"), QMessageBox::Ok);
        }
    }
}


void MainWindow::on_pushButton_apply_clicked()
{
    DEBUGTRACE();

    if(!m_pchexview->GetFileHandler()->isOpen())
        return;

    auto reply = QMessageBox::question(this, QObject::tr("Apply changes"), QObject::tr("Apply changes?"),
                                       QMessageBox::Yes|QMessageBox::No);

    if (reply != QMessageBox::Yes)
    {
        return;
    }

    auto infoDialog = new DialogSaveToFile(this);
    infoDialog->setWindowTitle(tr("Save to ") + m_PathFilename);
    infoDialog->setFont(font());
    infoDialog->show();
    infoDialog->raise();
    infoDialog->activateWindow();

    if(!m_pceditview->Apply(m_pchexview->GetFileHandler(), m_pcpropertyview->GetFileHandler(), infoDialog))
    {
        //        QMessageBox::critical(this, QObject::tr("Apply changes"), errorInfo, QMessageBox::Ok);
    }
    else
    {
       m_pcsearch->Clear();
    }

    m_pchexview->Reset();
    m_pchexview->UpdateScrollbarProps();
    m_pchexview->layoutChanged();
    m_pchexview->UpdateTable(false);
}

void MainWindow::on_pushButton_search_clicked()
{
    DEBUGTRACE();
    on_lineEdit_searchtext_textChanged(m_ui->lineEdit_searchtext->text());

    auto firstErrorPos = -1;
    auto bytesToSearch = ConvertHexTextToByteArray(m_ui->label_search_info->text(), &firstErrorPos);
    if(bytesToSearch.size() < 1 || firstErrorPos != -1)
        return;

    m_ui->lineEdit_searchtext->setEnabled(false);
    m_ui->pushButton_search->setEnabled(false);
    m_ui->progressBar_search->setVisible(true);
    m_ui->pushButton_abortSearch->setVisible(true);
	m_ui->pushButtonOpen->setEnabled(false);
	m_ui->pushButton_apply->setEnabled(false);

    auto timeStart = std::chrono::high_resolution_clock::now();

    if(m_pcsearch->Search(bytesToSearch.data(), static_cast<int32_t>(bytesToSearch.size()), m_PathFilename))
    {
        auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - timeStart);
        QString info = tr("Found %1 in %2 seconds.").arg(m_pcsearch->GetResultCount()).arg(QString::number(time_span.count(), 'f', 3));
        m_ui->label_search_info->setText(info);
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Open"), m_pcsearch->GetErrorString(), QMessageBox::Ok);
    }

    m_ui->pushButton_apply->setEnabled(true);
	m_ui->pushButtonOpen->setEnabled(true);
    m_ui->lineEdit_searchtext->setEnabled(true);
    m_ui->pushButton_search->setEnabled(true);
    m_ui->progressBar_search->setVisible(false);
    m_ui->pushButton_abortSearch->setVisible(false);
	m_pchexview->layoutChanged();
	m_pchexview->UpdateTable(false);
}

void MainWindow::on_pushButton_abortSearch_clicked()
{
    DEBUGTRACE();
    m_pcsearch->Abort();
}

void MainWindow::searchSelectionModelChanged(const QItemSelection &selected, const QItemSelection &)
{
    DEBUGTRACE();
    QModelIndexList items = selected.indexes();
    foreach (auto &tmpindex, items)
    {
        auto val = tmpindex.data().toString();
        auto list = val.split(' ', QString::SplitBehavior::SkipEmptyParts);
        foreach (auto &tmpstr, list)
        {
            auto tmpv = tmpstr.toLongLong();
            m_pchexview->SelectPosition(tmpv);
            break;
        }
        break;
    }
}

uint32_t MainWindow::HexChartoInt(uint32_t x)
{
    //DEBUGTRACE();
    uint32_t const xval = x;
    if(xval < 65)
        return xval - 48;
    if(xval < 97)
        return xval - (65 - 10);
    return xval - (97 - 10);
}

std::vector<uint8_t> MainWindow::ConvertHexTextToByteArray(const QString &src, int *firstErrorPos)
{
    DEBUGTRACE();
    std::vector<uint8_t> data;

    uint32_t tmp[2] = {0};
    auto tmpInd = 0;

    for (auto ind = 0; ind < src.length(); ind++)
    {

        if(src[ind].isSpace())
            continue;

        auto val = src[ind].toLatin1();
        if(!isxdigit(val))
        {
            if(firstErrorPos != nullptr && *firstErrorPos == -1)
               *firstErrorPos = ind;
            continue;
        }

        tmp[tmpInd++] = static_cast<uint32_t>(val);
        if(tmpInd == 2)
        {
            data.push_back(static_cast<uint8_t>(HexChartoInt(tmp[0]) << 4 | HexChartoInt(tmp[1])));
            tmpInd = 0;
        }
    }

    return data;
}

QString MainWindow::ConvertByteArrayToHexText(const std::vector<uint8_t> &byteArray)
{
    DEBUGTRACE();
    QString hex;
    foreach(auto &item, byteArray)
        hex += QString("%1 ").arg(item, 2, 16, QLatin1Char('0')).toUpper();
    return hex;
}

void MainWindow::CollectCodecs()
{
    DEBUGTRACE();
    QMap<QString, QTextCodec *> codecMap;
    QRegularExpression iso8859RegExp("^ISO[- ]8859-([0-9]+).*$");
    QRegularExpressionMatch match;

    foreach (int mib, QTextCodec::availableMibs())
    {
        QTextCodec *codec = QTextCodec::codecForMib(mib);

        QString sortKey = codec->name().toUpper();
        int rank;

        if (sortKey.startsWith(QLatin1String("UTF-8")))
        {
            rank = 1;
        }
        else if (sortKey.startsWith(QLatin1String("UTF-16")))
        {
            rank = 2;
        }
        else if ((match = iso8859RegExp.match(sortKey)).hasMatch())
        {
            if (match.captured(1).size() == 1)
                rank = 3;
            else
                rank = 4;
        }
        else
        {
            rank = 5;
        }
        sortKey.prepend(QLatin1Char(static_cast<char>('0' + rank)));
        codecMap.insert(sortKey, codec);
    }

    foreach (const QTextCodec *codec, codecMap.values())
    {
        m_ui->comboBox_textcodec->addItem(QLatin1String(codec->name()), QVariant(codec->mibEnum()));
    }
}

bool MainWindow::DecodeText(const QString &sourceString, QLabel *info, bool bHex, int *firstErrorPos)
{
    DEBUGTRACE();
    if(bHex)
    {
        info->setText(ConvertByteArrayToHexText(ConvertHexTextToByteArray(sourceString, firstErrorPos)));
        info->setStyleSheet(QString());
        return true;
    }

    const auto mib = m_ui->comboBox_textcodec->itemData(m_ui->comboBox_textcodec->currentIndex()).toInt();
    const auto codec = QTextCodec::codecForMib(mib);
    const auto encoder = codec->makeEncoder( QTextCodec::IgnoreHeader );
    auto success = codec->canEncode(sourceString);
    auto decodedStr = encoder->fromUnicode(sourceString);
    info->setText(ConvertByteArrayToHexText(ConvertHexTextToByteArray(decodedStr.toHex(' '), firstErrorPos)));
    info->setStyleSheet(success ? QString() : QStringLiteral("background-color: \"red\";"));
    return success;
}

void MainWindow::HighlightError(int firstErrorPos, QLineEdit *pEdit)
{
    DEBUGTRACE();
    if(firstErrorPos == -1)
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event(QString(), attributes);
        QCoreApplication::sendEvent(pEdit, &event);
        return;
    }

    QList<QTextLayout::FormatRange> formats;

    QTextCharFormat format;
    format.setFontItalic(false);
    format.setFontWeight(QFont::Bold);
    format.setBackground(Qt::darkRed);
    format.setForeground(Qt::white);

    QTextLayout::FormatRange formatRange;
    formatRange.start = firstErrorPos;
    formatRange.length = 1;
    formatRange.format = format;
    formats.append(formatRange);

    QList<QInputMethodEvent::Attribute> attributes;
    foreach(const QTextLayout::FormatRange& fr, formats)
    {
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                       fr.start - pEdit->cursorPosition(),
                                                       fr.length,
                                                       fr.format));
    }

    QInputMethodEvent event(QString(), attributes);
    QCoreApplication::sendEvent(pEdit, &event);
}

void MainWindow::HighlightError(int firstErrorPos, QPlainTextEdit *pEdit)
{
    DEBUGTRACE();
    if(firstErrorPos == -1)
    {
        QList<QTextEdit::ExtraSelection> extraSelections;
        pEdit->setExtraSelections(extraSelections);
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(Qt::darkRed);
    selection.format.setForeground(Qt::white);
    selection.format.setProperty(QTextFormat::FullWidthSelection, false);
    selection.cursor = pEdit->textCursor();
    selection.cursor.clearSelection();
    selection.cursor.setPosition(firstErrorPos);
    selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.append(selection);
    pEdit->setExtraSelections(extraSelections);

}
void MainWindow::on_lineEdit_searchtext_textChanged(const QString &arg1)
{
    DEBUGTRACE();
    auto firstErrorPos = -1;
    auto res = DecodeText(arg1, m_ui->label_search_info, m_ui->checkBox_hexCoded->isChecked(), &firstErrorPos);

    HighlightError(firstErrorPos, m_ui->lineEdit_searchtext);

    if(firstErrorPos != -1)
        res = false;

    m_ui->pushButton_search->setEnabled(res);
    m_pcsearch->Clear();
}

void MainWindow::on_lineEdit_searchtext_textEdited(const QString &arg1)
{
    DEBUGTRACE();
    on_lineEdit_searchtext_textChanged(arg1);
}

void MainWindow::on_comboBox_textcodec_currentIndexChanged(int)
{
    DEBUGTRACE();
    on_lineEdit_searchtext_textChanged(m_ui->lineEdit_searchtext->text());
    on_textDataEditor_textChanged();
}

void MainWindow::on_checkBox_hexCoded_stateChanged(int)
{
    DEBUGTRACE();
    on_lineEdit_searchtext_textChanged(m_ui->lineEdit_searchtext->text());
    on_textDataEditor_textChanged();
}

void MainWindow::on_textDataEditor_textChanged()
{
    DEBUGTRACE();
    auto firstErrorPos = -1;
    auto res = DecodeText(m_ui->textDataEditor->toPlainText(), m_ui->label_edit_info, m_ui->checkBox_hexCoded->isChecked(), &firstErrorPos);
    HighlightError(firstErrorPos, m_ui->textDataEditor);

    if(firstErrorPos != -1)
        res = false;

    m_ui->pushButton_apply->setEnabled(res);

    auto isOverwrite = m_ui->radioButton_overwrite->isChecked();
    auto isDelete    = m_ui->radioButton_delete->isChecked();
    auto event = isOverwrite ? CEditEvent::CEditEventOverwrite :
                               isDelete ? CEditEvent::CEditEventDelete : CEditEvent::CEditEventInsert;

    m_ui->textDataEditor->setEnabled(!isDelete);
    m_ui->spinBox_bytesCountToDelete->setEnabled(isDelete);

    m_pceditview->SetOperation(m_pchexview->GetCurrentPos(), event, m_ui->spinBox_bytesCountToDelete->value(),
                               ConvertHexTextToByteArray(m_ui->label_edit_info->text(), nullptr));

    m_ui->label_operationInfo->setText(m_pceditview->GetInfo());
    m_pchexview->RepaintDisplay();
}

void MainWindow::on_spinBox_bytesCountToDelete_valueChanged(int)
{
    DEBUGTRACE();
    on_textDataEditor_textChanged();
}

void MainWindow::SelectionChange()
{
    DEBUGTRACE();
    auto pos = m_pchexview->GetCurrentPos();
    m_pcpropertyview->DecodeValue(pos);
    m_pchexview->SetInfo(pos);
}

void MainWindow::on_checkBox_displayDecodedText_stateChanged(int)
{
    DEBUGTRACE();
    m_pcpropertyview->SetDisplayText(m_ui->checkBox_displayDecodedText->isChecked());
}

void MainWindow::on_pushButton_update_position_clicked()
{
    on_textDataEditor_textChanged();
}
