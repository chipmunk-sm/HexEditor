/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include "cconfigdialog.h"
#include "ui_cconfigdialog.h"
#include "defines.h"

#include <QColorDialog>
#include <QSettings>
#include <QMessageBox>
#include <QTextEdit>
#include <QFile>
#include <QDir>
#include <QTextStream>

#include "versionhelper.h"
CConfigDialog::CConfigDialog(std::function<void()> callbackUpdate, std::function<void()> callbackClose, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::CConfigDialog)
{

    m_ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    m_callbackUpdate = callbackUpdate;
    m_callbackClose = callbackClose;
    m_ccfontsize.SetUpdateCallback(callbackUpdate);

    m_lang.InitCombo(m_ui->comboBox_language, m_callbackUpdate);

    LoadConfig();

}

CConfigDialog::~CConfigDialog()
{
    m_callbackClose();
    delete m_ui;
}

void CConfigDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
    {
        m_ui->retranslateUi(this);
        LoadConfig();
    }
    break;
    default:
        break;
    }
}

void CConfigDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    if (!m_ccfontsize.Init(m_ui->horizontalSlider_Zoom, m_ui->fontComboBox, this))
        return;

}

void CConfigDialog::SaveChklBox(QCheckBox* chkBox, const char* configName, bool checked)
{
    {
        QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
        settings.setValue(configName, checked);
    }

    if (chkBox)
        chkBox->setCheckState(checked ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    m_callbackUpdate();
}

bool CConfigDialog::LoadChklBox(QCheckBox * chkBox, const char* configName, bool defChecked)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    auto res = settings.value(configName, defChecked).toBool();

    if (chkBox)
        chkBox->setCheckState(res ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    return res;
}

void CConfigDialog::SaveColor(QLabel*, const char* configName, const QColor & defColor)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue(configName, defColor);
}

void CConfigDialog::LoadColor(QLabel * plabel, const char* configName, const QColor & defColor)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QString clrStyle("QLabel { background-color:rgb(%1, %2, %3);}");
    auto bkClr = settings.value(configName, defColor).value<QColor>();
    plabel->setStyleSheet(clrStyle.arg(bkClr.red()).arg(bkClr.green()).arg(bkClr.blue()));
}

void CConfigDialog::LoadConfig()
{
    LoadChklBox(m_ui->checkBox_showGrid, CONFIG_SHOWGRID, CONFIG_SHOWGRID_DEF);
    LoadChklBox(m_ui->checkBox_alternatingRowColors, CONFIG_ROWCOLORS, CONFIG_ROWCOLORS_DEF);
    LoadChklBox(m_ui->checkBox_BackupOnSave, CONFIG_BACKUPONSAVE, CONFIG_BACKUPONSAVE_DEF);

    LoadColor(m_ui->label_color_overwrite, DEFCFG_CLR_OVERWRITE, DEFCFG_CLR_OVERWRITE_DEF);
    LoadColor(m_ui->label_color_insert, DEFCFG_CLR_INSERT, DEFCFG_CLR_INSERT_DEF);
    LoadColor(m_ui->label_color_delete, DEFCFG_CLR_DELETE, DEFCFG_CLR_DELETE_DEF);
    LoadColor(m_ui->label_color_search, DEFCFG_CLR_SEARCH, DEFCFG_CLR_SEARCH_DEF);

    m_ui->label_app_version->setText(tr("Application version: ") + QCoreApplication::applicationVersion() + "\n" +
        tr("Build Time: ") + BUILDDATETIME);

    m_ui->label_qt_version->setText(tr("Qt version: ") +
        " Runtime: " + qVersion() +
        " Build: " + QT_VERSION_STR);

}

void CConfigDialog::on_pushButton_restore_default_clicked()
{
    SaveChklBox(m_ui->checkBox_showGrid, CONFIG_SHOWGRID, CONFIG_SHOWGRID_DEF);
    SaveChklBox(m_ui->checkBox_alternatingRowColors, CONFIG_ROWCOLORS, CONFIG_ROWCOLORS_DEF);
    SaveChklBox(m_ui->checkBox_BackupOnSave, CONFIG_BACKUPONSAVE, CONFIG_BACKUPONSAVE_DEF);

    SaveColor(m_ui->label_color_overwrite, DEFCFG_CLR_OVERWRITE, DEFCFG_CLR_OVERWRITE_DEF);
    SaveColor(m_ui->label_color_insert, DEFCFG_CLR_INSERT, DEFCFG_CLR_INSERT_DEF);
    SaveColor(m_ui->label_color_delete, DEFCFG_CLR_DELETE, DEFCFG_CLR_DELETE_DEF);
    SaveColor(m_ui->label_color_search, DEFCFG_CLR_SEARCH, DEFCFG_CLR_SEARCH_DEF);

    m_ccfontsize.Reset();

    LoadConfig();
}

void CConfigDialog::on_pushButton_close_clicked()
{
    close();
}

void CConfigDialog::on_checkBox_showGrid_stateChanged(int arg1)
{
    SaveChklBox(nullptr, CONFIG_SHOWGRID, arg1);
}

void CConfigDialog::on_checkBox_alternatingRowColors_stateChanged(int arg1)
{
    SaveChklBox(nullptr, CONFIG_ROWCOLORS, arg1);
}

void CConfigDialog::on_checkBox_BackupOnSave_stateChanged(int arg1)
{
    SaveChklBox(nullptr, CONFIG_BACKUPONSAVE, arg1);
}

void CConfigDialog::ColorUpdateDialog(const char* configName)
{
    {
        QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
        auto color = QColorDialog::getColor(settings.value(configName).value<QColor>(), this);
        if (color.isValid())
            settings.setValue(configName, color);
    }
    LoadConfig();
    m_callbackUpdate();
}

void CConfigDialog::on_pushButton_color_overwrite_clicked()
{
    ColorUpdateDialog(DEFCFG_CLR_OVERWRITE);
}

void CConfigDialog::on_pushButton_color_insert_clicked()
{
    ColorUpdateDialog(DEFCFG_CLR_INSERT);
}

void CConfigDialog::on_pushButton_color_delete_clicked()
{
    ColorUpdateDialog(DEFCFG_CLR_DELETE);
}

void CConfigDialog::on_pushButton_color_search_clicked()
{
    ColorUpdateDialog(DEFCFG_CLR_SEARCH);
}

void CConfigDialog::on_pushButton_releaseNote_clicked()
{

    QString path = QDir(QDir(".").absolutePath()).filePath("releaseNote.txt");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "Error", path + "\n" + file.errorString());
        return;
    }

    auto infoDialog = new QDialog(this);
    infoDialog->setWindowTitle(tr("Release note"));
    infoDialog->setWindowFlags(infoDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto infoEditor = new QTextEdit;
    infoEditor->setLineWrapMode(QTextEdit::NoWrap);

    infoEditor->setPlainText(QTextStream(&file).readAll());
    infoEditor->setReadOnly(true);

    QFont newfont("monospace");
    newfont.setStyleHint(QFont::Monospace);
    newfont.setPointSize((this->font()).pointSize());
    infoEditor->setFont(newfont);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(infoEditor);
    infoDialog->setLayout(mainLayout);

    infoDialog->setAttribute(Qt::WA_DeleteOnClose);
    auto newsize = this->size();
    infoDialog->resize(newsize.width(), newsize.height());
    infoDialog->show();
    infoDialog->raise();
    infoDialog->activateWindow();

}

