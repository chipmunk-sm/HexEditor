/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "chexviewmodel.h"
#include "chexviewverticalheader.h"

#include <QPainter>
#include <QResizeEvent>

CHexViewVerticalHeader::CHexViewVerticalHeader(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
}

void CHexViewVerticalHeader::paintEvent(QPaintEvent *event)
{
    if (count() == 0)
        return;

    auto proxy = qobject_cast<const CHexViewModel*>(model());
    if(!proxy)
        return;

    QPainter painter(viewport());
    const QPoint offset = dirtyRegionOffset();
    QRect translatedEventRect = event->rect();
    translatedEventRect.translate(offset);

    auto start = visualIndexAt(translatedEventRect.top());
    auto end = visualIndexAt(translatedEventRect.bottom());

    start = (start == -1 ? 0 : start);
    end = (end == -1 ? count() - 1 : end);

    auto tmp = start;
    start = qMin(start, end);
    end = qMax(tmp, end);

    auto width = viewport()->width();

    QStyleOptionHeader opt;
    initStyleOption(&opt);
    painter.save();

    for (auto index = start; index <= end; ++index)
    {
        if (isSectionHidden(index))
            continue;

        opt.section = logicalIndex(index);
        opt.rect.setRect(0, sectionViewportPosition(opt.section), width, sectionSize(opt.section));
        opt.rect.translate(offset);
        opt.state |= QStyle::State_Enabled |QStyle::State_Active;

        opt.textAlignment = defaultAlignment();
        opt.text = proxy->headerData(opt.section, orientation(), Qt::DisplayRole).toString();

        style()->drawControl(QStyle::CE_Header, &opt, &painter, this);
    }

    painter.restore();
}

