/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QException>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDebug>
#include <QFileInfo>

#include "cconfigdialog.h"
#include "chexviewselectionmodel.h"
#include "defines.h"
#include "dialogsavetofile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

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
                                    m_ui->lineEdit_goto);

    setWindowIcon(QPixmap(":/data/hexeditor_logo.png"));

    m_windowTitle = windowTitle();

    connect(this, &MainWindow::callCloseConfig,       this, &MainWindow::CloseConfig);
    connect(this, &MainWindow::callUpdateConfig,      this, &MainWindow::UpdateConfig);

}

MainWindow::~MainWindow()
{
    delete m_pchexview;
    delete m_pceditview;
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
        QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
        settings.setValue(DEFCFG_MAINWINDOWGEOM, saveGeometry());
        settings.setValue(DEFCFG_MAINWINDOWSPLITS, m_ui->splitter->saveState());
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
            restoreGeometry(settings.value(DEFCFG_MAINWINDOWGEOM,"").toByteArray());
            m_ui->splitter->restoreState(settings.value(DEFCFG_MAINWINDOWSPLITS,"").toByteArray());
            m_ui->propertyView->header()->restoreState(settings.value(DEFCFG_PROPRTYTABS,"").toByteArray());
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
                QMessageBox::critical(this, QObject::tr("Save as..."), QObject::tr("Access denied"), QMessageBox::Ok);
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




