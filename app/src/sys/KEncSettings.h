#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

// Аксессор к зашифрованным файлам-спискам моделей эндоскопов/камер (реф. KEncSettings,
// X-2600). База QObject (НЕ диалог/OSD/железо) — чистая файловая логика на QFile/QSettings.
//
// «Enc» = Encrypted: шифрование — побайтовое дополнение (bitwise NOT, ~b). Файл хранит
// ОДИН CSV-блоб моделей (genc/cenc/uenc/denc/benc.ini — гастро/коло/uh/ed/eb), а не
// секции/ключи: аргументы key у value/setValue ИГНОРИРУЮТСЯ (API в стиле QSettings).
// Закрывает отложенный getScopeList нашего KEncStyle (глобальный enc-список моделей).
//
// Путь — аргумент ctor (KEncStyle склеивает <stylepath>/scope/<x>enc.ini). Не синглтон.
class KEncSettings : public QObject
{
public:
    explicit KEncSettings(const QString &path, QObject *parent = nullptr);

    // Чтение/запись с побайтовым NOT. readData возвращает число прочитанных байт
    // (0 при ошибке открытия); writeData возвращает len; файл НЕ усекается (реф.).
    // writeData портит буфер вызывающего in-place (реф.).
    int readData(char *buf, int len, int offset);
    int writeData(char *buf, int len, int offset);
    int getDataLen();   // размер файла

    // key ИГНОРИРУЕТСЯ — хранится один блоб. value: пустой файл → def.
    QVariant value(const QString &key, const QVariant &def = QVariant());
    void setValue(const QString &key, const QString &value);   // чанки ≤1024 байт

    QStringList getStringList();                    // value("").split(",", KeepEmpty)
    void loadFileFromList(const QStringList &list);  // join "," → setValue (зашифрованно)
    // Импорт с USB-ini: QSettings(usbIniPath) → Model/Num + Model/ID<i> → блоб.
    void loadFileFromUsb(const QString &usbIniPath);

private:
    QString path_;   // +0x10
};
