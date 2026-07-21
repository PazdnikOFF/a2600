#pragma once

#include <QComboBox>
#include <QStringList>
#include <functional>

class KQuickInputWidget;

// Редактируемый комбо с «памятью» (реф. KMemComboBox @ctor 0x690a48, base QComboBox).
// UI-порт РЕАЛЬНОГО кастом-виджета — ранее подставлялся editable QComboBox. Вместо
// нативного дропдауна — кастомный find-попап KQuickInputWidget с живым инкрементальным
// поиском под полем. На editTextChanged дергается OnRadarChange → провайдер совпадений →
// попап. Записи — «name - id» (сепаратор « - »). Наружу: itemSelect(int)/FocusOut().
// SetDisplayId(bool) переключает, что писать в поле при выборе (false→id, true→name).
//
// DEVICE-STUB: реф. персистентность — зашифрованный SQLite (KDbSqlite, ключ
// SONOSCOPE_X2000_KEY, таблицы tb_QuickInputApplicant/Patient/Doctor, tb_Report) через
// KQuickInput*DbTableHandler::GetMatchDate(prefix). В порте вынесено за инъектируемый
// провайдер SetMatchProvider(fn): fn(prefix) → QStringList «name - id». По умолчанию пуст
// (попап не показывается) — на устройстве подключается DB-провайдер, в превью — in-memory.
//
// ВАЛИДАТОР (реф. @0x883068): негативный класс, запрещающий пунктуацию/разделители. Точный
// 0x9b-байтовый литерал (вкл. unicode-fullwidth варианты) НЕ транскрибирован дословно —
// в порте ASCII-набор из декода субагента (аппроксимация, помечено).
class KMemComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit KMemComboBox(QWidget *parent = nullptr);

    void SetTableName(const QString &table, bool displayId);   // реф. @0x691628
    void SetDisplayId(bool displayId);                          // реф. @0x691370
    void SetMatchProvider(std::function<QStringList(const QString &)> fn);  // DEVICE-STUB инъекция

    void setMaxLength(int len);    // реф. @0x690cd8 — на lineEdit
    QString currentText() const;   // trimmed (реф. @0x690e00)
    QString text() const;          // алиас (реф. @0x690e98)
    void setText(const QString &t);// без триггера попапа (реф. @0x690ec0)

    QString GetName(int idx) const;   // из записи «name - id»
    QString GetId(int idx) const;

signals:
    void itemSelect(int index);   // реф. @0x82b668
    void FocusOut();              // реф. @0x82b698

protected:
    void focusOutEvent(QFocusEvent *) override;

private slots:
    void OnRadarChange(const QString &text);   // реф. @0x690fe0
    void ClickedSlot(int index);               // = SelectFindWndItem + HideFindWnd

private:
    void ShowFindWnd(const QString &prefix);   // реф. @0x690f68
    void HideFindWnd();
    void AdjustFindWndPos();
    void SelectFindWndItem(int index);         // реф. @0x691390

    KQuickInputWidget *m_popup = nullptr;   // +0x38
    bool m_displayId = false;               // +0x34 (false→id, true→name)
    QString m_tableName;                    // +0x460 попапа
    QStringList m_records;                  // текущие совпадения «name - id»
    std::function<QStringList(const QString &)> m_matchProvider;
};
