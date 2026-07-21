#pragma once

#include <QWidget>
#include <QMap>
#include <QString>

class QLineEdit;
class QComboBox;
class QPushButton;
class KMemComboBox;
class KPatientDateEdit;

// Панель поиска списка осмотров (реф. KExamListSearch @ctor 0x7f42d8, base QWidget,
// setupUi @0x7f6a68). UI-порт НА ПОЛНОЙ ТОЧНОСТИ — использует РЕАЛЬНЫЕ KMemComboBox (врач) +
// KPatientDateEdit (диапазон дат), не подстановки. Сиблинг KPatientListSearch. Поля:
// PatientID / Name / Doctor(KMemComboBox, DrExamName) / Status(ReportStatus enum) +
// ExamDate-диапазон (2 KPatientDateEdit, кросс-ограничение from≤to). Наружу — сигнал
// QueryItems(map условий) (DB-seam: запрос выполняет вью). Реф. абсолютная геометрия; в
// порте — QGridLayout (как у сиблинга).
class KExamListSearch : public QWidget
{
    Q_OBJECT
public:
    explicit KExamListSearch(QWidget *parent = nullptr);

    void MoveFocusToFirstWidget();

signals:
    // Реф. QueryItems(std::map<string,string>) — DB-seam. В порте QMap.
    void QueryItems(const QMap<QString, QString> &conditions);
    void SigToFocusOutWidgetSearch();

private slots:
    void SlotToStartSearch();       // реф. @0x7f4788: собрать условия, emit QueryItems
    void SlotToResetSearchData();   // реф.: очистить поля
    void OnStartDateChanged(const QDate &d);
    void OnEndDateChanged(const QDate &d);

private:
    void setupUi();

    QLineEdit *m_editPatientId = nullptr;
    QLineEdit *m_editName = nullptr;
    KMemComboBox *m_cmbDoctor = nullptr;
    QComboBox *m_cmbStatus = nullptr;
    KPatientDateEdit *m_dateStart = nullptr;
    KPatientDateEdit *m_dateEnd = nullptr;
    QPushButton *m_btnSearch = nullptr;
    QPushButton *m_btnReset = nullptr;
};
