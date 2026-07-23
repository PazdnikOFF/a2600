#pragma once

#include <QLineEdit>

// Строка-спеллинг пиньинь-клавиатуры (реф. KPinyinLineEdit : QLineEdit, X-2600).
// Живёт внутри KPinyinWidget (реф. KPinyinWidget::InitUI @0x763db8 создаёт именно её,
// а не голый QLineEdit — прежний порт подставлял QLineEdit).
//
// Класс крошечный, восстановлен ЦЕЛИКОМ (все 7 методов):
//   ctor @0x763d80 — только `QLineEdit(parent)` + установка vptr, больше ничего;
//   text/insert/setText @0x764220/0x764248/0x764250 — прямые проксирования в QLineEdit
//     (insert/setText — хвостовые вызовы, т.е. поведение базы без изменений);
//   focusOutEvent @0x764218 — КЛЮЧЕВОЕ: хвостовой вызов `QWidget::focusOutEvent`,
//     то есть реализация QLineEdit НАМЕРЕННО ПРОПУСКАЕТСЯ. Поэтому при уходе фокуса
//     строка не сбрасывает выделение и не «завершает» ввод — попап кандидатов может
//     забрать фокус, а спеллинг остаётся как есть;
//   paintEvent @0x765ca0 — заливает `QLineEdit::cursorRect()` белой сплошной кистью
//     (QBrush(Qt::white, Qt::SolidPattern)) и ТОЛЬКО ПОТОМ зовёт `QLineEdit::paintEvent`.
//     Порядок сверен дизасмом (fillRect @0x765d14 → база @0x765d20). Практического
//     эффекта заливка почти не даёт — база перерисовывает поверх; воспроизводим как есть,
//     не «исправляя» реф.
class KPinyinLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KPinyinLineEdit(QWidget *parent = nullptr);

    QString text() const;                  // реф. @0x764220
    void insert(const QString &s);         // реф. @0x764248
    void setText(const QString &s);        // реф. @0x764250

protected:
    void focusOutEvent(QFocusEvent *e) override;   // реф. @0x764218
    void paintEvent(QPaintEvent *e) override;      // реф. @0x765ca0
};
