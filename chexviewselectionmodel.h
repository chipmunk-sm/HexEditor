/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CHEXVIEWSELECTIONMODEL_H
#define CHEXVIEWSELECTIONMODEL_H

#include <QItemSelectionModel>
#include <QScrollBar>

struct CHexViewSelectionModelItem
{
    int64_t column = -1;
    int64_t row = -1;
};

class CHexViewSelectionModel : public QItemSelectionModel
{
    Q_OBJECT
public:
    explicit CHexViewSelectionModel(QAbstractItemModel *model, QScrollBar *pSlider, QObject *parent = nullptr);
    bool isSelectedEx(int64_t col, int64_t row) const;
    bool GetSelectedEx(CHexViewSelectionModelItem * pItemFirst, CHexViewSelectionModelItem * pItemSecond) const;
    void scrollSelection();

public slots:
    virtual void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;
    virtual void clear() override;
    virtual void reset() override;
    virtual void clearCurrentIndex() override;

signals:
    void selectionChangedEx();

private:
    QScrollBar* m_pSlider = nullptr;
    int64_t m_offset = -1;
    CHexViewSelectionModelItem m_selectFirst;
    CHexViewSelectionModelItem m_selectSecond;
};

#endif // CHEXVIEWSELECTIONMODEL_H
