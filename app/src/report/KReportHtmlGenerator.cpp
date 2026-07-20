#include "report/KReportHtmlGenerator.h"
#include "report/KReportTemplate.h"
#include "report/KReportDataSource.h"

#include <QTextDocument>   // Qt::escape → Qt5: QString::toHtmlEscaped
#include <QStringList>

QString KReportHtmlGenerator::Generate(const QVector<ReportItem> &items,
                                     const KReportDataSource &ds) const
{
    QString body;
    for (const ReportItem &it : items)
        renderItem(it, ds, body, 0);
    return QStringLiteral("<html><body>\n") + body + QStringLiteral("</body></html>\n");
}

void KReportHtmlGenerator::renderItem(const ReportItem &it, const KReportDataSource &ds,
                                    QString &out, int depth) const
{
    const QString pad(depth * 2, ' ');

    // Стиль из ItemConfig (реф. раскладка): выравнивание, ширина изображения, шрифт.
    auto styleAttr = [&](bool withWidth) -> QString {
        QStringList st;
        if (!it.alignH.isEmpty())
            st << "text-align:" + it.alignH.toLower();
        if (withWidth && it.imageWidth > 0)
            st << QString("width:%1px").arg(it.imageWidth);
        if (it.fontType == "ThirdTitle")
            st << "font-weight:bold";
        return st.isEmpty() ? QString() : QString(" style=\"%1\"").arg(st.join(';'));
    };

    if (it.isImage()) {
        // RT_IMAGE_BLOCK: путь снимка из DataSrc; ширина/выравнивание из ItemConfig.
        const QString path = ds.GetValue(it.dataSource(), it.dataField());
        if (!path.isEmpty())
            out += pad + QString("<img class=\"rt-image\" src=\"%1\"%2/>\n")
                             .arg(path.toHtmlEscaped(), styleAttr(true));
        return;
    }
    if (it.isText()) {
        // RT_TEXT_BLOCK: подпись (если ShowTitle) + значение.
        const QString val = ds.GetValue(it.dataSource(), it.dataField());
        if (val.isEmpty() && !it.showTitle)
            return;                         // пустое поле без заголовка не выводим
        out += pad + QString("<div class=\"rt-text\"%1>").arg(styleAttr(false));
        if (it.showTitle && !it.title.isEmpty())
            out += QString("<span class=\"rt-title\">%1</span>: ")
                       .arg(it.title.toHtmlEscaped());
        out += val.toHtmlEscaped() + "</div>\n";
        return;
    }

    // Блоки-контейнеры (TITLE_TABLE/TABLE): заголовок секции + рекурсия по детям.
    out += pad + QString("<div class=\"rt-block\" data-name=\"%1\" data-col=\"%2\"%3>\n")
                     .arg(it.name.toHtmlEscaped()).arg(it.column)
                     .arg(it.section.isEmpty() ? QString()
                          : QString(" data-section=\"%1\"").arg(it.section.toHtmlEscaped()));
    if (it.showTitle && !it.title.isEmpty())
        out += pad + QString("  <div class=\"rt-section-title\">%1</div>\n")
                         .arg(it.title.toHtmlEscaped());
    for (const ReportItem &c : it.children)
        renderItem(c, ds, out, depth + 1);
    out += pad + "</div>\n";
}
