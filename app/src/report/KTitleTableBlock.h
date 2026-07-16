#pragma once

#include "report/KTableBlock.h"

#include <QString>
#include <QSize>

// Модель табличного блока отчёта С ЗАГОЛОВКОМ (реф. KTitleTableBlock, X-2600).
// ТОНКИЙ поведенческий наследник KTableBlock (typeinfo __si_class_type_info → база
// KTableBlock; sizeof 0x78 — НИ ОДНОГО нового поля). «Заголовок» хранится не в блоке,
// а в узле шаблона: KReportTemplateItem::m_strTitle.
//
// Переопределяет ровно два virtual базы (Size/GetTemplateItemForCell) — резервирует
// первую строку сетки под заголовок; ShowTitle() форсирует true (невиртуально скрывает
// базовый). Рендер (KTemplateEditDocument::InsertTitleTable) — не наш.
class KTitleTableBlock : public KTableBlock
{
public:
    KTitleTableBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew);

    // Сетка = базовая + 1 строка под заголовок (width = строки+1, height = столбцы).
    QSize Size() const override;
    // row 0 — строка заголовка; данные сдвинуты вниз: делегирует базе с (row-1).
    bool GetTemplateItemForCell(int row, int col, KReportTemplateItem *&out) const override;

    // Невиртуальный, скрывает KTableBlock::ShowTitle — у таблицы-с-заголовком ВСЕГДА true.
    bool ShowTitle() const { return true; }

    QString Title() const;                 // item->m_strTitle (fromLatin1 — sic, см. .cpp)
    void SetTitle(const QString &title);   // item->m_strTitle = title.toUtf8() (асимметрия!)
};
