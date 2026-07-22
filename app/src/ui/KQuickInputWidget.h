#pragma once

#include <QWidget>
#include <QDate>
#include <QString>
#include <QStringList>

#include <functional>

class QListView;
class QStringListModel;
class QModelIndex;

// Буфер совпадений быстрого ввода (реф. `_ListBuff`, встроен в KQuickInputWidget по +0x48).
// НЕ список строк: параллельные массивы ФИКСИРОВАННОЙ длины 10 + счётчик — layout сверен
// дизасмом Get*/ClearListBuffData/SearchMatchItem:
//   Id[10]     std::string  +0x48  (шаг 0x20)
//   Name[10]   std::string  +0x188
//   Gender[10] int          +0x2c8 (шаг 4)
//   DoB[10]    std::string  +0x2f0 (формат "yyyy-MM-dd", литерал @0x85dc10)
//   Age[10]    int          +0x430
//   Count      int          +0x458
// Предел 10 записей зашит в каждый геттер (`cmp w1, #0x9`).
struct _ListBuff
{
    enum { kMaxItems = 10 };

    QString Id[kMaxItems];
    QString Name[kMaxItems];
    int     Gender[kMaxItems];
    QString DoB[kMaxItems];
    int     Age[kMaxItems];
    int     Count = 0;

    _ListBuff() { Clear(); }
    // Реф. KQuickInputWidget::ClearListBuffData @0x692a70: строки — пустые, Gender=2 (!),
    // Age=0, Count=0.
    void Clear();
};

// Find-попап для KMemComboBox (реф. KQuickInputWidget @ctor 0x691a70, безрамочное Popup-окно).
// Живой инкрементальный поиск: QListView + QStringListModel под полем ввода. Владелец
// (KMemComboBox) ставит windowFlags(Popup|Frameless) и позиционирует его.
//
// ВАЖНО (сверено дизасмом, исправляет прежний порт): отображаемая строка строится в
// SearchMatchItem @0x692d68 как «Id + " - " + Name» (сепаратор — литерал @0x88ba80), с
// откатом на голое Name, если Id пуст; записи с пустым Name пропускаются целиком. Сами же
// GetId/GetName/GetGender/GetDoB/GetAge читают ПОЛЯ буфера, а не парсят строку.
//
// DEVICE-STUB: реф. SearchMatchItem по имени таблицы (+0x460) роутит на
// KQuickInput{Patient,Doctor,Applicant}DbTableHandler::GetMatchDate / KQuickInputReportTitle*
// (зашифрованная SQLite, ключ SONOSCOPE_X2000_KEY). В порте — инъектируемый провайдер
// SetMatchProvider(fn): fn(prefix, buff) заполняет `_ListBuff`. По умолчанию пуст.
class KQuickInputWidget : public QWidget
{
    Q_OBJECT
public:
    using MatchProvider = std::function<void(const QString &prefix, _ListBuff &buff)>;

    explicit KQuickInputWidget(QWidget *parent = nullptr);

    void SetTableName(const QString &table);        // реф. @0x6921c8 → +0x460
    void SetMatchProvider(MatchProvider fn);         // DEVICE-STUB инъекция
    // Реф. @0x692d68: очистить буфер → GetMatchDate(prefix, buff) → собрать строки показа.
    QStringList SearchMatchItem(const QString &prefix);
    void ClearListBuffData();                        // реф. @0x692a70

    void SetItems(const QStringList &items);   // наполнить модель + подогнать высоту
    int CurrentRow() const;
    void MoveCursor(int delta);                // клавиатурная навигация (реф. MovePrev/Next)
    int Count() const;

    // Реф. геттеры полей буфера. Индекс вне [0,9] → пусто / Gender=2 / Age=0 / QDate().
    QString GetId(int index) const;        // реф. @0x6921c8
    QString GetName(int index) const;      // реф. @0x692250
    int     GetGender(int index) const;    // реф. @0x6924f8
    QDate   GetDoB(int index) const;       // реф. @0x692518 (пустая строка → 2100-01-01)
    int     GetAge(int index) const;       // реф. @0x692688

signals:
    void itemSelect(int index);   // реф. @0x82b9e8

protected:
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void onClicked(const QModelIndex &idx);

private:
    int GetListViewHeight() const;

    QListView        *m_list = nullptr;
    QStringListModel *m_model = nullptr;
    QString           m_tableName;      // +0x460
    _ListBuff         m_buff;           // +0x48
    MatchProvider     m_matchProvider;
};
