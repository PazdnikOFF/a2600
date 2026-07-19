#pragma once

#include <QString>

// Уведомления о стороннем ПО (реф. KThirdPartyLegalNotices, X-2600).
// В оригинале это Qt-диалог (KDialog + moc: InitWidget/ShowInfo/OnExit, виджет
// "textEdit_legalNotice", свободная функция OpenThirdPartyLegalNotices()), но его
// прикладное ядро — чтение текстового файла — вынесено сюда как non-UI класс
// (тот же приём, что с KVideoCal). UI-часть — Фаза F.
class KThirdPartyLegalNotices
{
public:
    // Реф. ReadLegalNoticeText(QString &out, const QString &fileName) @0x5faa98 — void.
    // Путь: KSystem::SystemPath() + "presetdata/thirdpartylegalnotice/" + fileName.
    // Файл не открылся → лог-предупреждение и ВЫХОД; out НЕ ТРОГАЕТСЯ (реф.).
    // Реальное имя файла в прошивке — "thirdPartyLegalNotices.txt".
    static void ReadLegalNoticeText(QString &out, const QString &fileName);

    // Каталог уведомлений (реф. — литерал @0x884740, длина 33).
    static QString LegalNoticeDir();
};
