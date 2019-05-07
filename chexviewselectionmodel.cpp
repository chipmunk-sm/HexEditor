/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#include "chexviewselectionmodel.h"
#include "chexviewmodel.h"

#if 0
#   include <QDebug>
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

CHexViewSelectionModel::CHexViewSelectionModel(QAbstractItemModel* model, QScrollBar* pSlider, QObject* parent)
    : QItemSelectionModel(model, parent)
    , m_pSlider(pSlider)
{
    DEBUGTRACE();
    m_selectFirst.column = 0;
    m_selectFirst.row = 0;
    m_selectSecond.column = 0;
    m_selectSecond.row = 0;
}

bool CHexViewSelectionModel::isSelectedEx(int64_t col, int64_t row) const
{
    if (!m_pSlider || m_offset < 0)
        return false;

    auto currentRow = row + m_pSlider->value();

    auto bCol = m_selectFirst.column < m_selectSecond.column ?
        m_selectFirst.column <= col && col <= m_selectSecond.column :
        m_selectFirst.column >= col && col >= m_selectSecond.column;

    auto bRow = m_selectFirst.row < m_selectSecond.row ?
        m_selectFirst.row <= currentRow && currentRow <= m_selectSecond.row :
        m_selectFirst.row >= currentRow && currentRow >= m_selectSecond.row;

    return bCol & bRow;
}

bool CHexViewSelectionModel::GetSelectedEx(CHexViewSelectionModelItem * pItemFirst, CHexViewSelectionModelItem * pItemSecond) const
{
    if (!m_pSlider || m_offset < 0)
        return false;

    if (pItemFirst)
        memcpy(pItemFirst, &m_selectFirst, sizeof(CHexViewSelectionModelItem));

    if (pItemSecond)
        memcpy(pItemSecond, &m_selectSecond, sizeof(CHexViewSelectionModelItem));

    return true;
}

void CHexViewSelectionModel::scrollSelection()
{
    auto ind = currentIndex();
    m_offset = m_pSlider->value();
    m_selectFirst.row = ind.row() + m_offset;
    m_selectSecond.row = ind.row() + m_offset;
}

void CHexViewSelectionModel::select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command)
{
    DEBUGTRACE();

    Q_UNUSED(selection);

    if (m_offset < 0)
        command = SelectionFlag::Clear;

    auto slider = m_pSlider->value();

    if ((command & SelectionFlag::Current) == SelectionFlag::Current)
    {
        m_selectSecond.column = currentIndex().column();
        m_selectSecond.row = static_cast<int64_t>(currentIndex().row()) + slider;
    }
    else if ((command & SelectionFlag::Clear) == SelectionFlag::Clear)
    {
        m_offset = slider;
        m_selectFirst.column = m_selectSecond.column = currentIndex().column();
        m_selectFirst.row = m_selectSecond.row = currentIndex().row() + m_offset;
    }
    else
    {
        m_offset = -1;
    }

    emit selectionChangedEx();

    qobject_cast<const CHexViewModel*>(model())->RepaintDisplay();
}

void CHexViewSelectionModel::clear()
{
    DEBUGTRACE();
    m_offset = -1;
    QItemSelectionModel::clear();
}

void CHexViewSelectionModel::reset()
{
    DEBUGTRACE();
    //QItemSelectionModel::reset();
}

void CHexViewSelectionModel::clearCurrentIndex()
{
    DEBUGTRACE();
    m_offset = -1;
    QItemSelectionModel::clearCurrentIndex();
}
