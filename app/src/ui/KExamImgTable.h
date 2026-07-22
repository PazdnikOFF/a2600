#pragma once

#include "KImgTableView.h"

// Триплет MVC грид-таблицы снимков осмотра (реф. KExamImg{Model,View,Delegate}, хостится
// KReportEditUi::InitExamImg). Сетка 1×10 (горизонт. полоса тумбнейлов), ячейка 159×130,
// тумбнейл 130×110, скругление r8. Источник строк реф. = глобальный g_vecSelectedItems (device)
// → в порте инъект-список (LoadImages). Делегат рисует тумбнейл + order-num-бейдж + рамку.

// Модель (реф. KExamImgModel : KImgTableModel : QStandardItemModel). Тонкая: 1×10 + seam-загрузка.
class KExamImgModel : public KImgTableModel
{
    Q_OBJECT
public:
    explicit KExamImgModel(QObject *parent = nullptr);
    void LoadImages(const QVector<KImgTableItem *> &items) { SetImages(items); }   // seam
};

// Делегат (реф. KExamImgDelegate : KImgTableDelegate). Богатый рендер: центрир. скруглённый
// тумбнейл + order-num-бейдж (top-right 30×20, darkGray/cyan) + рамка состояния (blue/cyan/grey).
class KExamImgDelegate : public KImgTableDelegate
{
    Q_OBJECT
public:
    explicit KExamImgDelegate(int imgW = 130, int imgH = 110, QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;

private:
    int m_imgW, m_imgH;
};

// Вью (реф. KExamImgView : KImgTableView). 1×10, фикс-высота 130; клик/двойной клик по ячейке
// эмитят сигналы (реф. RequestSwitchImgOfAddMark / RequestUpdateUnusedImg).
class KExamImgView : public KImgTableView
{
    Q_OBJECT
public:
    explicit KExamImgView(QWidget *parent = nullptr);

signals:
    void RequestUpdateUnusedImg();
    void RequestSwitchImgOfAddMark(KImgTableItem *item);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
    void OnItemActivated(const QModelIndex &idx);
};
