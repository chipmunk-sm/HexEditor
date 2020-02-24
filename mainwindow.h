/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ccfontsize.h"
#include "chexviewmodel.h"
#include "clanguage.h"
#include "csearch.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    void RunFromCmd(QString& sourceFile, QString& hexString);

private:
    Ui::MainWindow* m_ui;
    CPropertyView* m_pcpropertyview = nullptr;
    CHexViewModel* m_pchexview = nullptr;
    CEditView* m_pceditview = nullptr;
    CSearch* m_pcsearch = nullptr;

    bool           m_searchInProgress = false;
    bool           m_editInactive = false;
    bool           m_disableGoToUpdate = false;

    CCFontSize     m_ccfontsize;
    QString        m_filename;
    QString        m_PathFilename;
    CLanguage      m_lang;

    void UpdateConfig();
    void CloseConfig();
    void searchSelectionModelChanged(const QItemSelection& selected, const QItemSelection&);
    uint32_t HexChartoInt(uint32_t x);
    std::vector<uint8_t> ConvertHexTextToByteArray(const QString& src, int* firstErrorPos);
    QString ConvertByteArrayToHexText(const std::vector<uint8_t>& byteArray);
    void CollectCodecs();
    bool DecodeText(const QString& sourceString, QLabel* info, bool bHex, int* firstErrorPos);
    void HighlightError(int firstErrorPos, QLineEdit* pEdit);
    void HighlightError(int firstErrorPos, QPlainTextEdit* pEdit);
    void LockInterfaceWhileSearch(bool lock);
    void OpenFile(const QString& filePath);
protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* e) override;
    void showEvent(QShowEvent* event) override;

    QString m_windowTitle;

private slots:
    void on_pushButtonOpen_clicked();
    void on_pushButton_config_clicked();
    void on_pushButton_SaveSelected_clicked();
    void on_pushButton_apply_clicked();
    void on_pushButton_search_clicked();
    void on_pushButton_abortSearch_clicked();
    void on_lineEdit_searchtext_textChanged(const QString& arg1);
    void on_lineEdit_searchtext_textEdited(const QString& arg1);
    void on_comboBox_textcodec_currentIndexChanged(int index);
    void on_checkBox_hexCoded_stateChanged(int arg1);
    void on_textDataEditor_textChanged();
    void on_spinBox_bytesCountToDelete_valueChanged(int arg1);
    void SelectionChange();
    void on_checkBox_displayDecodedText_stateChanged(int arg1);
    void on_pushButton_update_position_clicked();
    void on_verticalScrollBarHexView_valueChanged(int value);
    void on_lineEdit_goto_textChanged(const QString& arg1);
    void on_lineEdit_goto_textEdited(const QString& arg1);

    void on_pushButton_compareTo_clicked();

signals:
    void callUpdateConfig();
    void callCloseConfig();

};

#endif // MAINWINDOW_H
