#include "db/KSaveFile.h"

#include <QDir>
#include <QFileInfo>

QString KSaveFile::FormatFlowNumber(int n)
{
    // 1:1 с @0x6a89b8: std::stringstream, width(3) + fill('0'), `ss << n`.
    // Ни сравнения с 999, ни маркера '^' здесь НЕТ — переполнение оформляет
    // вызывающий (GetFileFlowNumber), см. MakeFlowName().
    QString s = QString::number(n);
    while (s.size() < 3)
        s.prepend(QChar('0'));        // левый паддинг, как у stringstream
    return s;
}

QString KSaveFile::MakeFlowName(int n)
{
    // Реф. GetFileFlowNumber @0x6a99d8 / KExamDataFileNameGenerator::GetFileSerialNum
    // @0x48BEA8: после 999 нумерация идёт «вторым кругом» с префиксом "999^".
    if (n <= kMaxFlow)
        return FormatFlowNumber(n < 0 ? 0 : n);
    int r = n % kMaxFlow;
    if (r == 0)
        r = kMaxFlow;
    return OverflowPrefix() + FormatFlowNumber(r);
}

int KSaveFile::FlowNumberFromName(const QString &fileName)
{
    // Базовое имя без расширения ("001.jpg" → "001", "999^001.mp4" → "999^001").
    const QString base = QFileInfo(fileName).completeBaseName();

    const int mark = base.indexOf(QChar(kOverflowMark));
    if (mark >= 0) {
        // Форма переполнения: "<999>^<NNN>" → линейный номер 999 + NNN.
        bool okHi = false, okLo = false;
        const int hi = base.left(mark).toInt(&okHi);
        const int lo = base.mid(mark + 1).toInt(&okLo);
        if (!okHi || !okLo || hi != kMaxFlow || lo < 1 || lo > kMaxFlow)
            return -1;
        return kMaxFlow + lo;
    }

    bool ok = false;
    const int v = base.toInt(&ok);
    return (ok && v >= 0) ? v : -1;
}

bool KSaveFile::CheckIsFileNumberUseUp(const QString &fileName)
{
    // Реф. @0x6a92c8: сравнение с литералом "999^999" (.rodata @0x8695d0).
    return QFileInfo(fileName).completeBaseName() == UseUpFlowName();
}

QString KSaveFile::MakeFileName(int flow, bool video)
{
    return MakeFlowName(flow) + "." + (video ? VideoExt() : ImageExt());
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
    return next > kMaxFlowOverall ? kMaxFlowOverall : next;
}
