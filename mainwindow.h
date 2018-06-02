/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ccfontsize.h"
#include "chexviewmodel.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *m_ui;
    CHexViewModel  *m_pchexview = nullptr;
    CEditView      *m_pceditview = nullptr;

    CCFontSize     m_ccfontsize;
    QString        m_filename;
    QString        m_PathFilename;

    void UpdateConfig();
    void CloseConfig();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent *event) override;

    QString m_windowTitle;

private slots:
    void on_pushButtonOpen_clicked();
    void on_pushButton_config_clicked();
    void on_pushButton_SaveSelected_clicked();
    void on_pushButton_apply_clicked();

signals:
    void callUpdateConfig();
    void callCloseConfig();

};

#endif // MAINWINDOW_H
