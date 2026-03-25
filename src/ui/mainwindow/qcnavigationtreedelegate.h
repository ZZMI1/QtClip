#ifndef QTCLIP_QCNAVIGATIONTREEDELEGATE_H_
#define QTCLIP_QCNAVIGATIONTREEDELEGATE_H_

#include <QStyledItemDelegate>

class QCNavigationTreeDelegate : public QStyledItemDelegate
{
public:
    explicit QCNavigationTreeDelegate(QObject *pParent = nullptr);
    virtual ~QCNavigationTreeDelegate() override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
    void paint(QPainter *pPainter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};

#endif
