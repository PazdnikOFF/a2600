#pragma once

#include <QString>
#include <QVector>

// Парсер структуры DICOM-датасета (реф. чтение Mpps{Create,Set}DatasetFormat.xml, X-2600).
// Определяет дерево тегов, которое собирается в датасет MPPS-сообщения (в паре с
// KDicomFieldMap: формат задаёт СТРУКТУРУ, field-map заполняет ЗНАЧЕНИЯ из БД).
// Узлы XML:
//   <DcmItem name="DCM_..." />                — лист (одиночный тег);
//   <SequenceItem name="DCM_...">…</SequenceItem> — последовательность (вложенные узлы).
class KDicomDatasetFormat
{
public:
    struct Item {
        QString name;             // имя тега (DCM_...)
        bool    isSequence = false;
        QVector<Item> children;   // для последовательностей
    };

    bool Load(const QString &xmlPath);

    const QVector<Item> &Items() const { return items_; }  // корневые узлы
    int RootCount() const { return items_.size(); }
    // Полное число узлов (рекурсивно).
    int TotalCount() const;
    // Найти узел по имени (рекурсивно). found=nullptr допустим.
    Item FindItem(const QString &name, bool *found = nullptr) const;
    // Есть ли тег с таким именем (рекурсивно).
    bool Contains(const QString &name) const;

private:
    QVector<Item> items_;
    static int countRec(const QVector<Item> &v);
    static const Item *findRec(const QVector<Item> &v, const QString &name);
};
