/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QException>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDebug>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QTextCodec>

#include "cconfigdialog.h"
#include "chexviewselectionmodel.h"
#include "csearch.h"
#include "defines.h"
#include "dialogsavetofile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_pcsearch = new CSearch(this);

    m_pceditview = new CEditView(m_ui->label_operationInfo,
                                 m_ui->textEdit,
                                 m_ui->spinBox_bytesCountToDelete,
                                 m_ui->buttonGroup_EditInputType,
                                 m_ui->buttonGroup_EditOverwrite);

    m_pchexview = new CHexViewModel(m_ui->hexView,
                                    m_ui->propertyView,
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
    //horizontalLineWidget->setStyleSheet(QString("background-color: white;"));
    //horizontalLineWidget->setStyleSheet(QString("background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 cyan, stop:1 blue);"));
    horizontalLineWidget->setStyleSheet(QString("background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 Whitesmoke, stop:1 Darkslategray);"));

    m_ui->dockWidget_control->setTitleBarWidget(horizontalLineWidget);

    m_ui->treeView_searchResult->setModel(new QStandardItemModel(0, 1, this));
    m_ui->treeView_searchResult->model()->setHeaderData(0, Qt::Horizontal, tr("Position"));

    connect(m_ui->treeView_searchResult->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::searchSelectionModelChanged);

    m_pcsearch->SetControl(qobject_cast<QStandardItemModel*>(m_ui->treeView_searchResult->model()), m_ui->progressBar_search);
    m_ui->progressBar_search->setVisible(false);
    m_ui->pushButton_abortSearch->setVisible(false);

}

MainWindow::~MainWindow()
{
    delete m_pchexview;
    delete m_pceditview;
    delete m_pcsearch;
    delete m_ui;
}

void MainWindow::changeEvent(QEvent *e)
{
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
    try
    {
		m_pcsearch->Abort();

        QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
        settings.setValue(DEFCFG_MAINWINDOWGEOM, saveGeometry());
        settings.setValue(DEFCFG_MAINWINDOWSTATE, saveState());
        //settings.setValue(DEFCFG_MAINWINDOWSPLITS, m_ui->splitter->saveState());
        settings.setValue(DEFCFG_PROPRTYTABS, m_ui->propertyView->header()->saveState());
    }
    catch(...)
    {
        QMessageBox::critical(this, windowTitle(), "Failed on closeEvent", QMessageBox::Ok);
    }
}

void MainWindow::showEvent(QShowEvent *event)
{

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
            m_pchexview->OpenFile(m_PathFilename);

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
    m_ui->hexView->setShowGrid(            CConfigDialog::LoadChklBox(nullptr, CONFIG_SHOWGRID, CONFIG_SHOWGRID_DEF));
    m_ui->hexView->setAlternatingRowColors(CConfigDialog::LoadChklBox(nullptr, CONFIG_ROWCOLORS, CONFIG_ROWCOLORS_DEF));
    m_ccfontsize.LoadConfig();
    m_pchexview->UpdateColorConfig();
}

void MainWindow::CloseConfig()
{
    m_ui->pushButton_config->setEnabled(true);
}

void MainWindow::on_pushButton_SaveSelected_clicked()
{

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

    if(!m_pceditview->Apply(m_pchexview->GetFileHandler(), m_pchexview->GetPropertyFileHandler(), infoDialog))
    {
        //        QMessageBox::critical(this, QObject::tr("Apply changes"), errorInfo, QMessageBox::Ok);
    }

    m_pchexview->UpdateSelectionModelEx(true);
    m_pchexview->UpdateScrollbarProps();
    m_pchexview->layoutChanged();
    m_pchexview->UpdateTable(false);
}

void MainWindow::on_pushButton_search_clicked()
{
    m_ui->lineEdit_searchtext->setEnabled(false);
    m_ui->pushButton_search->setEnabled(false);
    m_ui->progressBar_search->setVisible(true);
    m_ui->pushButton_abortSearch->setVisible(true);
	m_ui->pushButtonOpen->setEnabled(false);
	m_ui->pushButton_apply->setEnabled(false);
    m_ui->radioButton_search_hex->setEnabled(false);
    m_ui->radioButton_search_text->setEnabled(false);

    auto searchText = m_ui->lineEdit_searchtext->text();

    on_lineEdit_searchtext_textChanged(searchText);

    auto bytesToSearch = ConvertHexTextToByteArray(m_ui->radioButton_search_hex->isChecked() ? searchText : searchText.toLatin1().toHex(' '));

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

    m_ui->radioButton_search_hex->setEnabled(true);
    m_ui->radioButton_search_text->setEnabled(true);
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
    m_pcsearch->Abort();
}

void MainWindow::searchSelectionModelChanged(const QItemSelection &selected, const QItemSelection &)
{

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
    uint32_t const xval = x;
    if(xval < 65)
        return xval - 48;
    if(xval < 97)
        return xval - (65 - 10);
    return xval - (97 - 10);
}

std::vector<uint8_t> MainWindow::ConvertHexTextToByteArray(const QString &src)
{

    std::vector<uint8_t> data;

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

    return data;
}

QString MainWindow::ConvertByteArrayToHexText(const std::vector<uint8_t> &byteArray)
{
    QString hex;
    foreach(auto &item, byteArray)
        hex += QString("%1 ").arg(item, 2, 16, QLatin1Char('0')).toUpper();
    return hex;
}

void MainWindow::on_lineEdit_searchtext_textChanged(const QString &arg1)
{
    auto searchText = m_ui->radioButton_search_hex->isChecked() ? arg1 : arg1.toLatin1().toHex(' ');
    m_ui->label_search_info->setText(ConvertByteArrayToHexText(ConvertHexTextToByteArray(searchText)));
    m_pcsearch->Clear();
}

void MainWindow::on_lineEdit_searchtext_textEdited(const QString &arg1)
{
    on_lineEdit_searchtext_textChanged(arg1);
}

void MainWindow::on_radioButton_search_text_toggled(bool)
{
    on_lineEdit_searchtext_textChanged(m_ui->lineEdit_searchtext->text());
}
