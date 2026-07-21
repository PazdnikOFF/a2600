#include "KIpAddrEdit.h"
#include "KMessageBox.h"

#include <QLabel>
#include <QIntValidator>
#include <QRegExpValidator>
#include <QKeyEvent>

// ── KIpLineEdit1 (октет) ─────────────────────────────────────────────────────

KIpLineEdit1::KIpLineEdit1(QWidget *parent)
    : QLineEdit(parent)
{
    // Реф. ctor @0x67ab90.
    setMaxLength(3);
    setFrame(false);
    setAlignment(Qt::AlignCenter);
    setContextMenuPolicy(Qt::NoContextMenu);
    setValidator(new QIntValidator(0, 255, this));
    connect(this, &QLineEdit::textEdited, this, &KIpLineEdit1::TextEdited);
}

void KIpLineEdit1::focusInEvent(QFocusEvent *e)
{
    // Реф. @0x67c3c8: выделить всё при получении фокуса.
    QLineEdit::focusInEvent(e);
    selectAll();
}

void KIpLineEdit1::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x67c3f8.
    const int k = e->key();
    if (k == Qt::Key_Period || k == Qt::Key_Space) {
        if (!text().isEmpty() && next) {
            next->setFocus();
            next->selectAll();
        }
        return;   // клавиша съедена
    }
    if ((k == Qt::Key_Backspace || k == Qt::Key_Delete) && text().isEmpty() && prev) {
        prev->setFocus();
        prev->selectAll();
        // затем передать базовому (реф. forward)
    }
    QLineEdit::keyPressEvent(e);
}

void KIpLineEdit1::TextEdited(const QString &t)
{
    // Реф. @0x67c680: ревалидация; авто-вперёд при 2 цифрах со значением ≥26.
    if (t.isEmpty())
        return;
    bool ok = false;
    const int v = t.toInt(&ok);
    if (ok && v >= 0 && v <= 255) {
        if (t.length() == 2 && v >= 26 && next) {
            // 3-я цифра могла бы переполнить 255 → сразу к следующему октету.
            next->setFocus();
            next->selectAll();
        }
    } else {
        // Вне диапазона — предупреждение + клип к 255 (реф.).
        KMessageBox::warning(this, tr("TR_Wng"), tr("TR_Rge0255"), QMessageBox::Ok);
        if (v > 255)
            setText(QStringLiteral("255"));
        setFocus();
        selectAll();
    }
}

// ── KIpAddrEdit (композит) ───────────────────────────────────────────────────

KIpAddrEdit::KIpAddrEdit(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x67acd0.
    setFocusPolicy(Qt::NoFocus);

    for (int i = 0; i < 4; ++i) {
        m_octet[i] = new KIpLineEdit1(this);
        m_octet[i]->setAlignment(Qt::AlignCenter);
    }
    for (int i = 0; i < 3; ++i) {
        m_dot[i] = new QLabel(QStringLiteral("."), this);
        m_dot[i]->setObjectName(QStringLiteral("IP_LABEL_DOT"));
        m_dot[i]->setAlignment(Qt::AlignCenter);
    }

    resize(200, 40);

    // Tab-order + указатели соседей (реф.: first->prev=self, last->next=self).
    for (int i = 0; i < 4; ++i) {
        m_octet[i]->next = (i < 3) ? m_octet[i + 1] : m_octet[i];
        m_octet[i]->prev = (i > 0) ? m_octet[i - 1] : m_octet[i];
        if (i < 3)
            setTabOrder(m_octet[i], m_octet[i + 1]);
        connect(m_octet[i], &QLineEdit::textChanged, this, &KIpAddrEdit::textChangedSlot);
        connect(m_octet[i], &QLineEdit::textEdited, this, &KIpAddrEdit::textEditedSlot);
    }
}

void KIpAddrEdit::resizeEvent(QResizeEvent *e)
{
    // Реф. @0x67bd60: дети позиционируются вручную (без QHBoxLayout). Точные координаты в
    // реф. не извлечены — равномерная раскладка: 4 октета + 3 точки по ширине.
    QFrame::resizeEvent(e);
    const int h = height();
    const int dotW = 8;
    const int octW = (width() - 3 * dotW) / 4;
    int x = 0;
    for (int i = 0; i < 4; ++i) {
        m_octet[i]->setGeometry(x, 0, octW, h);
        x += octW;
        if (i < 3) {
            m_dot[i]->setGeometry(x, 0, dotW, h);
            x += dotW;
        }
    }
}

void KIpAddrEdit::focusInEvent(QFocusEvent *)
{
    // Реф. @0x67c3b0: перенаправить фокус на первый октет.
    if (m_octet[0])
        m_octet[0]->setFocus();
}

QString KIpAddrEdit::joinOctets() const
{
    // Реф. @0x67be88: "" если любой октет пуст, иначе "a.b.c.d".
    for (int i = 0; i < 4; ++i)
        if (m_octet[i]->text().isEmpty())
            return QString();
    return QStringLiteral("%1.%2.%3.%4")
        .arg(m_octet[0]->text(), m_octet[1]->text(), m_octet[2]->text(), m_octet[3]->text());
}

QString KIpAddrEdit::text() const
{
    return joinOctets();
}

void KIpAddrEdit::SetText(const QString &ip)
{
    // Реф. @0x67c9a0: пусто → очистить все; иначе regex-валидация и сплит по «.».
    if (ip.isEmpty()) {
        for (int i = 0; i < 4; ++i)
            m_octet[i]->clear();
        return;
    }
    QRegExp re(QStringLiteral(
        "((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)"));
    QRegExpValidator v(re, nullptr);
    int pos = 0;
    QString s = ip;
    if (v.validate(s, pos) != QValidator::Acceptable)
        return;
    const QStringList parts = ip.split(QLatin1Char('.'));
    if (parts.size() != 4)
        return;
    for (int i = 0; i < 4; ++i)
        m_octet[i]->setText(parts[i]);
}

void KIpAddrEdit::textChangedSlot()
{
    emit textChanged(joinOctets());
}

void KIpAddrEdit::textEditedSlot()
{
    emit textEdited(joinOctets());
}
