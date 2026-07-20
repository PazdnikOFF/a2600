#pragma once

#include <QString>
#include <QVector>

struct ReportItem;
class KReportDataSource;

// !!! ЭТО НАШ КЛАСС, А НЕ РЕФЕРЕНСНЫЙ. Имени KReportHtmlGenerator в прошивке НЕТ. !!!
//
// Раньше класс назывался KDocumentGenerator и занимал реф.-имя незаконно: его API
// (Generate/renderItem поверх ReportItem+KReportDataSource) выдуман целиком, 0%
// совпадения с одноимённым классом прошивки. Настоящий KDocumentGenerator @0x53bf38 —
// это 34 метода поверх KReportTemplateDataNew, которые строят QTextDocument через
// KRTCreatorContext (см. docs/PROGRESS.md §10). Имя освобождено под него.
//
// Назначение: обходит дерево шаблона (ReportItem), разрешает привязки DataSrc через
// KReportDataSource и формирует семантический HTML — упрощённый эквивалент рендера
// оригинала (тот пишет в QTextDocument/PDF), удобный для проверки на десктопе.
//
// Правила блоков:
//   RT_TITLE_TABLE_BLOCK / RT_TABLE_BLOCK — секция/таблица (рекурсия по детям);
//   RT_TEXT_BLOCK  — подпись(Title)+значение из DataSrc;
//   RT_IMAGE_BLOCK — изображение по пути из DataSrc.
class KReportHtmlGenerator
{
public:
    // Собрать HTML отчёта из корневых элементов шаблона и источника данных.
    QString Generate(const QVector<ReportItem> &items, const KReportDataSource &ds) const;

private:
    void renderItem(const ReportItem &it, const KReportDataSource &ds,
                    QString &out, int depth) const;
};
