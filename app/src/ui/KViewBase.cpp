#include "KViewBase.h"

#include "KStopWatch.h"
#include "KUiMsgProxy.h"
#include "KViewHardEndo.h"
#include "sys/KSystemStatus.h"
#include "video/KViewSoftEndo.h"

#include <QApplication>
#include <QCursor>
#include <QTimer>

namespace {
// Реф. код сообщения главному контроллеру из отложенной лямбды ctor'а.
const int kMsgViewReady = 47;      // реф. SendToMainCtrl(47) через 3000 мс
// Реф. код при нажатии Rst на НЕактивном окне.
const int kMsgPanelReset = 0x25;   // 37
// Реф. порог переключения курсора в 500-мс таймере.
const int kBlinkThreshold = 5;
}   // namespace

KViewBase::KViewBase(QWidget *parent)
    : QDialog(parent)
{
    // Реф. ctor @0x45d460. Разметки нет — дети позиционируются абсолютно.
    setObjectName(QStringLiteral("KViewBase"));
    setWindowTitle(QStringLiteral("Dialog"));   // реф. translate("KViewBase","Dialog")

    m_softEndo = new KViewSoftEndo(this);
    m_softEndo->setObjectName(QStringLiteral("frameSoftEndo"));
    m_hardEndo = new KViewHardEndo(this);
    m_hardEndo->setObjectName(QStringLiteral("frameHardEndo"));
    m_stopWatch = new KStopWatch(this);
    m_stopWatch->setObjectName(QStringLiteral("stopwatchframe"));

    // Реф. лямбда #1: 500-мс «мигающий курсор».
    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &KViewBase::BlinkCursor);
    m_blinkTimer->start(500);

    InitUiType();
    InitConnect();

    // Реф. лямбда #2: через 3 с — уведомление главному контроллеру.
    QTimer::singleShot(3000, this, [] {
        if (KUiMsgProxy *p = GetKUiMsgProxy())
            p->SendToMainCtrl(kMsgViewReady);
    });

    // Реф. дальше: если !KSystemSet::IsAutoLogin() → OpenLoginDlg(this, ""). Автологин —
    // device-настройка, поэтому вход отдан хуку и здесь не дёргается автоматически.
}

KViewBase::~KViewBase() = default;

bool KViewBase::IsSoftEndoView() const
{
    // Реф. @0x45c4b0: KSystemStatus поле +20 == 0. У нас это ViewType() (0 = мягкий).
    return KSystemStatus::GetInstance().ViewType() == 0;
}

void KViewBase::InitUiType()
{
    // Реф. @0x45d280: по IsSoftEndoView показывает нужный фрейм и прячет второй,
    // затем растягивает себя и активный фрейм под UI-разрешение (KSystemSet::
    // GetUIResolution — device-настройка; здесь берём собственный текущий размер).
    const bool soft = IsSoftEndoView();
    m_softEndo->setVisible(soft);
    m_hardEndo->setVisible(!soft);

    QWidget *active = soft ? static_cast<QWidget *>(m_softEndo)
                           : static_cast<QWidget *>(m_hardEndo);
    active->move(0, 0);                 // реф. активный фрейм всегда в (0,0)
    active->resize(size());
    InitStopWatch();                    // реф.: вызывается в конце InitUiType
}

void KViewBase::InitStopWatch()
{
    // Реф. @0x45d220: X берётся из KDisplayOption::Get{Soft,Hard}EndoViewConf(),
    // Y — ФИКСИРОВАННО 66. Конфиги — device-часть; X оставляем текущим, Y ставим реф.
    if (m_stopWatch)
        m_stopWatch->move(m_stopWatch->x(), 66);
}

void KViewBase::InitConnect()
{
    // Реф. @0x45bf00 — 10 связей. Восемь идут от device-синглтонов (KUiMsgProxy /
    // GetSystemStatus / GetEndoScope); наш KUiMsgProxy смоделирован методами-
    // регистраторами, а не сигналами, поэтому здесь их нет — соответствующие слоты
    // публичны и вызываются напрямую. Реально подключаем единственную виджет-связь.
    if (m_stopWatch)
        connect(m_stopWatch, &KStopWatch::StopWatchStateChanged,
                this, &KViewBase::StopWatchStateChanged);
}

void KViewBase::BlinkCursor()
{
    // Реф. лямбда #1 @0x45bb60.
    QWidget *targets[3] = {this, m_hardEndo, m_softEndo};
    if (!isActiveWindow()) {
        for (QWidget *w : targets)
            if (w)
                w->setCursor(Qt::ArrowCursor);
        m_blinkCount = 0;
        return;
    }
    const Qt::CursorShape shape = (m_blinkCount >= kBlinkThreshold) ? Qt::BlankCursor
                                                                    : Qt::ArrowCursor;
    for (QWidget *w : targets)
        if (w)
            w->setCursor(shape);
    ++m_blinkCount;
}

void KViewBase::Warn(const QString &title, const QString &msg)
{
    // Реф. KMessageBox::warning(...). Offscreen модальное окно проверить нельзя,
    // поэтому текст запоминаем — на нём и держится self-test.
    m_lastTitle = title;
    m_lastMessage = msg;
}

void KViewBase::ShowExportErrorMsg()
{
    // Реф. @0x45bdd0: один ключ, показывается ОБОИМ фреймам.
    const QString msg = QStringLiteral("TR_ESDIUAEAbnormal");
    if (m_hardEndo)
        m_hardEndo->DisplayMsg(msg);
    if (m_softEndo)
        m_softEndo->DisplayMsg(msg);
    m_lastMessage = msg;
}

void KViewBase::SystemStatusChangeAct(int what, int)
{
    if (what == 0)          // реф. @0x45d450: только what==0 что-то делает
        InitUiType();
}

void KViewBase::PanelKeyRstAct(int pressed)
{
    // Реф. @0x45c140 — это НЕ таблица кодов клавиш, а press/release одной кнопки Rst.
    if (pressed == 0) {                       // отпускание
        if (m_rstDlgShown) {
            m_lastMessage.clear();            // реф. KMessageBox::clearMessageBox()
            m_rstDlgShown = false;
        }
        return;
    }
    if (!isActiveWindow()) {                  // нажатие при неактивном окне
        m_lastMessage.clear();
        m_rstDlgShown = false;
        if (KUiMsgProxy *p = GetKUiMsgProxy())
            p->SendToMainCtrl(kMsgPanelReset);
        return;
    }
    // Нажатие при активном окне — предупреждение Yes/No.
    m_rstDlgShown = true;
    const bool alt = m_rstAltText ? m_rstAltText() : false;
    const QString msg = QStringLiteral("TR_ESDIUAEAbnormal\n")
                        + (alt ? QStringLiteral("TR_LPRTCSPRTCancel1")
                               : QStringLiteral("TR_LPRTCSPRTCancel"));
    Warn(QStringLiteral("TR_Wng"), msg);
    // ⚠️ Реф.: результат Yes/No НИ НА ЧТО не влияет — обе ветки лишь освобождают
    // временные строки. Какое действие должно следовать за «Yes» — НЕ ВОССТАНОВЛЕНО.
}

void KViewBase::PanelKeyOpenVersionDlgAct()
{
    if (!isActiveWindow())      // реф. @0x45c448
        return;
    if (m_openVersion)
        m_openVersion();
}

void KViewBase::PanelKeyOpenEndoInfoDlgAct()
{
    if (!isActiveWindow())      // реф. @0x45c470
        return;
    // Реф.: KSystemStatus+20 != 0 → OpenKScopeInfoEdit(), иначе OpenKCameraInfoEdit(false).
    const bool isScope = !IsSoftEndoView();
    if (m_openEndoInfo)
        m_openEndoInfo(isScope);
}

void KViewBase::ShowEndoControlInfo(int code)
{
    // Реф. @0x45c4d0: ОДНОРАЗОВАЯ защёлка (байт @a4c000+957) — второй раз ничего не покажет.
    if (m_endoInfoShown)
        return;

    const QString prefix = QStringLiteral("TR_TYFUOProduct");
    QString msg;
    if (code == 1 || code == 3) {
        msg = prefix + QStringLiteral("TR_ENCPUTEOCTAOASales");
    } else if (code == 4) {
        msg = prefix + QStringLiteral("TR_TEEPCTAOAIYWTCUIt");
    } else if (code == 5) {
        // Реф. читает EEPROM: дату производства ("yyyyMMdd") и остаток использований,
        // сравнивает с порогами 14 и 30.
        int days = 0, uses = 0;
        if (m_endoEeprom)
            m_endoEeprom(days, uses);
        if (days > 14 || uses > 30)
            msg = QStringLiteral("TR_Rmnng:") + QString::number(days);
        else
            msg = QStringLiteral("TR_NORUses:") + QString::number(uses);
    } else {
        msg = prefix + QStringLiteral("TR_TEWBEPCTAOAIYWTCUIt");
    }
    Warn(QStringLiteral("TR_Wng"), msg);
    m_endoInfoShown = true;   // реф. защёлка ставится в 1
}

void KViewBase::CheckMachineControl()
{
    // Реф. @0x45cb90: гейт по KControlProc::IsStartTimeMc() и isActiveWindow().
    if (!isActiveWindow())
        return;
    int days = 0;
    const bool enabled = m_machineControl ? m_machineControl(days) : false;
    if (!enabled)
        return;

    const QString prefix = QStringLiteral("TR_TYFUOProduct");
    if (days == 0) {
        Warn(QStringLiteral("TR_Wng"), prefix + QStringLiteral("TR_TIPHEPCTAOAIYWTCUIt"));
    } else if (days >= 1 && days <= 14) {
        Warn(QStringLiteral("TR_Wng"),
             prefix + QStringLiteral("TR_EDate:") + QStringLiteral("TR_AOnly")
                 + QString::number(days) + QStringLiteral("TR_DALeft")
                 + QStringLiteral("TR_TIPWBEPCTAOAIYWTCUIt")
                 + QStringLiteral("TR_TIPHEPCTAOAIYWTCUIt"));
    }
    // days > 14 — реф. молчит.
}

void KViewBase::StopWatchStateChanged(int status)
{
    // Реф. @0x45d180: Run(2) → показать секундомер + SetStopWatchStatus(1);
    // Stop(0) → спрятать + SetStopWatchStatus(0); Pause(1) — видимость не трогается.
    // ⚠️ Какая именно виртуальная функция стоит на vtbl+0x68 — НЕ ВОССТАНОВЛЕНО;
    // по сигнатуре (один bool-аргумент) это почти наверняка setVisible.
    if (status == KStopWatch::Run) {
        if (m_stopWatch)
            m_stopWatch->setVisible(true);
        KSystemStatus::GetInstance().SetStopWatchStatus(1);
    } else if (status == KStopWatch::Stop) {
        if (m_stopWatch)
            m_stopWatch->setVisible(false);
        KSystemStatus::GetInstance().SetStopWatchStatus(0);
    }
    // Реф. в любом случае форвардит обоим фреймам — у нас у них этого слота нет
    // (device-OSD), поэтому форвард опущен и помечен здесь.
}

QWidget *KViewBase::GetActiveWidget()
{
    // Реф. @0x45daa0: модальное → активное окно → первый подходящий top-level.
    if (QWidget *w = QApplication::activeModalWidget())
        return w;
    if (QWidget *w = QApplication::activeWindow())
        return w;
    const auto tops = QApplication::topLevelWidgets();
    return tops.isEmpty() ? nullptr : tops.first();
}

bool KViewBase::BackMainView()
{
    // Реф. @0x45db60: если активен уже сам KViewBase — true; иначе закрыть активный
    // виджет и вернуть false (реф. дополнительно пишет в лог имя класса).
    QWidget *w = GetActiveWidget();
    if (!w)
        return true;
    if (QString::fromLatin1(w->metaObject()->className()) == QLatin1String("KViewBase"))
        return true;
    w->close();
    return false;
}

void KViewBase::ForceLogoutSystem()
{
    // Реф. @0x45dce8: если что-то закрывали — снова показать логин с пояснением.
    if (BackMainView())
        return;
    if (m_openLogin)
        m_openLogin(QStringLiteral("TR_YHNOFALTAHBALOut"));
}

void KViewBase::SetOpenLoginDlgHook(std::function<void(const QString &)> fn)
{ m_openLogin = std::move(fn); }
void KViewBase::SetOpenVersionDlgHook(std::function<void()> fn)
{ m_openVersion = std::move(fn); }
void KViewBase::SetOpenEndoInfoDlgHook(std::function<void(bool)> fn)
{ m_openEndoInfo = std::move(fn); }
void KViewBase::SetEndoEepromHook(std::function<void(int &, int &)> fn)
{ m_endoEeprom = std::move(fn); }
void KViewBase::SetMachineControlHook(std::function<bool(int &)> fn)
{ m_machineControl = std::move(fn); }
void KViewBase::SetRstAltTextHook(std::function<bool()> fn)
{ m_rstAltText = std::move(fn); }
