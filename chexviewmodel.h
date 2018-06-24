/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CHEXVIEWMODEL_H
#define CHEXVIEWMODEL_H

#include "ceditview.h"
#include "cpropertyview.h"

#include <QAbstractTableModel>
#include <QFile>
#include <QLabel>
#include <QListView>
#include <QTableView>
#include <QTreeView>

const int COLS_HEX = 16;

class CHexViewModel : public QAbstractTableModel
{

    Q_OBJECT

public:
    explicit CHexViewModel(QTableView *pHexView,
                           QTreeView  *pPropertyView,
                           QScrollBar *pVerticalScrollBarHexView,
                           QLineEdit  *pLineEditInfo,
                           CEditView  *pEditView,
                           QLineEdit  *pLineEditGoTo);

    ~CHexViewModel() override;

    void UpdateTable(bool forceReformat);
    void OpenFile(const QString &path);
    QTableView *GetTableView() const;
    bool isSelectedEx(int64_t col, int64_t row) const;

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QColor GetCellStatus(const QModelIndex &index) const;

    QString dataEx(int64_t row, int64_t col) const;
    bool EventHandler(QEvent *event) const;
    void UpdateScrollbarProps() const;
    int64_t GetCurrentPos();

    void UpdateColorConfig();
    QFile *GetFileHandler();
    QFile *GetPropertyFileHandler();
    void UpdateSelectionModelEx(bool reset) const;
    int GetColHex() const {return m_cols_hex;}
    void SelectPosition(int64_t pos, int64_t len);
private:

    mutable QFile m_file;
    mutable std::vector<unsigned char> m_buffer;
    mutable int64_t m_cachePos;
    mutable int64_t m_cacheLen;

    QTableView                  *m_hexView                   = nullptr;
    CEditView                   *m_editview                  = nullptr;
    CPropertyView               *m_pCPropertyView            = nullptr;
    QScrollBar                  *m_pVerticalScrollBarHexView = nullptr;
    QLineEdit                   *m_lineEditInfo              = nullptr;
    QLineEdit                   *m_lineEditGoTo              = nullptr;

    QColor                      m_color_overwrite;
    QColor                      m_color_insert;
    QColor                      m_color_delete;
    QColor                      m_color_search;

    int m_cols_hex = COLS_HEX;

    int64_t GetRowCount() const;
    void SetInfo(int64_t val) const;

public slots:
    void UpdateSelectionModel() const;
    void UpdatePos(int pos);
    //void HistorySelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void lineEdit_goto_textEdited(const QString &arg1);
};

#endif // CHEXVIEWMODEL_H
