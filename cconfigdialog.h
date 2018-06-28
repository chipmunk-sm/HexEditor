/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CCONFIGDIALOG_H
#define CCONFIGDIALOG_H

#include "ccfontsize.h"

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QAbstractButton>
#include <functional>
#include <QGroupBox>
#include <QSettings>

namespace Ui {
    class CConfigDialog;
}

class CConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CConfigDialog(std::function<void(void)> callbackUpdate, std::function<void(void)> callbackClose, QWidget *parent = nullptr);
    ~CConfigDialog() override;

    void SaveChklBox(QCheckBox* chkBox, const char *configName, bool checked);
    static bool LoadChklBox(QCheckBox* chkBox, const char *configName, bool defChecked);

    void SaveColor(QLabel *plabel, const char *configName, const QColor &defColor);
    void LoadColor(QLabel *plabel, const char *configName, const QColor &defColor);

protected:
    void changeEvent(QEvent *e) override;
    virtual void showEvent(QShowEvent *event) override;

private slots:
    void on_pushButton_restore_default_clicked();
    void on_pushButton_close_clicked();

    void on_checkBox_showGrid_stateChanged(int arg1);
    void on_checkBox_alternatingRowColors_stateChanged(int arg1);
    void on_checkBox_BackupOnSave_stateChanged(int arg1);

    void on_pushButton_color_overwrite_clicked();
    void on_pushButton_color_insert_clicked();
    void on_pushButton_color_delete_clicked();
    void on_pushButton_color_search_clicked();

    void on_pushButton_releaseNote_clicked();

private:
    Ui::CConfigDialog *m_ui;
    CCFontSize      m_ccfontsize;

    void LoadConfig();
    void LoadConfigColor(const char *configName, QColor defClr, QLabel *ctrl);

    std::function<void(void)> m_callbackUpdate;
    std::function<void(void)> m_callbackClose;

    void ColorUpdateDialog(const char *configName);
};

#endif // CCONFIGDIALOG_H
