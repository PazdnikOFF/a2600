#pragma once

#include <QString>
#include <QSet>
#include <QVector>

struct ReportItem;
class KReportDataSource;

// Параметры отображения элементов отчёта (реф. KReportDisplayParam, X-2600).
// Держит множество «валидных» (отображаемых) элементов и решает, показывать ли
// элемент шаблона. Валидность выводится из наличия данных в источнике
// (реф. UpdateTemplateDisplayParam + IsItemValid/AppendValidItem/SetRefValidItems/Reset).
//
// Модель: имя элемента шаблона (ReportItem::name) валидно, если у него есть
// непустое связанное значение в KReportDataSource (текст/таблица — по DataSrc;
// изображение — по пути снимка), либо есть хотя бы один валидный потомок.
// Опциональное «эталонное» множество (SetRefValidItems) ограничивает, какие имена
// вообще допустимы (реф.: AppendValidItem гейтит по ref-множеству).
class KReportDisplayParam
{
public:
    void Reset();                                          // очистить валидные

    // Эталонное множество допустимых имён (реф. SetRefValidItems). Пустое = без ограничения.
    void SetRefValidItems(const QSet<QString> &ref) { ref_ = ref; }

    // Добавить элемент в валидные (реф. AppendValidItem). Если задано ref-множество и
    // имя в него не входит — игнорируется. true, если добавлен/уже присутствует.
    bool AppendValidItem(const QString &name);

    bool IsItemValid(const QString &name) const;          // реф. IsItemValid
    int  ValidCount() const { return valid_.size(); }
    QSet<QString> ValidItems() const { return valid_; }

    // Пройти дерево шаблона + источник данных, пометить валидные элементы
    // (реф. UpdateTemplateDisplayParam). Возвращает число валидных.
    int UpdateTemplateDisplayParam(const QVector<ReportItem> &items,
                                   const KReportDataSource &ds);

private:
    // Рекурсивная проверка одного элемента: валиден ли он сам или его поддерево.
    bool markItem(const ReportItem &it, const KReportDataSource &ds);
    QSet<QString> valid_;
    QSet<QString> ref_;
};
