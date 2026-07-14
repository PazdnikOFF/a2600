#pragma once

#include <QString>
#include <QVector>

struct ReportItem;
class KReportDataSource;

// Генератор документа отчёта (реф. KDocumentGenerator, X-2600). Обходит дерево
// шаблона (ReportItem), разрешает привязки DataSrc через KReportDataSource и
// формирует представление отчёта. На устройстве оригинал рендерит в QTextDocument/
// PDF; здесь генерируется семантический HTML (эквивалент, проверяемо на десктопе).
//
// Правила блоков:
//   RT_TITLE_TABLE_BLOCK / RT_TABLE_BLOCK — секция/таблица (рекурсия по детям);
//   RT_TEXT_BLOCK  — подпись(Title)+значение из DataSrc;
//   RT_IMAGE_BLOCK — изображение по пути из DataSrc.
class KDocumentGenerator
{
public:
    // Собрать HTML отчёта из корневых элементов шаблона и источника данных.
    QString Generate(const QVector<ReportItem> &items, const KReportDataSource &ds) const;

private:
    void renderItem(const ReportItem &it, const KReportDataSource &ds,
                    QString &out, int depth) const;
};
