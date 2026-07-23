#include "KPosNameLineEditDelegate.h"

#include <QLineEdit>

KPosNameLineEditDelegate::KPosNameLineEditDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    // Реф. ctor @0x5a8fb0: + KObject::SubscribeMsg(10001) — DEVICE-seam, опущено
    // (обработчика сообщения у делегата всё равно нет).
}

QWidget *KPosNameLineEditDelegate::createEditor(QWidget *parent,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const
{
    // Реф. @0x5a90b0: ровно три шага, аргументы option/index не используются.
    Q_UNUSED(option);
    Q_UNUSED(index);
    auto *edit = new QLineEdit(parent);
    edit->setMaxLength(50);   // реф. 0x32
    edit->setStyleSheet(QStringLiteral("QLineEdit { background-color:#1a1a1a; outline: none;}"));
    return edit;
}
