#pragma once

#include <QComboBox>
#include <QStringList>
#include <functional>

// Комбо быстрого ввода с MRU (реф. KQuickInputComboBox @ctor 0x5a91e0, base QComboBox).
// СИБЛИНГ KMemComboBox, но ПРОЩЕ и самодостаточнее: нативный дропдаун поверх
// KQuickInputModel (не кастомный find-попап). Используется в редактировании отчёта для
// повторного ввода частых фраз. UI-порт РЕАЛЬНОГО кастом-виджета.
//
// Отличия от KMemComboBox: модель показывается КАК нативный дропдаун (setModel); есть
// явный Save() — MRU-вставка текущего текста с дедупом и таймстампом; сигнал показа попапа.
//
// DEVICE-STUB: KQuickInputModel — DB-бэкенд (тот же зашифрованный SQLite). В порте
// уплощён на нативную item-модель QComboBox + инъектируемая начальная загрузка
// SetLoadProvider(fn). Save() ведёт MRU in-memory (дедуп + вставка в начало).
class KQuickInputComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit KQuickInputComboBox(QWidget *parent = nullptr);

    // Реф. Init @0x5a9278: new KQuickInputModel + LoadData(table,field) + setModel.
    // В порте: загрузка элементов через провайдер (DEVICE-STUB), заполнение комбо.
    void Init(const QString &tableName, const QString &field, int flag = 0);
    void SetLoadProvider(std::function<QStringList(const QString &table, const QString &field)> fn);

    // Реф. Save @0x5a9358: закоммитить currentText в MRU. Дедуп: если такой элемент уже
    // есть — вернуть -1 (ничего не делать); иначе вставить (с таймстампом) в начало и
    // вернуть индекс.
    int Save();

signals:
    void SigIsShowPopup(bool shown);   // реф. showPopup emit

private slots:
    void OnCurrentIndexChanged(int index);   // лениво подключается в showPopup

public:
    void showPopup() override;   // реф. @0x5a9730
    void hidePopup() override;

private:
    QString m_tableName;
    QString m_field;
    bool m_flag = false;   // +0x38
    bool m_idxConnected = false;
    std::function<QStringList(const QString &, const QString &)> m_loadProvider;
};
