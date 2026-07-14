#include "db/KSaveFile.h"

#include <QDir>
#include <QFileInfo>

QString KSaveFile::FormatFlowNumber(int n)
{
    // реф.: 3-значный zero-padded; переполнение (>999) → "999^".
    if (n > kMaxFlow)
        return QString("%1%2").arg(kMaxFlow).arg(kOverflowMark);
    if (n < 0)
        n = 0;
    return QString("%1").arg(n, 3, 10, QChar('0'));
}

int KSaveFile::FlowNumberFromName(const QString &fileName)
{
    // Базовое имя без расширения ("001.jpg" → "001", "999^.mp4" → "999^").
    QString base = QFileInfo(fileName).completeBaseName();
    if (base.endsWith(kOverflowMark))
        base.chop(1);                 // "999^" → "999"
    bool ok = false;
    const int v = base.toInt(&ok);
    return (ok && v >= 0) ? v : -1;
}

QString KSaveFile::MakeFileName(int flow, bool video)
{
    return FormatFlowNumber(flow) + "." + (video ? VideoExt() : ImageExt());
}

int KSaveFile::FindMaxFileFlowNumber(const QString &dir)
{
    int maxFlow = -1;
    const QStringList files =
        QDir(dir).entryList({"*.jpg", "*.mp4"}, QDir::Files);
    for (const QString &f : files) {
        const int n = FlowNumberFromName(f);
        if (n > maxFlow)
            maxFlow = n;
    }
    return maxFlow;
}

int KSaveFile::NextFlowNumber(const QString &dir)
{
    const int mx = FindMaxFileFlowNumber(dir);
    const int next = mx + 1;          // -1 → 0 (первый файл)
    return next > kMaxFlow ? kMaxFlow : next;
}
