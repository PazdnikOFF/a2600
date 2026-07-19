#include "sys/KThirdPartyLegalNotices.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

QString KThirdPartyLegalNotices::LegalNoticeDir()
{
    // Реф.: SystemPath() + "presetdata/thirdpartylegalnotice/" (литерал длиной 33).
    return QDir(KSystem::SystemPath()).absoluteFilePath("presetdata/thirdpartylegalnotice");
}

void KThirdPartyLegalNotices::ReadLegalNoticeText(QString &out, const QString &fileName)
{
    const QString path = QDir(LegalNoticeDir()).absoluteFilePath(fileName);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        // Реф.-текст лога сохранён ДОСЛОВНО, включая опечатку "fialed" (уровень [APP][W]).
        qWarning("LegalNotice.open(QFile::ReadOnly) fialed.");
        return;                       // out НЕ ТРОГАЕТСЯ
    }
    QTextStream ts(&f);
    out = ts.readAll();
    f.close();
}
