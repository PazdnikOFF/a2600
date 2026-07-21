#pragma once

#include <QWidget>
#include <QMap>
#include <QString>

class QLineEdit;
class QComboBox;
class QPushButton;
class KPatientDateEdit;

// Панель поиска очереди DICOM (реф. KDicomQueueSearch @ctor 0x808f40, base QWidget, setupUi
// @0x80b198). UI-порт full-fidelity (реальный KPatientDateEdit). Близнец KExamListSearch, НО:
// нет поля врача/KMemComboBox; ДОБАВЛЕН DICOM-комбо типа сервиса cmb_msgtype (CommandType:
// MPPS/Storage/Commitment); статус → CommandStatus; диапазон дат по LastUpdateDate. Наружу —
// QueryItems({"Where": <готовый SQL WHERE>}), фрагменты склеены " and " (DB-seam, запрос в
// KDicomQueueViewUi). Поля: PatientID / PatientName / Type / Status + дата-диапазон.
class KDicomQueueSearch : public QWidget
{
    Q_OBJECT
public:
    explicit KDicomQueueSearch(QWidget *parent = nullptr);

    void MoveFocusToFirstWidget();

signals:
    void QueryItems(const QMap<QString, QString> &conditions);   // {"Where": clause}
    void SigToExitSearchItem();
    void SigToFocusOutWidgetSearch();

private slots:
    void SlotToStartSearch();       // реф. @0x809028: собрать WHERE, emit QueryItems
    void SlotToResetSearchData();
    void OnStartDateChanged(const QDate &d);
    void OnEndDateChanged(const QDate &d);

private:
    void setupUi();

    QLineEdit *m_editPatientId = nullptr;
    QLineEdit *m_editName = nullptr;
    QComboBox *m_cmbMsgType = nullptr;
    QComboBox *m_cmbStatus = nullptr;
    KPatientDateEdit *m_dateStart = nullptr;
    KPatientDateEdit *m_dateEnd = nullptr;
    QPushButton *m_btnSearch = nullptr;
    QPushButton *m_btnReset = nullptr;
};
