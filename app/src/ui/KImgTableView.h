#pragma once

#include <QTableView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QObject>
#include <QImage>
#include <QString>
#include <QVector>
#include <QMetaType>

// Элемент грид-таблицы изображений (реф. KImgTableItem : QObject, ctor @0x4b6310). Несёт
// QImage-тумбнейл (ленивая загрузка с диска), тип (0=image/1=video), пути, флаги. Данные —
// device (файлы захвата) → путь тумбнейла задаётся снаружи.
class KImgTableItem : public QObject
{
    Q_OBJECT
public:
    enum E_ITEM_TYPE { E_IMAGE = 0, E_VIDEO = 1 };
    explicit KImgTableItem(QObject *parent = nullptr);

    void SetImgPathThumb(const QString &p) { m_thumbPath = p; }
    QString GetImgPathThumb() const { return m_thumbPath; }
    void SetType(int t) { m_type = t; }
    int GetType() const { return m_type; }
    void SetIsSelected(bool s) { m_selected = s; }
    bool GetIsSelected() const { return m_selected; }
    bool isNullPath() const { return m_thumbPath.isEmpty(); }
    void LoadImgToQImage();            // реф.: ленивая загрузка m_thumbPath → m_img
    const QImage &Image() const { return m_img; }
    void SetImage(const QImage &img) { m_img = img; }   // прямая установка (стаб-тумбнейл)

    void SetOrderNum(int n) { m_orderNum = n; }   // реф. GetOrderNum (порядковый номер в отчёте)
    int GetOrderNum() const { return m_orderNum; }
    void SetEdited(bool e) { m_edited = e; }       // реф. GetEdited (есть аннотации)
    bool GetEdited() const { return m_edited; }
    void SetFileName(const QString &n) { m_fileName = n; }   // реф. GetFileBaseName
    QString GetFileName() const { return m_fileName; }

private:
    QImage m_img;          // +0x10
    int m_type = E_IMAGE;  // +0x30
    bool m_selected = false;
    bool m_edited = false;
    int m_orderNum = 0;
    QString m_thumbPath;   // +0x38
    QString m_fileName;
};

// Модель грид-таблицы изображений (реф. KImgTableModel : QStandardItemModel, ctor @0x4b6de8).
// data() отвечает ТОЛЬКО на DecorationRole → QVariant с указателем KImgTableItem* (кастомный
// метатип); делегат рисует. flat = cols*row + (page-1)*pageSize + col (reshape).
class KImgTableModel : public QStandardItemModel
{
    Q_OBJECT
public:
    KImgTableModel(int rows, int cols, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    void SetImages(const QVector<KImgTableItem *> &items);   // наполнить + пересчитать rows
    void SetCurrentPage(int page) { m_currentPage = page; }

private:
    QVector<KImgTableItem *> m_items;
    int m_cols = 1;
    int m_currentPage = 1;
    int m_pageSize = 0;
};

// Делегат: рисует тумбнейл по центру ячейки + рамку выделения (реф. KImgTableDelegate :
// QStyledItemDelegate, paint @0x4b41b0). Богатые подклассы (KExamImgDelegate: checkbox/
// order-num/edited-mark) не портируем — базовый рендер тумбнейла.
class KImgTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit KImgTableDelegate(int imgW, int imgH, QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;

private:
    int m_imgW, m_imgH;
};

// Грид-таблица тумбнейлов (реф. KImgTableView : QTableView, ctor @0x4b9ab0). Реальная
// reshape-сетка rows×cols фикс-ячеек; заголовки скрыты, showGrid off, NoEditTriggers,
// SingleSelection, WA_Hover. Подклассы KExamImgView/KUnusedImgView (разные источники/оверлеи).
class KImgTableView : public QTableView
{
    Q_OBJECT
public:
    explicit KImgTableView(QWidget *parent = nullptr);

    // Реф. InitTableView @0x4b9b90: размеры ячейки/тумбнейла + конфиг.
    void InitTableView(int cellW, int cellH, int imgW, int imgH);
    KImgTableModel *ImgModel() const { return m_model; }

private:
    KImgTableModel *m_model = nullptr;
    KImgTableDelegate *m_delegate = nullptr;
    int m_cols = 3;
};

Q_DECLARE_METATYPE(KImgTableItem *)
