#include "sys/KLoadUnicodeText.h"
#include "sys/KSystem.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDir>

KLoadUnicodeText &KLoadUnicodeText::GetInstance()
{
    static KLoadUnicodeText inst;
    return inst;
}

QString KLoadUnicodeText::cfgFile(const QString &given) const
{
    if (!given.isEmpty())
        return given;
    // Реф. путь: system/language/multi_language_unicode_2_text.xml.
    return QDir(KSystem::SystemPath())
        .absoluteFilePath("language/multi_language_unicode_2_text.xml");
}

bool KLoadUnicodeText::BuildUnic2TextLayoutLibFromXml(const QString &xmlPath)
{
    lib_.clear();
    QFile f(cfgFile(xmlPath));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&f);
    in.setCodec("UTF-8");

    // Построчный разбор (файл — НЕвалидный строгий XML: голые &/</пробел в value=).
    static const QRegularExpression reMachine(
        "<MachineType\\s+name=\"([^\"]*)\"");
    static const QRegularExpression reKeyboard(
        "<KeyboardVersion\\s+name=\"([^\"]*)\"");
    static const QRegularExpression reLang(
        "<LanguageType\\s+name=\"([^\"]*)\"");
    // Key: имя, необязательный value=, внутренний текст.
    static const QRegularExpression reKey(
        "<Key\\s+name=\"([^\"]*)\"(?:\\s+value=\"(.*?)\")?\\s*>(.*?)</Key>");

    QString curMachine, curKeyboard;
    int layoutIdx = -1;   // индекс текущей раскладки в lib_
    int mapIdx    = -1;   // индекс текущей карты языка в раскладке

    QString line;
    while (in.readLineInto(&line)) {
        auto mMachine = reMachine.match(line);
        if (mMachine.hasMatch()) {
            curMachine = mMachine.captured(1);
            continue;
        }
        auto mKb = reKeyboard.match(line);
        if (mKb.hasMatch()) {
            curKeyboard = mKb.captured(1);
            // Новая раскладка на (машина, версия клавиатуры).
            stUnicode2TextLayout layout;
            layout.machineType     = curMachine;
            layout.keyboardVersion = curKeyboard;
            lib_.append(layout);
            layoutIdx = lib_.size() - 1;
            mapIdx = -1;
            continue;
        }
        auto mLang = reLang.match(line);
        if (mLang.hasMatch() && layoutIdx >= 0) {
            stUnicode2TextMap m;
            m.language = mLang.captured(1);
            lib_[layoutIdx].maps.append(m);
            mapIdx = lib_[layoutIdx].maps.size() - 1;
            continue;
        }
        auto mKey = reKey.match(line);
        if (mKey.hasMatch() && layoutIdx >= 0 && mapIdx >= 0) {
            const QString name  = mKey.captured(1);
            const QString value = mKey.captured(2);   // может отсутствовать
            const QString text  = mKey.captured(3);
            // Спец-правило: текст "name2value" → символ из атрибута value=.
            const QString resolved = (text == "name2value") ? value : text;
            lib_[layoutIdx].maps[mapIdx].keyToText.insert(name, resolved);
        }
    }
    return !lib_.isEmpty();
}

QString KLoadUnicodeText::FindTextFromUnic2TextLayoutLib(
    const QString &machineType, const QString &keyboardVersion,
    const QString &keyName, const QString &language) const
{
    // Обход: раскладка по (машина, версия клавиатуры) → карта языка → имя клавиши.
    for (const auto &layout : lib_) {
        if (layout.machineType != machineType ||
            layout.keyboardVersion != keyboardVersion)
            continue;
        for (const auto &m : layout.maps) {
            if (m.language != language)
                continue;
            auto it = m.keyToText.find(keyName);
            if (it != m.keyToText.end())
                return it.value();
        }
    }
    return QString();   // не найдено
}
