#include "sys/KEncSettings.h"

#include <QDebug>
#include <QFile>
#include <QSettings>

#include <cstring>
#include <vector>

KEncSettings::KEncSettings(const QString &path, QObject *parent)
    : QObject(parent), path_(path) {}

int KEncSettings::readData(char *buf, int len, int offset)
{
    QFile f(path_);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[APP][E]: readData open fail:" << path_;
        return 0;
    }
    std::memset(buf, 0, len);
    f.seek(offset);
    const int n = static_cast<int>(f.read(buf, len));
    for (int i = 0; i < n; ++i)
        buf[i] = ~buf[i];   // побайтовое дополнение
    f.close();
    return n;
}

int KEncSettings::writeData(char *buf, int len, int offset)
{
    QFile f(path_);
    if (!f.open(QIODevice::ReadWrite)) {   // реф. — ReadWrite, файл НЕ усекается
        qWarning() << "[APP][E]: writeData open fail:" << path_;
        return len;
    }
    for (int i = 0; i < len; ++i)
        buf[i] = ~buf[i];   // реф. портит буфер вызывающего in-place
    f.seek(offset);
    f.write(buf, len);
    f.close();
    return len;
}

int KEncSettings::getDataLen()
{
    return static_cast<int>(QFile(path_).size());
}

QVariant KEncSettings::value(const QString &key, const QVariant &def)
{
    Q_UNUSED(key);   // key игнорируется (реф.)
    const int len = getDataLen();
    if (len == 0)
        return def;
    std::vector<char> buf(len + 1, '\0');
    readData(buf.data(), len, 0);
    return QVariant(QString::fromUtf8(buf.data()));
}

void KEncSettings::setValue(const QString &key, const QString &value)
{
    Q_UNUSED(key);
    const QByteArray ba = value.toUtf8();
    int offset = 0;
    const int total = ba.size();
    // Реф. пишет чанками ≤1024 байт; файл предварительно не усекается (вызывающий
    // loadFileFromUsb делает remove() до записи).
    while (offset < total) {
        const int chunk = qMin(1024, total - offset);
        std::vector<char> tmp(ba.constData() + offset, ba.constData() + offset + chunk);
        writeData(tmp.data(), chunk, offset);
        offset += chunk;
    }
}

QStringList KEncSettings::getStringList()
{
    return value(QString()).toString().split(",", Qt::KeepEmptyParts, Qt::CaseSensitive);
}

void KEncSettings::loadFileFromList(const QStringList &list)
{
    // Сепаратор join — "," (по симметрии с getStringList; точный литерал реф. в
    // append-ветке не изолирован однозначно — принят ",").
    QFile(path_).remove();
    setValue(QString(), list.join(","));
}

void KEncSettings::loadFileFromUsb(const QString &usbIniPath)
{
    QSettings s(usbIniPath, QSettings::IniFormat);
    QFile(path_).remove();
    const int n = s.value("Model/Num").toInt();
    QStringList models;
    // Индекс старта цикла Model/ID из дизасма не восстановлен однозначно — принят
    // 0..n-1 (помечено). ID-ключ реф.: QString("Model/ID%1").arg(i).
    for (int i = 0; i < n; ++i)
        models << s.value(QString("Model/ID%1").arg(i)).toString();
    setValue(QString(), models.join(","));
}
