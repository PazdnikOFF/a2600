#include "KIpLineEdit.h"

#include <QStringList>

KIpLineEdit::KIpLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // Реф. ctor @0x6f9030: IME off + textChanged→маска. Без inputMask/QValidator.
    setAttribute(Qt::WA_InputMethodEnabled, false);
    connect(this, &QLineEdit::textChanged, this, &KIpLineEdit::IpAddressInputMask);
}

QString KIpLineEdit::text() const
{
    // Реф. @0x6f90d8: без пробелов.
    return QLineEdit::text().remove(QLatin1Char(' '));
}

void KIpLineEdit::setText(const QString &t)
{
    // Реф. @0x6f9228: пусто → скелет-плейсхолдер, иначе как есть.
    m_updating = true;
    QLineEdit::setText(t.isEmpty() ? QStringLiteral(" . . . ") : t);
    m_updating = false;
}

QString KIpLineEdit::formatText(const QString &in) const
{
    // Реф. @0x6fa530: сплит по «.», пер-октетный клип. Реф. на выходе-за-диапазон — KMessageBox;
    // в порте молча клипуем.
    const QStringList parts = in.split(QLatin1Char('.'));
    QStringList out;
    for (int i = 0; i < parts.size(); ++i) {
        QString p = parts[i].trimmed();
        if (p.isEmpty()) { out << p; continue; }
        bool ok = false;
        int v = p.toInt(&ok);
        if (!ok) { out << QString(); continue; }
        if (m_subnetMask) {
            if (v > 255) v = 255;
            if (v < 0) v = 0;
        } else if (i == 0) {
            if (v > 223) v = 223;          // реф. octet0 ≤ 223
            if (v == 127) v = 126;         // реф. 127 (loopback) отклоняется
            if (v < 1) v = 1;
        } else {
            if (v > 255) v = 255;
            if (v < 0) v = 0;
        }
        out << QString::number(v);
    }
    return out.join(QLatin1Char('.'));
}

void KIpLineEdit::IpAddressInputMask(const QString &t)
{
    // Реф. @0x6faf00: нормализовать + авто-точка при 3 цифрах октета; переписать при изменении.
    if (m_updating)
        return;
    QString stripped = t;
    stripped.remove(QLatin1Char(' '));
    QString norm = formatText(stripped);
    // Авто-точка: если последний октет достиг 3 цифр и точек < 3 — добавить точку.
    const QStringList octs = norm.split(QLatin1Char('.'));
    if (octs.size() < 4 && !octs.isEmpty() && octs.last().size() >= 3)
        norm += QLatin1Char('.');
    if (norm != t) {
        m_updating = true;
        QLineEdit::setText(norm);
        setCursorPosition(norm.size());
        m_updating = false;
    }
}

bool KIpLineEdit::isValidIP() const
{
    // Реф. @0x6f9b38: ровно 4 непустых октета.
    const QStringList octs = text().split(QLatin1Char('.'));
    if (octs.size() != 4)
        return false;
    for (const QString &o : octs)
        if (o.isEmpty())
            return false;
    return true;
}
