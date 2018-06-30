/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#include "chexviewmodel.h"
#include "chexviewselectionmodel.h"
#include "chexviewverticalheader.h"
#include "defines.h"

#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QLineEdit>

#if 0
#   include <QDebug>
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

CHexViewModel::CHexViewModel(QTableView *pHexView,
                             QScrollBar *pVerticalScrollBarHexView,
                             QLineEdit  *pLineEditInfo,
                             CEditView  *pEditView,
                             QLineEdit  *pLineEditGoTo,
                             CSearch    *pcsearch)

    : QAbstractTableModel(pHexView)
    , m_cachePos(-1)
    , m_cacheLen(0)
    , m_hexView(pHexView)
    , m_editview(pEditView)
    , m_pVerticalScrollBarHexView(pVerticalScrollBarHexView)
    , m_lineEditInfo(pLineEditInfo)
    , m_lineEditGoTo(pLineEditGoTo)
    , m_pcsearch(pcsearch)
{
    DEBUGTRACE();

    m_buffer.resize(m_cols_hex);

    SetInfo(0);

    m_pVerticalScrollBarHexView->setMinimum(0);
    m_pVerticalScrollBarHexView->setMaximum(0);
    m_pVerticalScrollBarHexView->setSingleStep(1);
    m_pVerticalScrollBarHexView->setPageStep(100);

    m_hexView->setVerticalHeader(new CHexViewVerticalHeader(Qt::Orientation::Vertical, pHexView));
    m_hexView->setModel(this);
    m_hexView->setSelectionModel(new CHexViewSelectionModel(this, pVerticalScrollBarHexView, pHexView));

    auto displayFont = m_hexView->font();
    displayFont.setStyleHint(QFont::Monospace);
    m_hexView->setFont(displayFont);

    m_hexView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    auto headerVertical = m_hexView->verticalHeader();
    headerVertical->setSectionResizeMode(QHeaderView::Fixed);
    headerVertical->setDefaultAlignment(Qt::AlignCenter | Qt::AlignJustify);
    headerVertical->setHighlightSections(true);

    connect(m_pVerticalScrollBarHexView, &QScrollBar::valueChanged,              this, &CHexViewModel::RedrawDisplayArea);
    connect(m_lineEditGoTo,              &QLineEdit::textEdited,                 this, &CHexViewModel::lineEdit_goto_textEdited);

    m_lineEditGoTo->setValidator(new QRegExpValidator(QRegExp("^\\d{1,10}$"), this));

    UpdateColorConfig();

}

CHexViewModel::~CHexViewModel()
{
    DEBUGTRACE();
}

void CHexViewModel::UpdateColorConfig()
{
    DEBUGTRACE();
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    m_color_overwrite = settings.value(DEFCFG_CLR_OVERWRITE, DEFCFG_CLR_OVERWRITE_DEF).value<QColor>();
    m_color_insert    = settings.value(DEFCFG_CLR_INSERT,    DEFCFG_CLR_INSERT_DEF).value<QColor>();
    m_color_delete    = settings.value(DEFCFG_CLR_DELETE,    DEFCFG_CLR_DELETE_DEF).value<QColor>();
    m_color_search    = settings.value(DEFCFG_CLR_SEARCH,    DEFCFG_CLR_SEARCH_DEF).value<QColor>();
}

QFile *CHexViewModel::GetFileHandler()
{
    DEBUGTRACE();
    return &m_file;
}

void CHexViewModel::UpdateTable(bool forceReformat)
{
    DEBUGTRACE();

    auto displayFont = m_hexView->font();
    auto fontMetric = QFontMetrics(displayFont);
    auto wdth = fontMetric.width("DD");
    wdth = static_cast<int>(wdth + wdth * 0.2);
    auto height = static_cast<int>(fontMetric.height() + fontMetric.lineSpacing() * 0.3);

    int nCols = m_cols_hex * 2;
    if (nCols > 0 && (forceReformat || wdth != m_hexView->columnWidth(0)))
    {
        for (int col = 0; col < nCols/2; col++)
            m_hexView->setColumnWidth(col, wdth);

        wdth = fontMetric.width("00");
        for (int col = nCols/2; col < nCols; col++)
            m_hexView->setColumnWidth(col, wdth);
    }

    auto headerVertical = m_hexView->verticalHeader();
    if(headerVertical->defaultSectionSize() != height)
        headerVertical->setDefaultSectionSize(height);

}

bool CHexViewModel::OpenFile(const QString &path)
{
    DEBUGTRACE();
    m_cachePos = -1;
    m_cacheLen = 0;

    if( m_file.isOpen() )
        m_file.close();

    m_editview->Clear();
	m_pcsearch->Clear();

    m_cachePos = -1;
    m_cacheLen = 0;
    m_lineEditInfo->setText("");

    m_file.setFileName(path);

    if (!m_file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(m_hexView, QObject::tr("Open"), m_file.errorString(), QMessageBox::Ok);
        return false;
    }

    const int64_t treshold32gb = 34359738368 - 1000000;
    const int64_t treshold64gb = treshold32gb * 2; // 64gb - 2mb !!!

    const int64_t maxSupported = treshold64gb;

    if(m_file.size() > treshold64gb)
    {
        m_file.close();
        auto tmp = m_file.size();
        QMessageBox::critical(m_hexView, QObject::tr("Open"),
                              tr("File is larger than maximum file size supported.\n") +
                              tr("Support up to\t") + QString::number(maxSupported / 1024 / 1024) + tr(" MB\n") +
                              tr("File size\t\t")   + QString::number(tmp          / 1024 / 1024) + tr(" MB"),
                              QMessageBox::Ok);
        return false;
    }

    if(m_file.size() > treshold32gb)
        m_cols_hex = 32;
    else
        m_cols_hex = 16;


    m_buffer.resize(m_cols_hex);

    m_pVerticalScrollBarHexView->setValue(0);
    UpdateScrollbarProps();

    m_hexView->reset();

    auto newIndex = index(0,0);
    m_hexView->selectionModel()->select(newIndex, QItemSelectionModel::ClearAndSelect);
    m_hexView->setCurrentIndex(newIndex);
    m_hexView->setFocus();

    SetInfo(0);

    layoutChanged();
    UpdateTable(true);
    return true;
}

QTableView *CHexViewModel::GetTableView() const
{
    return m_hexView;
}

bool CHexViewModel::isSelectedEx(int64_t col, int64_t row) const
{
    auto selsection = static_cast<CHexViewSelectionModel*>(m_hexView->selectionModel());
    if(selsection)
        return selsection->isSelectedEx(col, row);
    return false;
}

int64_t CHexViewModel::GetRowCount() const
{
    if(!m_file.isOpen())
        return 0;

    auto size = m_file.size();
    auto rows = size / m_cols_hex;
    if(size % m_cols_hex)
        rows++;

    return rows;
}

void CHexViewModel::SetInfo(int64_t val) const
{
    DEBUGTRACE();
    if(!m_file.isOpen())
    {
        m_lineEditInfo->setText("");
        return;
    }

    auto size = m_file.size() - 1;

    auto str = QString("DEC [%1 - %2] HEX [0x%3 - 0x%4]")
            .arg(val, 8, 10, QLatin1Char('0'))
            .arg(size, 8, 10, QLatin1Char('0'))
            .arg(val, 8, 16, QLatin1Char('0')).toUpper()
            .arg(size, 8, 16, QLatin1Char('0')).toUpper();

    m_lineEditInfo->setText(str);
    m_lineEditGoTo->setText(QString("%1").arg(val, 8, 10, QLatin1Char('0')));

}

int CHexViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    int64_t visibleCount = m_hexView->height() / m_hexView->verticalHeader()->defaultSectionSize();
    visibleCount--;

    if(visibleCount < 1)
        visibleCount = 1;

    auto res = std::min(GetRowCount(), visibleCount);
    uint mark64;
    return res;
}

int CHexViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_cols_hex * 2;
}

QString CHexViewModel::dataEx(int64_t row, int64_t col) const
{

    if(!m_file.isOpen())
        return " ";

    if(m_cachePos != row)
    {
        m_cachePos = row;
        if(!m_file.seek(row * m_cols_hex))
        {
            memset(m_buffer.data(), 0, m_buffer.size());
        }
        else
        {
            m_cacheLen = m_file.read(reinterpret_cast<char*>(m_buffer.data()), static_cast<int64_t>(m_buffer.size()));
        }
    }

    if(col >= m_cols_hex)
    {
        col -= m_cols_hex;
        if(col >= 0 && m_cacheLen > col)
        {
            auto charx = m_buffer[static_cast<uint32_t>(col)];
            if(charx <= 0x1F || charx >= 0x7F)
                charx = '.';

            return QChar(charx);
        }
    }
    else if(col >= 0 && m_cacheLen > col)
    {
        return QString("%1").arg(m_buffer[static_cast<uint32_t>(col)], 2, 16, QLatin1Char('0')).toUpper();
    }

    return " ";
}

QVariant CHexViewModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        auto indRow = static_cast<int64_t>(index.row()) + m_pVerticalScrollBarHexView->value();
        auto val = m_editview->GetEditStatus(indRow, index.column(), m_cols_hex);
        if(!val.isNull())
            return val;
        return dataEx(indRow, index.column());
    }
    return QVariant();
}

QVariant CHexViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //DEBUGTRACE();
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        if(section >= m_cols_hex)
        {
            return " ";
        }

        return QString("%1").arg(section, 1, 16, QLatin1Char('0')).toUpper();
    }
    else if(orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        section += m_pVerticalScrollBarHexView->value();

        if(section >= (m_pVerticalScrollBarHexView->maximum() + rowCount(QModelIndex())))
            return "";

        auto val = static_cast<uint64_t>(section) * static_cast<uint64_t>(m_cols_hex);
        QString str("   ");
        str += QString("%1").arg(val, 8, 16, QLatin1Char('0')).toUpper();
        str += "   ";
        return str;
    }

    return QVariant();
}

QColor CHexViewModel::GetCellStatus(const QModelIndex &index) const
{
    //DEBUGTRACE();
    auto indRow = static_cast<int64_t>(index.row()) + m_pVerticalScrollBarHexView->value();

    auto col = index.column();
    auto pos = indRow * m_cols_hex;
    pos += col;

    if(col >= m_cols_hex)
        pos -= m_cols_hex;

    switch (m_editview->GetCellStatus(pos))
    {
        case CEditEvent::CEditEventOverwrite:return m_color_overwrite;
        case CEditEvent::CEditEventInsert:   return m_color_insert;
        case CEditEvent::CEditEventDelete:   return m_color_delete;
        default: break;
    }

    if(m_pcsearch->GetCellStatus(pos))
    {
        return m_color_search;
    }

    return nullptr;
}

void CHexViewModel::Reset()
{
    DEBUGTRACE();
    m_cachePos = -1;
}

void CHexViewModel::RepaintDisplay() const
{
    DEBUGTRACE();
    m_hexView->viewport()->repaint();
}

void CHexViewModel::SelectPosition(int64_t pos)
{
    DEBUGTRACE();
    auto scrollRow = pos / m_cols_hex;
    m_pVerticalScrollBarHexView->setValue(scrollRow);
}

void CHexViewModel::RedrawDisplayArea()
{
    DEBUGTRACE();
    m_hexView->verticalHeader()->viewport()->update(0, 0, m_hexView->verticalHeader()->viewport()->width(), m_hexView->verticalHeader()->viewport()->height());
    m_hexView->viewport()->update(0, 0, m_hexView->viewport()->width(), m_hexView->viewport()->height());
}

void CHexViewModel::lineEdit_goto_textEdited(const QString &arg1)
{
    DEBUGTRACE();
    SelectPosition(arg1.toLongLong());
}

int64_t CHexViewModel::GetCurrentPos()
{
    DEBUGTRACE();
    auto selsection = static_cast<CHexViewSelectionModel*>(m_hexView->selectionModel());
    if(!selsection)
        return -1;

    CHexViewSelectionModelItem item;
    if(!selsection->GetSelectedEx(nullptr, &item))
    {
        if(m_file.isOpen() && m_file.size() == 0)
            return 0;
        return -1;
    }

    if(item.column >= m_cols_hex)
        item.column -= m_cols_hex;

    auto pos = item.row * m_cols_hex + item.column;

    return pos;
}

bool CHexViewModel::EventHandler(QEvent *event) const
{
    //DEBUGTRACE();
    return m_pVerticalScrollBarHexView->event(event);
}

void CHexViewModel::UpdateScrollbarProps() const
{
    DEBUGTRACE();
    auto nRowsTotal = GetRowCount();
    auto nRowsVisible = rowCount(QModelIndex());
    auto nRowsRes = nRowsTotal - nRowsVisible;
    if( nRowsRes < 0 )
        nRowsRes = 0;

    m_pVerticalScrollBarHexView->setMaximum(nRowsRes);
}



