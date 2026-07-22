#include "KQuickInputWidget.h"

#include <QListView>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QKeyEvent>

// Сепаратор строки показа «Id - Name» (реф. литерал @0x88ba80, длина 3).
static const QString kSep = QStringLiteral(" - ");
// Формат даты рождения в буфере (реф. литерал @0x85dc10).
static const QString kDoBFormat = QStringLiteral("yyyy-MM-dd");

void _ListBuff::Clear()
{
    // Реф. ClearListBuffData @0x692a70: 10 итераций, Gender=2 (mov w0,#2; str w0,[x20]),
    // Age=0 (str wzr,[x20,#360]); строки очищаются; Count=0.
    for (int i = 0; i < kMaxItems; ++i) {
        Id[i].clear();
        Name[i].clear();
        DoB[i].clear();
        Gender[i] = 2;
        Age[i] = 0;
    }
    Count = 0;
}

KQuickInputWidget::KQuickInputWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("KQuickInputWidget"));
    m_model = new QStringListModel(this);
    m_list = new QListView(this);
    m_list->setObjectName(QStringLiteral("quickInputList"));
    m_list->setModel(m_model);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(m_list);

    connect(m_list, &QListView::clicked, this, &KQuickInputWidget::onClicked);
}

void KQuickInputWidget::SetTableName(const QString &table)
{
    m_tableName = table;   // реф. @0x6921c8 → +0x460 (по нему роутится GetMatchDate)
}

void KQuickInputWidget::SetMatchProvider(MatchProvider fn)
{
    m_matchProvider = std::move(fn);
}

void KQuickInputWidget::ClearListBuffData()
{
    m_buff.Clear();
}

QStringList KQuickInputWidget::SearchMatchItem(const QString &prefix)
{
    // Реф. @0x692d68: ClearListBuffData → роутинг по m_tableName → GetMatchDate(prefix, buff)
    // → цикл по 10 слотам. Пустое Name — слот пропускается (не считается). Иначе ++Count и
    // строка показа: Id пуст → голое Name, иначе Id + " - " + Name.
    ClearListBuffData();
    if (m_matchProvider)
        m_matchProvider(prefix, m_buff);

    QStringList out;
    m_buff.Count = 0;
    for (int i = 0; i < _ListBuff::kMaxItems; ++i) {
        if (m_buff.Name[i].isEmpty())
            continue;
        ++m_buff.Count;
        out << (m_buff.Id[i].isEmpty() ? m_buff.Name[i]
                                       : m_buff.Id[i] + kSep + m_buff.Name[i]);
    }
    return out;
}

QString KQuickInputWidget::GetId(int index) const
{
    if (index < 0 || index >= _ListBuff::kMaxItems)
        return QString();   // реф.: fromAscii_helper("") при index > 9
    return m_buff.Id[index];
}

QString KQuickInputWidget::GetName(int index) const
{
    if (index < 0 || index >= _ListBuff::kMaxItems)
        return QString();
    return m_buff.Name[index];
}

int KQuickInputWidget::GetGender(int index) const
{
    if (index < 0 || index >= _ListBuff::kMaxItems)
        return 2;   // реф. @0x69250c: mov w0,#2
    return m_buff.Gender[index];
}

QDate KQuickInputWidget::GetDoB(int index) const
{
    if (index < 0 || index >= _ListBuff::kMaxItems)
        return QDate();                  // реф.: QDate(0,0,0) — невалидна
    const QString &s = m_buff.DoB[index];
    if (s.isEmpty())
        return QDate(2100, 1, 1);        // реф. @0x6925a0: QDate(0x834,1,1)
    return QDate::fromString(s, kDoBFormat);
}

int KQuickInputWidget::GetAge(int index) const
{
    if (index < 0 || index >= _ListBuff::kMaxItems)
        return 0;   // реф. @0x69269c
    return m_buff.Age[index];
}

void KQuickInputWidget::SetItems(const QStringList &items)
{
    m_model->setStringList(items);
    if (!items.isEmpty())
        m_list->setCurrentIndex(m_model->index(0, 0));
    resize(width() > 0 ? width() : 200, GetListViewHeight());
}

int KQuickInputWidget::Count() const
{
    return m_model->rowCount();
}

int KQuickInputWidget::GetListViewHeight() const
{
    // Высота под число строк (реф. GetListViewHeight), с потолком.
    const int rows = m_model->rowCount();
    const int rowH = m_list->sizeHintForRow(0) > 0 ? m_list->sizeHintForRow(0) : 24;
    const int shown = qMin(rows, 8);   // потолок видимых строк
    return qMax(rowH, shown * rowH + 4);
}

int KQuickInputWidget::CurrentRow() const
{
    return m_list->currentIndex().row();
}

void KQuickInputWidget::MoveCursor(int delta)
{
    const int n = m_model->rowCount();
    if (n == 0)
        return;
    int r = CurrentRow() + delta;
    if (r < 0) r = 0;
    if (r >= n) r = n - 1;
    m_list->setCurrentIndex(m_model->index(r, 0));
}

void KQuickInputWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Up:    MoveCursor(-1); return;
    case Qt::Key_Down:  MoveCursor(+1); return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (CurrentRow() >= 0) { emit itemSelect(CurrentRow()); hide(); }
        return;
    case Qt::Key_Escape: hide(); return;
    default: QWidget::keyPressEvent(e);
    }
}

void KQuickInputWidget::onClicked(const QModelIndex &idx)
{
    if (idx.isValid()) {
        emit itemSelect(idx.row());
        hide();
    }
}
