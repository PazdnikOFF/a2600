#pragma once

#include <QWidget>

class QLineEdit;
class QComboBox;
class QDateEdit;

// Панель поиска по списку пациентов/worklist (реф. KPatientListSearch : QWidget — НЕ
// KDialog!, ctor @0x7d5cf0, Ui_KPatientListSearch::setupUi @0x7d99e8). UI-порт.
// Встраиваемый горизонтальный виджет-строка (реф. 1591×170, абсолютная геометрия) над
// таблицей пациентов; наружу отдаёт сигналы (QueryItems/SigToExit/SigToFocusOut).
// Порт как самостоятельный QWidget (базовый класс совпадает); строка перестроена в
// сетку 4×2 ячеек «подпись/поле» + колонка кнопок Search/Reset.
// 8 критериев: ID, имя, пол+возраст(диапазон), пункт обследования, направитель,
// статус, дата-от, дата-до (диапазон дат с «--»).
//
// Кастом KPatientDateEdit→QDateEdit. Reset (SlotToResetSearchData) — чистый UI,
// реализован. Search (SlotToStartSearch) собирает map и эмитит QueryItems — сам запрос
// в БД downstream (device), у нас no-op. cmb_examitem (GetEndoClassToQstring) — device.
class KPatientListSearch : public QWidget
{
    Q_OBJECT
public:
    explicit KPatientListSearch(QWidget *parent = nullptr);

private slots:
    void resetSearchData();   // реф. SlotToResetSearchData

private:
    void setupUi();

    QList<QLineEdit *> m_edits;
    QList<QComboBox *> m_combos;
    QList<QDateEdit *> m_dates;
};
