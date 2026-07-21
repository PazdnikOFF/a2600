#pragma once

#include <QLineEdit>

// Одно-полевой IP-редактор с ручной маской (реф. KIpLineEdit @ctor 0x6f9030, base QLineEdit).
// UI-порт РАНЕЕ ОТЛОЖЕННОГО варианта (при порте KIpAddrEdit взят KIpLineEdit1-октет + composite;
// этот — устаревший single-field, весь адрес в одном QLineEdit). БЕЗ inputMask/QValidator —
// вся валидация вручную в IpAddressInputMask (textChanged): нормализация формата + авто-точка
// при 3 цифрах октета + пер-октетный диапазон (non-subnet: octet0 1..223, 127/>223 отклон;
// иначе 0..255; subnet: все 0..255). text() без пробелов; isValidIP = 4 непустых октета.
// 100% PORT (реф. показывает KMessageBox на выход-за-диапазон — в порте молча клипуем, помечено).
class KIpLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KIpLineEdit(QWidget *parent = nullptr);

    QString text() const;                       // реф. @0x6f90d8: без пробелов
    void setText(const QString &t);             // реф. @0x6f9228: «»→скелет
    bool isValidIP() const;                     // реф. @0x6f9b38: 4 непустых октета
    void SetSubnetMaskEnable(bool b) { m_subnetMask = b; }   // реф. @0x6f9220

private slots:
    void IpAddressInputMask(const QString &t);  // реф. @0x6faf00

private:
    QString formatText(const QString &in) const;   // реф. @0x6fa530: клип октетов

    bool m_subnetMask = false;   // +0x38
    bool m_updating = false;     // антирекурсия setText/textChanged
};
