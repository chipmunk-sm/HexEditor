/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CSEARCH_H
#define CSEARCH_H

#include "cmemorymappedfile.h"

#include <QFile>
#include <QObject>
#include <QProgressBar>
#include <QStandardItemModel>
#include <QTreeView>

typedef struct FoundedResult{
    FoundedResult(int64_t inPos, int64_t inLen)
    {
        pos = inPos;
        len = inLen;
    }
    int64_t pos;
    int64_t len;
}FoundedResult;

class CSearch : public QObject
{
    Q_OBJECT
public:
    explicit CSearch(QObject *parent);
    void SetControl(QStandardItemModel* resultControl, QProgressBar *progressBar);
    bool Search(const uint8_t *searchBuffer, int32_t searchBufferLength, const QString &searchFile);
    void Abort();
    bool GetCellStatus(int64_t pos);
	void Clear();
    uint64_t GetResultCount();
    QString GetErrorString();
private:
    bool ParseFile(const uint8_t *searchBuffer, int32_t searchBufferLength);
    void AddResult(int64_t pos, int64_t len);
    CMemoryMappedFile m_mappedFile;
    QFile m_file;
    QStandardItemModel* m_resultControl = nullptr;
    QProgressBar *m_progressBar = nullptr;
    bool m_abort = false;
    std::vector<FoundedResult> m_result;
    QString m_error;
};



#endif // CSEARCH_H
