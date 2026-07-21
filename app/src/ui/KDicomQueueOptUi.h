#pragma once

#include <QWidget>

class QPushButton;

// Панель действий очереди DICOM (реф. KDicomQueueOptUi @ctor 0x806890, base QWidget). UI-порт.
// СИБЛИНГ KExamListOptUi/KPatientListOptUi: та же структура (фикс 270×385, QVBoxLayout
// spacing 13, margins 30/12/19/-1, кнопки 212×46 + нижний спейсер, Tab-циклинг,
// MoveFocusToFirstWidget, SigToFocusOutCurrentOpt), но ТРИ кнопки: refresh/resend/delete.
// Реф. clicked → слоты KDicomQueueViewUi — ЗАМЕНЕНО на 3 своих сигнала (VIEW-seam).
class KDicomQueueOptUi : public QWidget
{
    Q_OBJECT
public:
    explicit KDicomQueueOptUi(QWidget *parent = nullptr);

    void MoveFocusToFirstWidget();   // реф.: фокус на btn_refresh

signals:
    void sigRefresh();   // btn_refresh → OnReFreshCurPageData
    void sigResend();    // btn_resend → OnResendSelectedData
    void sigDelete();    // btn_delete → OpenDeleteDialog
    void SigToFocusOutCurrentOpt();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void setupUi();

    QPushButton *m_btnRefresh = nullptr;
    QPushButton *m_btnResend = nullptr;
    QPushButton *m_btnDelete = nullptr;
};
