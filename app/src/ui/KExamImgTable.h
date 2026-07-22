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

// ── KUnusedImg триплет (реф.): = KExamImg + filename под тумбнейлом; сетка 3×7; одиночный клик
// инвертирует выбор (реф. SetSelectedInverse) + сигнал RequestUpdateSelectedImgFromUnusedView. ──
class KUnusedImgModel : public KImgTableModel
{
    Q_OBJECT
public:
    explicit KUnusedImgModel(QObject *parent = nullptr);   // реф. 3×7
    void LoadImages(const QVector<KImgTableItem *> &items) { SetImages(items); }
};

class KUnusedImgDelegate : public KExamImgDelegate
{
    Q_OBJECT
public:
    explicit KUnusedImgDelegate(int imgW = 130, int imgH = 110, QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
};

class KUnusedImgView : public KImgTableView
{
    Q_OBJECT
public:
    explicit KUnusedImgView(QWidget *parent = nullptr);
signals:
    void RequestUpdateSelectedImgFromUnusedView();
protected:
    void mousePressEvent(QMouseEvent *e) override;
};

// ── KExamDetail триплет (реф.): делегат = чекбокс (вместо order-num) + filename + type-aware
// (image/video/report TR_Rprt); сетка динамическая (в порте задаётся ctor); двойной клик
// инвертирует выбор + сигнал SelectedChanged + type-диспетч (в порте — только сигнал). ──
class KExamDetailModel : public KImgTableModel
{
    Q_OBJECT
public:
    explicit KExamDetailModel(int rows = 3, int cols = 5, QObject *parent = nullptr);
    void LoadImages(const QVector<KImgTableItem *> &items) { SetImages(items); }
};

class KExamDetailDelegate : public KImgTableDelegate
{
    Q_OBJECT
public:
    explicit KExamDetailDelegate(int imgW = 130, int imgH = 110, QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
private:
    int m_imgW, m_imgH;
};

class KExamDetailView : public KImgTableView
{
    Q_OBJECT
public:
    explicit KExamDetailView(int rows = 3, int cols = 5, QWidget *parent = nullptr);
signals:
    void SelectedChanged();
protected:
    void mouseDoubleClickEvent(QMouseEvent *e) override;
};
