#pragma once

#include <QDialog>
#include <QString>

#include <functional>

class KStopWatch;
class KViewHardEndo;
class KViewSoftEndo;
class QTimer;

// Базовый экран просмотра (реф. KViewBase, ctor @0x45d460, dtor @0x45bb10) — общий контейнер
// для двух вьюверов (мягкий/жёсткий эндоскоп) и секундомера.
//
// Реф.-факты (сверены дизасмом):
// • Наследник **QDialog** (ctor зовёт QDialog(parent, flags=0); qt_metacast падает в
//   QDialog::qt_metacast). СИГНАЛОВ НЕТ — в qt_static_metacall @0x81bdf0 ровно 8 слотов
//   (0..7) и ни одного QMetaObject::activate.
// • Разметки НЕТ: три ребёнка создаются в ctor и позиционируются АБСОЛЮТНО
//   (setGeometry/resize/move), без QLayout:
//     KViewSoftEndo  objectName "frameSoftEndo"
//     KViewHardEndo  objectName "frameHardEndo"
//     KStopWatch     objectName "stopwatchframe"
//   Указатели лежат в отдельно выделенной 24-байтной структуре по this+48.
// • В ctor две лямбды:
//     #1 @0x45bb60 — QTimer(500 мс): «мигающий курсор». Пока окно активно, счётчик
//        (this+76) растёт и по порогу 5 курсор трёх виджетов переключается
//        Qt::ArrowCursor ⇄ Qt::BlankCursor; окно неактивно → всем Arrow и счётчик в 0.
//     #2 @0x45bd70 — QTimer::singleShot(3000): KUiMsgProxy::SendToMainCtrl(47).
// • IsSoftEndoView() @0x45c4b0 = (KSystemStatus поле +20) == 0 — у нас это ViewType().
//
// DEVICE-STUB-сеймы. Реф. InitConnect @0x45bf00 цепляет 10 связей, из которых 8 идут от
// device-синглтонов (KUiMsgProxy: CloseDesktop/PanelKeyRst/PanelKeyVersion/PanelKeyEndoInfo/
// CheckMachineControl/StopWatchStateChanged; GetSystemStatus: SystemStatusChange/FroceLogout
// [опечатка реф.]; GetEndoScope: UpdateEndoControlInfo). Наш KUiMsgProxy смоделирован
// методами-регистраторами, а не сигналами, поэтому здесь эти связи НЕ создаются — слоты
// сделаны публичными и вызываются напрямую (в превью/self-test). Реально подключается
// только KStopWatch::StopWatchStateChanged → наш слот.
// Открытие подчинённых диалогов (OpenLoginDlg/OpenVersionDlg/OpenKScopeInfoEdit/
// OpenKCameraInfoEdit) и чтение EEPROM эндоскопа вынесены за инъектируемые хуки.
class KViewBase : public QDialog
{
    Q_OBJECT
public:
    explicit KViewBase(QWidget *parent = nullptr);
    ~KViewBase() override;

    // Реф. @0x45c4b0. У нас поле +20 KSystemStatus — это ViewType() (0 = мягкий).
    bool IsSoftEndoView() const;

    void InitUiType();       // реф. @0x45d280
    void InitStopWatch();    // реф. @0x45d220
    void InitConnect();      // реф. @0x45bf00 (device-связи — см. комментарий класса)
    void ShowExportErrorMsg();   // реф. @0x45bdd0

    static QWidget *GetActiveWidget();   // реф. @0x45daa0
    bool BackMainView();                 // реф. @0x45db60

    KViewSoftEndo *SoftEndo() const { return m_softEndo; }
    KViewHardEndo *HardEndo() const { return m_hardEndo; }
    KStopWatch    *StopWatch() const { return m_stopWatch; }

    // ── DEVICE-STUB инъекции ──
    void SetOpenLoginDlgHook(std::function<void(const QString &)> fn);
    void SetOpenVersionDlgHook(std::function<void()> fn);
    // isScope: реф. выбирает OpenKScopeInfoEdit() при KSystemStatus+20 != 0,
    // иначе OpenKCameraInfoEdit(false).
    void SetOpenEndoInfoDlgHook(std::function<void(bool isScope)> fn);
    // Реф. ShowEndoControlInfo(5) читает KEndoScope::GetEepromData(): дату производства
    // ("yyyyMMdd") и остаток использований. Здесь — провайдер.
    void SetEndoEepromHook(std::function<void(int &daysSinceMade, int &remainUses)> fn);
    // Реф. CheckMachineControl берёт срок из KControlProc/KControlINI::ReadMcTime.
    void SetMachineControlHook(std::function<bool(int &daysLeft)> fn);
    // Реф. PanelKeyRstAct читает KSystemStatus поле **+16** — КАКОЕ это поле, НЕ
    // ВОССТАНОВЛЕНО. От него зависит лишь выбор одного из двух ключей перевода.
    void SetRstAltTextHook(std::function<bool()> fn);

    // Текст последнего показанного предупреждения — чтобы поведение было проверяемо
    // offscreen (реф. показывает KMessageBox::warning).
    QString LastMessage() const { return m_lastMessage; }
    QString LastMessageTitle() const { return m_lastTitle; }

public slots:
    // Реф. слоты в порядке индексов qt_static_metacall (0..7).
    void SystemStatusChangeAct(int what, int arg);        // 0 — реф. @0x45d450
    void PanelKeyRstAct(int pressed);                     // 1 — реф. @0x45c140
    void PanelKeyOpenVersionDlgAct();                     // 2 — реф. @0x45c448
    void PanelKeyOpenEndoInfoDlgAct();                    // 3 — реф. @0x45c470
    void ShowEndoControlInfo(int code);                   // 4 — реф. @0x45c4d0
    void CheckMachineControl();                           // 5 — реф. @0x45cb90
    void StopWatchStateChanged(int status);               // 6 — реф. @0x45d180
    void ForceLogoutSystem();                             // 7 — реф. @0x45dce8

private:
    void BlinkCursor();       // реф. лямбда #1 @0x45bb60
    void Warn(const QString &title, const QString &msg);   // реф. KMessageBox::warning

    KViewSoftEndo *m_softEndo = nullptr;    // this+48+0
    KViewHardEndo *m_hardEndo = nullptr;    // this+48+8
    KStopWatch    *m_stopWatch = nullptr;   // this+48+16
    QTimer        *m_blinkTimer = nullptr;  // this+80
    bool           m_rstDlgShown = false;   // this+72
    int            m_blinkCount = 0;        // this+76
    bool           m_endoInfoShown = false; // реф. одноразовая защёлка @a4c000+957

    QString m_lastMessage;
    QString m_lastTitle;

    std::function<void(const QString &)> m_openLogin;
    std::function<void()>                m_openVersion;
    std::function<void(bool)>            m_openEndoInfo;
    std::function<void(int &, int &)>    m_endoEeprom;
    std::function<bool(int &)>           m_machineControl;
    std::function<bool()>                m_rstAltText;
};
