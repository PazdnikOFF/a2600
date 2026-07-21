#pragma once

#include <QFrame>
#include <QLineEdit>

class QLabel;

// Октет-эдит: одно поле IP (реф. KIpLineEdit1 @ctor 0x67ab90, base QLineEdit). Кусок,
// из которого собран KIpAddrEdit. maxLength=3, без рамки, центр, без контекст-меню,
// валидатор QIntValidator(0,255). Указатели на соседние октеты (next/prev) для
// фокус-цепочки: авто-вперёд по «.» или когда 2 цифры со значением ≥26 (3-я переполнит
// 255); назад по Backspace в пустом поле. Вне диапазона → предупреждение + клип к 255.
class KIpLineEdit1 : public QLineEdit
{
    Q_OBJECT
public:
    explicit KIpLineEdit1(QWidget *parent = nullptr);
    KIpLineEdit1 *next = nullptr;   // +0x30
    KIpLineEdit1 *prev = nullptr;   // +0x38

protected:
    void focusInEvent(QFocusEvent *) override;   // реф. @0x67c3c8: selectAll
    void keyPressEvent(QKeyEvent *) override;     // реф. @0x67c3f8: «.»→next, BS в пустом→prev

private slots:
    void TextEdited(const QString &t);            // реф. @0x67c680
};

// Композитный IP-редактор (реф. KIpAddrEdit @ctor 0x67acd0, base QFrame): 4× KIpLineEdit1 +
// 3 метки-точки «.» (objectName IP_LABEL_DOT). UI-порт РЕАЛЬНОГО кастом-виджета — ранее
// подставлялся QLineEdit + inputMask "000.000.000.000;_". Геометрия детей — вручную в
// resizeEvent (без layout). resize(200,40), всё центрировано, tab-order 0→1→2→3, соседние
// указатели next/prev проставлены. text() джойнит «a.b.c.d» ("" если любой октет пуст);
// SetText() валидирует regex'ом и сплитит по «.». Сигналы наружу: textChanged(QString)/
// textEdited(QString) — оба реэмитят склеенную строку. 100% PORT (KMessageBox::warning —
// наш враппер QMessageBox).
class KIpAddrEdit : public QFrame
{
    Q_OBJECT
public:
    explicit KIpAddrEdit(QWidget *parent = nullptr);

    QString text() const;                 // реф. @0x67be88 — джойн «a.b.c.d» или ""
    void SetText(const QString &ip);      // реф. @0x67c9a0 — сплит по «.», regex-валидация

signals:
    void textChanged(const QString &ip);  // реф. @0x828b40
    void textEdited(const QString &ip);   // реф. @0x828b68

protected:
    void focusInEvent(QFocusEvent *) override;   // реф. @0x67c3b0: фокус на 1-й октет
    void resizeEvent(QResizeEvent *) override;   // реф. @0x67bd60: ручная раскладка

private slots:
    void textChangedSlot();
    void textEditedSlot();

private:
    KIpLineEdit1 *m_octet[4] = {nullptr, nullptr, nullptr, nullptr};   // +0x30/38/40/48
    QLabel *m_dot[3] = {nullptr, nullptr, nullptr};                     // +0x50/58/60
    QString joinOctets() const;
};
