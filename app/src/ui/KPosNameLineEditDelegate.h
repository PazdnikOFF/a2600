#pragma once

#include <QStyledItemDelegate>

// Делегат редактирования имени позиции (реф. KPosNameLineEditDelegate : QStyledItemDelegate
// + KObject, ctor @0x5a8fb0, sizeof 0x48). UI-порт.
//
// Реф. подписывается на сообщение шины KObject::SubscribeMsg(10001) и отписывается в dtor,
// но HandleMsg НЕ переопределяет ⇒ подписка на поведение не влияет; вторая база (KObject)
// в порт не тянется — это DEVICE-seam.
//
// createEditor @0x5a90b0 — весь класс: новый QLineEdit(parent), setMaxLength(50) и
// stylesheet «QLineEdit { background-color:#1a1a1a; outline: none;}» (@0x880af8, 53 симв.).
// Валидатора/маски НЕТ; setEditorData/setModelData/paint не переопределены.
//
// Ставится в KReportEditAddMarkView::InitPosName @0x4bfa38 через setItemDelegate НА ВЕСЬ
// вид (не на колонку).
class KPosNameLineEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit KPosNameLineEditDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
};
