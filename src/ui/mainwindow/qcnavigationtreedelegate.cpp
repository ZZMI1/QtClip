#include "qcnavigationtreedelegate.h"

#include <QPainter>
#include <QStyleOptionViewItem>

QCNavigationTreeDelegate::QCNavigationTreeDelegate(QObject *pParent)
    : QStyledItemDelegate(pParent)
{
}

QCNavigationTreeDelegate::~QCNavigationTreeDelegate()
{
}

QSize QCNavigationTreeDelegate::sizeHint(const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if (index.parent().isValid())
        size.setHeight(qMax(size.height(), 24));
    else
        size.setHeight(qMax(size.height(), 28));
    return size;
}

void QCNavigationTreeDelegate::paint(QPainter *pPainter,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    if (nullptr == pPainter)
        return;

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    if (!index.parent().isValid())
        opt.font.setBold(true);

    if (opt.state & QStyle::State_Selected)
        opt.backgroundBrush = QColor(232, 240, 255);

    const QRect rectOriginal = opt.rect;
    opt.rect = rectOriginal.adjusted(6, 1, -4, -1);
    QStyledItemDelegate::paint(pPainter, opt, index);
}
