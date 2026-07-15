#pragma once

#include <QString>
#include <QHash>
#include <QVector>

// Карта одного языка внутри раскладки (реф. struct stUnicode2TextMap):
// имя клавиши (SONO_*) → её текст/символ для данного языка.
struct stUnicode2TextMap {
    QString language;                     // Russian/Latin/French/Polish/Hungary
    QHash<QString, QString> keyToText;    // SONO_xxx → символ
};

// Раскладка для пары (тип машины, версия клавиатуры) (реф. struct
// stUnicode2TextLayout): содержит по одной карте на язык.
struct stUnicode2TextLayout {
    QString machineType;                  // PAD
    QString keyboardVersion;              // 1.1
    QVector<stUnicode2TextMap> maps;      // по языкам
};

// Загрузчик карты Unicode→текст экранной клавиатуры (реф. KLoadUnicodeText, X-2600).
// Читает system/language/multi_language_unicode_2_text.xml:
//   MachineType(PAD) → KeyboardVersion(1.1) → LanguageType(язык) → Key(name=SONO_* → символ).
// Спец-правило name2value: если текст элемента == "name2value", реальный символ берётся
// из атрибута value= (для символов, которые нельзя записать как текст XML: &, <, пробел).
//
// В оригинале библиотека раскладок — файловый глобал g_s_vec_stUnic2TextLayoutLib;
// здесь держится в синглтоне. Off-device-ядро (парсинг+лукап); маппинг int-кода клавиши
// в имя SONO_* (реф. KKey2Name::GetNameOfKey) — device/клавиатура, не здесь.
//
// ВАЖНО: файл прошивки — НЕвалидный строгий XML (голые &, <, пробел в value="..."),
// поэтому парсим построчно (оригинальный парсер тоже лоялен к этому), а не QXmlStreamReader.
class KLoadUnicodeText
{
public:
    static KLoadUnicodeText &GetInstance();

    // Построить библиотеку раскладок из XML (реф. BuildUnic2TextLayoutLibFromXml).
    // По умолчанию — из KSystem (system/language/...). true при успехе.
    bool BuildUnic2TextLayoutLibFromXml(const QString &xmlPath = QString());

    // Найти текст по (тип машины, версия клавиатуры, имя клавиши, язык)
    // (реф. FindTextFromUnic2TextLayoutLib). Не найдено → пустая строка.
    QString FindTextFromUnic2TextLayoutLib(const QString &machineType,
                                           const QString &keyboardVersion,
                                           const QString &keyName,
                                           const QString &language) const;

    const QVector<stUnicode2TextLayout> &Lib() const { return lib_; }

private:
    KLoadUnicodeText() = default;
    QString cfgFile(const QString &given) const;
    QVector<stUnicode2TextLayout> lib_;
};
