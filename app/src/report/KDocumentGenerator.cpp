#include "report/KDocumentGenerator.h"
#include "report/KReportTemplate.h"
#include "report/KReportDataSource.h"

#include <QTextDocument>   // Qt::escape → Qt5: QString::toHtmlEscaped

QString KDocumentGenerator::Generate(const QVector<ReportItem> &items,
                                     const KReportDataSource &ds) const
{
    QString body;
    for (const ReportItem &it : items)
        renderItem(it, ds, body, 0);
    return QStringLiteral("<html><body>\n") + body + QStringLiteral("</body></html>\n");
}

void KDocumentGenerator::renderItem(const ReportItem &it, const KReportDataSource &ds,
                                    QString &out, int depth) const
{
    const QString pad(depth * 2, ' ');

    if (it.isImage()) {
        // RT_IMAGE_BLOCK: путь снимка из DataSrc.
        const QString path = ds.GetValue(it.dataSource(), it.dataField());
        if (!path.isEmpty())
            out += pad + QString("<img class=\"rt-image\" src=\"%1\"/>\n")
                             .arg(path.toHtmlEscaped());
        return;
    }
    if (it.isText()) {
        // RT_TEXT_BLOCK: подпись (если ShowTitle) + значение.
        const QString val = ds.GetValue(it.dataSource(), it.dataField());
        if (val.isEmpty() && !it.showTitle)
            return;                         // пустое поле без заголовка не выводим
        out += pad + "<div class=\"rt-text\">";
        if (it.showTitle && !it.title.isEmpty())
            out += QString("<span class=\"rt-title\">%1</span>: ")
                       .arg(it.title.toHtmlEscaped());
        out += val.toHtmlEscaped() + "</div>\n";
        return;
    }

    // Блоки-контейнеры (TITLE_TABLE/TABLE): заголовок секции + рекурсия по детям.
    out += pad + QString("<div class=\"rt-block\" data-name=\"%1\" data-col=\"%2\">\n")
                     .arg(it.name.toHtmlEscaped()).arg(it.column);
    if (it.showTitle && !it.title.isEmpty())
        out += pad + QString("  <div class=\"rt-section-title\">%1</div>\n")
                         .arg(it.title.toHtmlEscaped());
    for (const ReportItem &c : it.children)
        renderItem(c, ds, out, depth + 1);
    out += pad + "</div>\n";
}
