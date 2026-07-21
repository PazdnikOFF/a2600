#pragma once

#include <QLineEdit>
#include <QRegExp>

class QValidator;

// Поле ввода с regexp-фильтром (реф. KPasswordLineEdit @ctor 0x690200, base QLineEdit).
// UI-порт РЕАЛЬНОГО кастом-виджета — ранее подставлялся QLineEdit(Password). ВОПРЕКИ имени:
// это НЕ контейнер с «глазом» и НЕ ставит echoMode(Password) — просто QLineEdit с живым
// regexp-фильтром ввода. echoMode(Password) — ответственность вызывающего/.ui.
// Единственный член — QRegExp (m_regexp @+0x30). На каждый textChanged слот OnTextChanged
// обрезает хвост, пока строка не exactMatch'нет regexp, и переписывает при изменении.
// 100% PORT: чистый Qt, ноль device.
class KPasswordLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KPasswordLineEdit(QWidget *parent = nullptr);

    // Реф. @0x690318 — ПЕРЕОпределён и ведёт себя НЕ как база: из QRegExpValidator берёт
    // его QRegExp в m_regexp (сам QValidator отбрасывается). null → no-op.
    void setValidator(const QValidator *v);

private slots:
    void OnTextChanged(const QString &s);   // реф. @0x690368

private:
    QRegExp m_regexp;   // +0x30
};
