/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CHEXVIEWCUSTOM_H
#define CHEXVIEWCUSTOM_H

#include <QTableView>

class CHexViewCustom : public QTableView
{
    Q_OBJECT

public:
    explicit CHexViewCustom(QWidget *parent = nullptr);

protected:
    virtual bool event(QEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    void drawCell(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index);
    bool UseScrollbar(int keyX);

};

#endif // CHEXVIEWCUSTOM_H
