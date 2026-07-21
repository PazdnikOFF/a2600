#pragma once

#include <QWidget>

class QPushButton;

// Панель действий списка пациентов (реф. KPatientListOptUi @ctor 0x7d2a78, base QWidget,
// setupUi @0x7d2da0). UI-порт. СИБЛИНГ KExamListOptUi: та же структура (фикс 270×385,
// QVBoxLayout spacing 13, 6 кнопок 212×46 + нижний спейсер, Tab-циклинг, MoveFocusToFirstWidget,
// SigToFocusOutCurrentOpt), но ДРУГИЕ margins (30/12/19/-1) и набор кнопок. Реф. clicked
// каждой → слот KPatientListViewUi — ЗАМЕНЕНО на 6 своих сигналов (VIEW-seam).
class KPatientListOptUi : public QWidget
{
    Q_OBJECT
public:
    explicit KPatientListOptUi(QWidget *parent = nullptr);

    void MoveFocusToFirstWidget();   // реф.: фокус на btn_check

signals:
    void sigExam();       // btn_check → OnTriggerExam
    void sigAdd();        // btn_add → OpenAddDialog
    void sigEdit();       // btn_edit → OpenEditDialog
    void sigDownload();   // btn_download → OnDownloadWorklist
    void sigDelete();     // btn_delete → OpenDeleteDialog
    void sigSetup();      // btn_set → OpenSetupDialog
    void SigToFocusOutCurrentOpt();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void setupUi();

    QPushButton *m_btnCheck = nullptr;
    QPushButton *m_btnAdd = nullptr;
    QPushButton *m_btnEdit = nullptr;
    QPushButton *m_btnDownload = nullptr;
    QPushButton *m_btnDelete = nullptr;
    QPushButton *m_btnSet = nullptr;
};
