#pragma once

#include <QWidget>

class QPushButton;

// Панель действий списка осмотров (реф. KExamListOptUi @ctor 0x7f1630, base QWidget,
// setupUi @0x7f1b20). UI-порт. Фикс 270×385, QVBoxLayout (spacing 13, margins 30/12/19/9),
// 6 кнопок 212×46 + нижний вертикальный спейсер. Реф. ctor берёт KExamListViewUi* как
// back-pointer и коннектит clicked каждой кнопки к слоту view — в порте это ЗАМЕНЕНО на 6
// собственных сигналов (VIEW/DEVICE-seam). Собственный сигнал реф. SigToFocusOutCurrentOpt
// (Tab с последней кнопки). Tab-циклинг — через eventFilter.
class KExamListOptUi : public QWidget
{
    Q_OBJECT
public:
    explicit KExamListOptUi(QWidget *parent = nullptr);

    void MoveFocusToFirstWidget();   // реф.: фокус на btn_report

signals:
    // Реф. clicked→слоты KExamListViewUi (VIEW-seam), в порте — свои сигналы:
    void sigReport();      // → OpenReportEdit
    void sigExport();      // → ReadyToExportCheckedItem
    void sigUpload();      // → ReadyToUploadCheckedItem
    void sigCancelExam();  // → OpenCancelExamDialog
    void sigDelete();      // → ReadyToDeleteCheckedItem
    void sigSetup();       // → OpenSetupDialog
    void SigToFocusOutCurrentOpt();   // реф. — Tab уводит фокус с панели

protected:
    bool eventFilter(QObject *o, QEvent *e) override;   // Tab-циклинг

private:
    void setupUi();

    QPushButton *m_btnReport = nullptr;
    QPushButton *m_btnExport = nullptr;
    QPushButton *m_btnUpload = nullptr;
    QPushButton *m_btnCancleCheck = nullptr;
    QPushButton *m_btnDelete = nullptr;
    QPushButton *m_btnSet = nullptr;
};
