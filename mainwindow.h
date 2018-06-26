/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ccfontsize.h"
#include "chexviewmodel.h"
#include "csearch.h"

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
    CSearch        *m_pcsearch = nullptr;

    CCFontSize     m_ccfontsize;
    QString        m_filename;
    QString        m_PathFilename;

    void UpdateConfig();
    void CloseConfig();
    void searchSelectionModelChanged(const QItemSelection &selected, const QItemSelection &);
    uint32_t HexChartoInt(uint32_t x);
    std::vector<uint8_t> ConvertHexTextToByteArray(const QString &src);
    QString ConvertByteArrayToHexText(const std::vector<uint8_t> &byteArray);
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
    void on_pushButton_search_clicked();
    void on_pushButton_abortSearch_clicked();

    void on_lineEdit_searchtext_textChanged(const QString &arg1);

    void on_lineEdit_searchtext_textEdited(const QString &arg1);

    void on_radioButton_search_text_toggled(bool checked);

signals:
    void callUpdateConfig();
    void callCloseConfig();

};

#endif // MAINWINDOW_H
