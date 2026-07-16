#include "report/KTitleTableBlock.h"

KTitleTableBlock::KTitleTableBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew)
    : KTableBlock(item, dataNew)   // база: InitItemConfig + CalcTableSize
{
    // Реф. ctor только форвардит в базу и подменяет vptr — доп. инициализации нет.
}

QSize KTitleTableBlock::Size() const
{
    // реф.: if (ShowTitle()) width+1 (лишняя строка под заголовок), height как есть.
    // ShowTitle() всегда true, но ветку сохраняем 1:1 с бинарником.
    if (ShowTitle())
        return QSize(KTableBlock::Size().width() + 1, KTableBlock::Size().height());
    return KTableBlock::Size();
}

bool KTitleTableBlock::GetTemplateItemForCell(int row, int col, KReportTemplateItem *&out) const
{
    // row 0 = заголовок; данные ниже → базовый lookup с (row-1). adj всегда 1.
    const int adj = ShowTitle() ? 1 : 0;
    return KTableBlock::GetTemplateItemForCell(row - adj, col, out);
}

QString KTitleTableBlock::Title() const
{
    // реф. — fromLatin1 от c-строки узла (БЕЗ null-guard).
    return QString::fromLatin1(GetTemplateItem()->m_strTitle.c_str());
}

void KTitleTableBlock::SetTitle(const QString &title)
{
    // реф. — пишет UTF-8-байты в m_strTitle узла. АСИММЕТРИЯ с Title() (fromLatin1):
    // round-trip корректен только для ASCII — воспроизводим как есть.
    GetTemplateItem()->m_strTitle = title.toUtf8().toStdString();
}
