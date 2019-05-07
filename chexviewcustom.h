/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CHEXVIEWCUSTOM_H
#define CHEXVIEWCUSTOM_H

#include <QTableView>

class CHexViewCustom : public QTableView
{
    Q_OBJECT

public:
    explicit CHexViewCustom(QWidget* parent = nullptr);

protected:
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void drawCell(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index);
    bool UseScrollbar(int keyX);
signals:
    void selectionChangedEx();

};

#endif // CHEXVIEWCUSTOM_H
