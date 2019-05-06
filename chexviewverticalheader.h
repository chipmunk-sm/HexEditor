/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CHEXVIEWVERTICALHEADER_H
#define CHEXVIEWVERTICALHEADER_H

#include <QHeaderView>

class CHexViewVerticalHeader : public QHeaderView
{
    Q_OBJECT
public:
    explicit CHexViewVerticalHeader(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
};

#endif // CHEXVIEWVERTICALHEADER_H
