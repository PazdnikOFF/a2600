#pragma once

#include "ui/KDialog.h"
#include "kernel/KMessage.h"

class QLabel;
class QProgressBar;

// Диалог прогресса операций с данными (реф. KProgressDlg : QDialog, ctor @0x451870,
// Ui_KProgressDlg::setupUi @0x452528). UI-порт. Реф.-база — QDialog (не KDialog), но чистый
// drop-in над KDialog. 512×278. Разблокирует KDataOprEventDeal (см. §10 блокеры).
// Фрейм umessage_frame_back (StyledPanel/Raised) → vbox: label_Text(TR_Prpng, AlignLeft|
// Bottom) + [lblCurrentProgress(TR_CFProgress) + progCurrentFile + lblTotalProgress
// (TR_TProgress) + progTotal] + спейсер + btn_Cancel(TR_Ccl, minW150, центр). Прогресс-бары
// с border-стилем, value 0. Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: ToDlgMsgDispatcher-сигналы (SigUpdate*Progress/Label/Hide/
// ShowResultMsgBox/OneExamRecordUpdateFinish — кросс-поточные апдейты) — заглушки.
// Публичный API (Set*Progress/Set*Label/SetTitle/DoModal/IsCancel) — портируемый.
// btn_Cancel→close.
class KProgressDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KProgressDlg(QWidget *parent = nullptr);

    // Реф. слоты @0x450830..0x451f20, подключаемые в InitConnect @0x451320 ко всем восьми
    // сигналам ToDlgMsgDispatcher().
    void OnSigUpdateTitleTotalProgress(const QString &title, int totalProgress);
    void OnSigUpdateHide();
    void OnSigShowResultMsgBox(const QString &msg);
    void OnSigUpdateTotalProgress(int progress);
    void OnSigUpdateSubProgress(int progress);
    void OnSigUpdateTotalLabel(const QString &text);
    void OnSigUpdateSubLabel(const QString &text);
    void OnSigOneExamRecordUpdateFinish(const KMessage &msg);

    // Для проверок (в реф. — поля Ui-структуры).
    QProgressBar *TotalBar() const { return m_progTotal; }
    QProgressBar *SubBar() const { return m_progCurrent; }
    QLabel *TotalLabel() const { return m_lblTotal; }
    QLabel *SubLabel() const { return m_lblCurrent; }

private:
    void setupUi();
    void InitConnect();   // реф. @0x451320

    QLabel *m_lblText = nullptr;
    QLabel *m_lblCurrent = nullptr;
    QLabel *m_lblTotal = nullptr;
    QProgressBar *m_progCurrent = nullptr;
    QProgressBar *m_progTotal = nullptr;
};
