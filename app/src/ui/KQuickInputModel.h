#pragma once

#include <QAbstractItemModel>

#include <memory>
#include <string>
#include <vector>

#include "db/KComboBoxItem.h"

// Стратегия доступа к одному словарю быстрого ввода (реф. общий базовый класс четырёх
// KQuickInputData*; ИМЯ БАЗОВОГО КЛАССА НЕ ВОССТАНОВЛЕНО — отдельного символа нет,
// восстановлена только vtable из 4 слотов: D1, D0, GetData@+0x10, SetData@+0x18,
// в базе — чисто виртуальные).
class KQuickInputDataBase
{
public:
    virtual ~KQuickInputDataBase() = default;
    // Реф. слот +0x10: очистить out и заполнить его первыми `count` записями словаря
    // (через <Handler>::GetSortedData — «time DESC, count DESC»).
    virtual void GetData(std::vector<KComboBoxItem> &out, int count) = 0;
    // Реф. слот +0x18: «отметить использование» существующей записи. 0 — успех, иначе ошибка.
    // ВАЖНО: Add НЕ вызывается — если записи нет, реф. возвращает ошибку (сверено на
    // KQuickInputDataDoctor::SetData @0x5aa4e8 и KQuickInputDataPatientID::SetData @0x5ab038).
    virtual int  SetData(const KComboBoxItem &item) = 0;
};

// Четыре конкретные стратегии (адреса — GetData/SetData из vtable).
// ⚠️ Построчно сверены: GetData у PatientName (@0x5ac520), SetData у Doctor (@0x5aa4e8) и
// PatientID (@0x5ab038). Остальные восстановлены ПО СТРУКТУРНОЙ АНАЛОГИИ (vtable-слоты
// найдены, тела не трассировались построчно) — помечено здесь и в PROGRESS.md.
class KQuickInputDataPatientName : public KQuickInputDataBase   // GetData 0x5ac520 / SetData 0x5ab728
{
public:
    void GetData(std::vector<KComboBoxItem> &out, int count) override;
    int  SetData(const KComboBoxItem &item) override;
};

class KQuickInputDataPatientID : public KQuickInputDataBase     // GetData 0x5ac8a8 / SetData 0x5ab038
{
public:
    void GetData(std::vector<KComboBoxItem> &out, int count) override;
    int  SetData(const KComboBoxItem &item) override;
};

class KQuickInputDataDoctor : public KQuickInputDataBase        // GetData 0x5ac110 / SetData 0x5aa4e8
{
public:
    void GetData(std::vector<KComboBoxItem> &out, int count) override;
    int  SetData(const KComboBoxItem &item) override;
};

class KQuickInputDataApplicant : public KQuickInputDataBase     // GetData 0x5acc30 / SetData 0x5aaad8
{
public:
    void GetData(std::vector<KComboBoxItem> &out, int count) override;
    int  SetData(const KComboBoxItem &item) override;
};

// Модель списка быстрого ввода (реф. KQuickInputModel @ctor 0x5a9928, sizeof 0x40).
// Наследник QAbstractItemModel; ПЛОСКИЙ односколоночный список, READ-ONLY: переопределены
// только index/parent/rowCount/columnCount/data (@0x5aa020/0x5aa060/0x5aa070/0x5aa098/
// 0x5aa0a0). headerData/flags/setData НЕ переопределены — запись идёт мимо модели, через
// SaveData().
//
// Поля реф.: +0x10 vector<KComboBoxItem>, +0x28/+0x30 shared_ptr<стратегия>,
// +0x38 int m_limit (в ctor = 10, безусловно перезаписывается 3-м аргументом LoadData).
class KQuickInputModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit KQuickInputModel(QObject *parent = nullptr);

    // Реф. @0x5a9ab8. Селекторы (СТРОКИ-КЛЮЧИ, а НЕ имена SQL-таблиц — те без префикса
    // `tb_`, см. KQuickInputDbTableHandler):
    //   ("tb_QuickInputPatient" @0x863598, "PatientName" @0x83e490) → KQuickInputDataPatientName
    //   ("tb_QuickInputPatient",           "PatientID"   @0x83eb60) → KQuickInputDataPatientID
    //   ("tb_QuickInputApplicant" @0x863528, любое поле)            → KQuickInputDataApplicant
    //   ("tb_QuickInputDoctor"    @0x863560, любое поле)            → KQuickInputDataDoctor
    // Ни одно не совпало → стратегия НЕ меняется. Затем, если стратегия есть,
    // вызывается GetData(m_items, m_limit).
    void LoadData(const std::string &table, const std::string &field, int limit);

    // Реф. @0x5a9e68: нет стратегии → -1; SetData != 0 → -1; иначе перезагрузка списка
    // (GetData) и возврат её результата.
    int  SaveData(const KComboBoxItem &item);

    // Реф. @0x5a9ec8: emit dataChanged(index(0,0), index(rowCount-1,0)) — «мягкий» reset
    // без begin/endResetModel.
    void AllDataChanged();

    const std::vector<KComboBoxItem> &Items() const { return m_items; }
    int Limit() const { return m_limit; }

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    std::vector<KComboBoxItem>           m_items;      // +0x10
    std::shared_ptr<KQuickInputDataBase> m_strategy;   // +0x28/+0x30
    int                                  m_limit = 10; // +0x38 (дефолт из ctor)
};
