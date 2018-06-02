/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef DIALOGSAVETOFILE_H
#define DIALOGSAVETOFILE_H

#include <QDialog>
#include <QFile>

namespace Ui {
    class DialogSaveToFile;
}

class DialogSaveToFile : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSaveToFile(QWidget *parent = nullptr);
    ~DialogSaveToFile();
    void DumpSelectionAsText(const QString &inFile, const QString &outFile,
                             int64_t top, int64_t left, int64_t bottom, int64_t right, int cols_hex);
    bool EditBytes(const QString &inFile, const QString &outFile,
                   int64_t pos, std::vector<uint8_t> &data, int64_t deleteSize);

    void setError(const QString &errString);
private slots:
    void on_pushButton_cancel_clicked();
    void on_pushButton_exit_clicked();

private:
    Ui::DialogSaveToFile *ui;
    bool m_cancel = false;
    bool m_exit = false;
    QString m_error;

    QString m_InFile;
    QString m_OutFile;

    int64_t m_pos = -1;
    double m_progressInc = 0.0;

    int64_t m_top = 0;
    int64_t m_left = 0;
    int64_t m_bottom = 0;
    int64_t m_right = 0;
    int m_cols_hex = 16;
    const int m_progressMax = 10000;

    std::vector<uint8_t> m_data;
    int64_t m_deleteSize = -1;

    void RunDumpThread();
    void CopyThread();
    bool EditBytes();
    bool InsertBytes(QFile *pSrc, QFile *pDst, int64_t length);

};

#endif // DIALOGSAVETOFILE_H
