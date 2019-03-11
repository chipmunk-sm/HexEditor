/* Copyright (C) 2018 chipmunk-sm <dannico@linuxmail.org> */

#include "chexviewcustom.h"
#include "chexviewmodel.h"
#include "chexviewselectionmodel.h"

#include <QResizeEvent>
#include <QHeaderView>
#include <QBitArray>


CHexViewCustom::CHexViewCustom(QWidget *parent)
    : QTableView(parent)
{
}

bool CHexViewCustom::event(QEvent *event)
{
    if(event->type() == QEvent::Wheel)
    {
        return static_cast<const CHexViewModel*>(model())->EventHandler(event);
    }
    else if(event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
    {
        QKeyEvent* evt = static_cast<QKeyEvent*>(event);
        switch (evt->key())
        {
            case Qt::Key_Down:
            case Qt::Key_Up:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            {
                if(UseScrollbar(evt->key()))
                {
                    static_cast<const CHexViewModel*>(model())->EventHandler(event);
                    QTableView::event(event);
                    static_cast<CHexViewSelectionModel*>(selectionModel())->scrollSelection();
                    emit selectionChangedEx();
                    //static_cast<const CHexViewModel*>(model())->RepaintDisplay();
                    return true;
                }
            }
            break;
            default:
            break;
        }
    }
    return QTableView::event(event);
}

void CHexViewCustom::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);

    if(model()->rowCount(QModelIndex()) != verticalHeader()->count())
    {
        static_cast<const CHexViewModel*>(model())->UpdateScrollbarProps();
        verticalHeader()->reset();
    }
}

void CHexViewCustom::paintEvent(QPaintEvent *event)
{
    const auto verticalHeader = QTableView::verticalHeader();
    const auto horizontalHeader = QTableView::horizontalHeader();

    if (horizontalHeader->count() == 0 || verticalHeader->count() == 0)
        return;

    auto option = viewOptions();
    const auto offset = dirtyRegionOffset();
    const auto showGrid = QTableView::showGrid();
    const auto gridSize = showGrid ? 1 : 0;
    const auto gridHint = style()->styleHint(QStyle::SH_Table_GridLineColor, &option, this);
    const auto gridColor = static_cast<QColor>(static_cast<QRgb>(gridHint));
    const auto gridPen = QPen(gridColor, 0, gridStyle());
    const auto alternate = alternatingRowColors();

    QPainter painter(viewport());

    auto x = horizontalHeader->length() - horizontalHeader->offset();
    auto y = verticalHeader->length() - verticalHeader->offset() - 1;

    auto firstVisualRow = qMax(verticalHeader->visualIndexAt(0),0);
    auto lastVisualRow = verticalHeader->visualIndexAt(verticalHeader->viewport()->height());

    if (lastVisualRow == -1)
        lastVisualRow = model()->rowCount() - 1;

    auto firstVisualColumn = horizontalHeader->visualIndexAt(0);
    auto lastVisualColumn = horizontalHeader->visualIndexAt(horizontalHeader->viewport()->width());

    if (firstVisualColumn == -1)
        firstVisualColumn = 0;

    if (lastVisualColumn == -1)
        lastVisualColumn = horizontalHeader->count() - 1;

    QBitArray drawn((lastVisualRow - firstVisualRow + 1) * (lastVisualColumn - firstVisualColumn + 1));

    const QRegion region = event->region().translated(offset);

    foreach (QRect dirtyArea , region)
    {

        dirtyArea.setBottom(qMin(dirtyArea.bottom(), int(y)));
        dirtyArea.setRight(qMin(dirtyArea.right(), int(x)));

        auto left = horizontalHeader->visualIndexAt(dirtyArea.left());
        auto right = horizontalHeader->visualIndexAt(dirtyArea.right());

        if (left == -1)
            left = 0;

        if (right == -1)
            right = horizontalHeader->count() - 1;

        auto bottom = verticalHeader->visualIndexAt(dirtyArea.bottom());

        if (bottom == -1)
            bottom = verticalHeader->count() - 1;

        auto top = verticalHeader->visualIndexAt(dirtyArea.top());
        auto alternateBase = (top & 1) && alternate;

        if (top == -1 || top > bottom)
            continue;

        // row
        for (auto visualRowIndex = top; visualRowIndex <= bottom; ++visualRowIndex)
        {

            auto row = verticalHeader->logicalIndex(visualRowIndex);

            if (verticalHeader->isSectionHidden(row))
                continue;

            auto rowY = rowViewportPosition(row) + offset.y();
            auto rowh = rowHeight(row) - gridSize;

            // column
            for (int visualColumnIndex = left; visualColumnIndex <= right; ++visualColumnIndex)
            {
                auto currentBit =
                        (visualRowIndex - firstVisualRow) * (lastVisualColumn - firstVisualColumn + 1)
                        + visualColumnIndex - firstVisualColumn;

                if (currentBit < 0 || currentBit >= drawn.size() || drawn.testBit(currentBit))
                    continue;

                drawn.setBit(currentBit);

                auto col = horizontalHeader->logicalIndex(visualColumnIndex);

                if (horizontalHeader->isSectionHidden(col))
                    continue;

                auto colp = columnViewportPosition(col) + offset.x();
                auto colw = columnWidth(col) - gridSize;

                const QModelIndex index = model()->index(row, col);
                if (index.isValid())
                {
                    option.rect = QRect(colp, rowY, colw, rowh);
                    if (alternate)
                    {
                        if (alternateBase)
                            option.features |= QStyleOptionViewItem::Alternate;
                        else
                            option.features &= ~QStyleOptionViewItem::Alternate;
                    }
                    drawCell(&painter, option, index);
                }
            }
            alternateBase = !alternateBase && alternate;
        }

        if (showGrid)
        {

            while (verticalHeader->isSectionHidden(verticalHeader->logicalIndex(bottom)))
            {
                --bottom;
            }

            auto savedPen = painter.pen();
            painter.setPen(gridPen);

            // row
            for (auto visualIndex = top; visualIndex <= bottom; ++visualIndex)
            {
                auto row = verticalHeader->logicalIndex(visualIndex);
                if (verticalHeader->isSectionHidden(row))
                    continue;

                auto rowY = rowViewportPosition(row) + offset.y();
                auto rowh = rowHeight(row) - gridSize;
                painter.drawLine(dirtyArea.left(), rowY + rowh, dirtyArea.right(), rowY + rowh);
            }

            // column
            for (auto h = left; h <= right; ++h)
            {
                auto col = horizontalHeader->logicalIndex(h);

                if (horizontalHeader->isSectionHidden(col))
                    continue;

                auto colp = columnViewportPosition(col) + offset.x();
                colp +=  columnWidth(col) - gridSize;
                painter.drawLine(colp, dirtyArea.top(), colp, dirtyArea.bottom());
            }

            painter.setPen(savedPen);
        }
    }
}

void CHexViewCustom::drawCell(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    auto proxy = qobject_cast<const CHexViewModel*>(index.model());
    if(!proxy)
        return;

    auto bSelectedCell = proxy->isSelectedEx(index.column(), index.row());
    auto bMirrorSelection = proxy->isSelectedEx(
                index.column() >= proxy->GetColHex() ? index.column() - proxy->GetColHex() : index.column() + proxy->GetColHex(),
                index.row());

    auto savedColor = painter->pen().color();
    auto penColor = option.palette.color(QPalette::Normal, bSelectedCell ? QPalette::HighlightedText : QPalette::Text);

    auto customColor = proxy->GetCellStatus(index);
    //auto customColor = proxy->data(index, Qt::TextColorRole);
    if(customColor.isValid())
    {
        //penColor = customColor.value<QColor>();
        penColor = customColor;
    }
    painter->setPen(penColor);

    if(bSelectedCell)
    {
        painter->fillRect(option.rect, option.palette.brush(QPalette::Normal, QPalette::Highlight));
    }
    else if((option.features & QStyleOptionViewItem::Alternate) == QStyleOptionViewItem::Alternate)
    {
        painter->fillRect(option.rect, option.palette.brush(QPalette::Normal, QPalette::AlternateBase));
    }

    if(bMirrorSelection)
    {
        auto rc = option.rect;
        painter->drawRoundedRect(rc.adjusted(0, 0, -1, -1), 2, 2);
    }

    option.widget->style()->drawItemText(painter,
                                         option.rect,
                                         Qt::AlignHCenter | Qt::AlignVCenter,
                                         option.palette,
                                         true,
                                         proxy->data(index, Qt::DisplayRole).toString());
    painter->setPen(savedColor);
}

bool CHexViewCustom::UseScrollbar(int keyX)
{
    auto bottom = model()->rowCount() - 1;
    if (bottom < 0)
        return false;

    auto current = currentIndex();
    if (!current.isValid())
        return false;

    auto currentRow = current.row();

    switch (keyX)
    {
        case Qt::Key_Down:
        case Qt::Key_PageDown:
        {
            if(currentRow == bottom)
                return true;
        }
        break;
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        {
            if(currentRow == 0)
                return true;
        }
        break;
        default:
        break;
    }
    return false;
}

