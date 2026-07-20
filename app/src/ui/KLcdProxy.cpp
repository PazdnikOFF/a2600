#include "ui/KLcdProxy.h"

#include <QStringList>

#include "ctrl/KColdLightConfig.h"
#include "endo/KEndoScope.h"
#include "kernel/KSystemLog.h"
#include "sys/KProjectSet.h"
#include "sys/KSystemStatus.h"
#include "sys/KSystemSet.h"
#include "sys/KUserOsdSet.h"
#include "ui/KUiMsgProxy.h"
#include "video/KVideoParam.h"
#include "video/KVideoSet.h"

// Таблица диспетчеризации сгенерирована из бинарника.
#include "ui/KLcdActTable.h"

namespace {
// Реф. список форматов даты (порядок важен: индекс = значение ключа 0x102).
const char *const kDateFormats[4] = {
    "yyyy/MM/dd", "MM/dd/yyyy", "dd/MM/yyyy", "yyyy-MM-dd",
};
}

KLcdProxy::KLcdProxy() : m_pUiMsgProxy(GetKUiMsgProxy()) {}

KLcdProxy *Get_KLcdProxy()
{
    static KLcdProxy inst;   // реф.: ленивый new в глобальную g_pobjKLcdProxy
    return &inst;
}

// --- диспетчер --------------------------------------------------------------

int KLcdProxy::KeyEventAct(quint16 usKey, quint8 ucAct, int nParam)
{
    LogPrintfEx(false, "[APP][D]: ",
                "HMI: key event, key=0x%02x, event=0x%02x, param=%d.\n",
                int(usKey), int(ucAct), nParam);
    // ⚠️ КВИРК: расширение знака смотрит ТОЛЬКО НА БИТ 7 и игнорирует биты
    // 8..31 — поэтому 0x180 превращается в 0x80. Латентный баг вендора,
    // воспроизводится как есть.
    if (nParam & 0x80)
        nParam -= 256;

    // ⚠️ Граница ЗАШИТА (0x52), а не sizeof/24. Совпадение по ПАРЕ (key, act),
    // причём act сравнивается как ОДИН БАЙТ.
    for (int i = 0; i < 0x52; ++i) {
        if (m_lcdActTab[i].usKey == usKey
            && (quint8)(m_lcdActTab[i].usAct) == ucAct) {
            return (this->*(m_lcdActTab[i].pfnAct))(nParam);
        }
    }
    return 2;   // ⚠️ промах: 2, без лога и без обработчика по умолчанию
}

// --- предикаты (оба имени вводят в заблуждение) -----------------------------

void KLcdProxy::CheckIsLightAdjustEnable()
{
    // ⚠️ Это НЕ предикат: только уведомление. Тип эндоскопа 13 → сигнал.
    if (GetEndoScope()->GetEndoType() == 13)
        m_pUiMsgProxy->SigLightAdjustDisabel();
}

int KLcdProxy::GetDateFormateIndex()
{
    const QString cur = KSystemSet::GetInstance().DateFormat();
    for (int i = 0; i < 4; ++i)
        if (cur == QLatin1String(kDateFormats[i]))
            return i;
    return 0;   // ⚠️ промах молча выглядит как yyyy/MM/dd
}

int KLcdProxy::SetTimeFormat(int nParam)
{
    m_lastAct = QStringLiteral("SetTimeFormat");
    // ⚠️ ИМЯ ЛЖЁТ: метод ставит формат ДАТЫ и никогда не трогает 12/24-часовой
    // режим.
    // ⚠️ ОТСТУПЛЕНИЕ: в реф. индекс НЕ ПРОВЕРЯЕТСЯ (d->begin + arg), поэтому
    // любое значение вне 0..3 читает указатель за границей QList и приводит
    // к порче кучи. Воспроизводить это нельзя — ставим границу.
    if (nParam < 0 || nParam > 3)
        return 1;
    KSystemSet::GetInstance().SetDateFormat(QString::fromLatin1(kDateFormats[nParam]));
    KSystemStatus::GetInstance().ChangeSystemSet(0x102, nParam);
    return 1;
}

int KLcdProxy::SetButtonFunc(int nKeyId, int nIndex)
{
    // Реф.: KVideoSet::SetButtonFuncId + KSystemStatus::ChangeUserSet, затем
    // Rigid/Flex-подсказка по признаку жёсткого эндоскопа.
    KSystemStatus::GetInstance().ChangeUserSet(nKeyId, nIndex);
    // Реф.: гид гейтится по ViewType (SS[+0x14] == 1 → жёсткий эндоскоп).
    if (KSystemStatus::GetInstance().ViewType() == 1)
        m_pUiMsgProxy->UpdateRigidEndoBtnGuide();
    else
        m_pUiMsgProxy->UpdateFlexEndoBtnGuide();
    return 1;
}

int KLcdProxy::LeftFootSwitchAct(int nParam)
{
    m_lastAct = QStringLiteral("LeftFootSwitchAct");
    // ⚠️ Полный no-op при ViewType == 1.
    if (KSystemStatus::GetInstance().ViewType() == 1)
        return 1;
    LogPrintf("[APP][I]: ", "footkey: left.");
    m_pUiMsgProxy->SendToMainCtrl(43, 0x211, nParam);
    return 1;
}

int KLcdProxy::RightFootSwitchAct(int nParam)
{
    m_lastAct = QStringLiteral("RightFootSwitchAct");
    if (KSystemStatus::GetInstance().ViewType() == 1)
        return 1;
    LogPrintf("[APP][I]: ", "footkey: right.");
    m_pUiMsgProxy->SendToMainCtrl(43, 0x212, nParam);
    return 1;
}

int KLcdProxy::LockScreenAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("LockScreenAct");
    // Реф.: ТОЛЬКО показ сообщения, больше ничего.
    m_pUiMsgProxy->DisplayMsg(QObject::tr("TR_PUTTouchscreen"));
    return 1;
}


// --- единственный источник значений для панели ------------------------------

int KLcdProxy::GetKeyStatus(quint16 usKey)
{
    KSystemStatus &st = KSystemStatus::GetInstance();
    KVideoSet &vs = KVideoSet::Instance();

    switch (usKey) {
    // ⚠️ ГРУППА 1-BASED: панель ждёт уровень С ЕДИНИЦЫ, а конфиг хранит с нуля.
    case 0x04: return vs.GetImgEnhLevel() + 1;
    case 0x05: return vs.GetColEnhLevel() + 1;
    case 0x06: return vs.GetZoomLevel() + 1;
    case 0x17: return vs.GetContrastLevel() + 1;

    case 0x02: return st.CHbStatus() == 1 ? 1 : 0;
    case 0x18: return vs.GetOperationMode();
    case 0x24: return st.RecordStatus() != 0 ? 1 : 0;

    case 0x102: return GetDateFormateIndex();

    // ⚠️ СЕНТИНЕЛ: при ненулевой форме углов реф. отдаёт 0xFFFE. В панель
    // уходит именно 0xFFFE (значение усекается в quint16), а НЕ -2.
    case 0x104:
        return GetEndoScope()->GetEndoShapeType() != 0 ? 0xFFFE : 0;

    case 0x101:
        // ⚠️ Реф. отдаёт ЦЕЛОЕ-энум (0=Chinese, 1=English, ... 7=Polish)
        // с клампом в 1, а не строку.
        return KSystemSet::GetInstance().GetSystemLanguage();

    // Назначения функций кнопкам ручки эндоскопа (плоский индекс 0..5,
    // = button*2 + press, A=0/B=1/M=2, Long=0/Short=1).
    // ⚠️ ИСПРАВЛЕНО: результат ОБЯЗАН проходить через FunctionIdToIndex —
    // панель ждёт ПОЗИЦИЮ в списке функций, а не сам ID. В первой версии
    // возвращался сырой ID, это была ошибка.
    case 0x213: case 0x214: case 0x215: case 0x216:
        return KUserOsdSet::FunctionIdToIndex(
            KUserOsdSet::GetInstance().GetButtonFunctionId(int(usKey) - 0x213));
    // ⚠️ 0x217 (индекс 4 = ButtomM/LongPress) НАМЕРЕННО пропущен: этот слот
    // конфига write-only и читателем osd.ini никогда не заполняется.
    case 0x217: return 0;
    case 0x218:
        return KUserOsdSet::FunctionIdToIndex(
            KUserOsdSet::GetInstance().GetButtonFunctionId(5));

    // ⚠️ КЛЮЧИ, ИСТОЧНИКОВ КОТОРЫХ У НАС ПОКА НЕТ, — падают в дефолт 0
    // (совпадает с поведением реф. для неизвестного ключа, но НЕ с его
    // поведением для этих ключей). Нужны, чтобы закрыть:
    //   0x00, 0x01 — KSystemStatus[+0x2C]/[+0x28] (Agc и соседнее поле;
    //                семантика второго НЕ УСТАНОВЛЕНА реверсом);
    //   0x07, 0x08 — KVideoParam Iris/Tone (у нас IrisMode есть в KVideoParam,
    //                но KVideoSet-обёртки нет);
    //   0x19       — DemoireStatus (есть в KVideoParam, нет в KVideoSet);
    //   0x103      — KColdLightConfig::GetUserVLSConfig;
    //   0x105/0x106 — GetResolutionType / GetSaveVideoSplit;
    //   0x201-0x203 — режим-зависимые Color R/B/C;
    //   0x204-0x20C — KVideoSet::Get{ImgEnh,ColorEnh,Zoom}Value(0..2);
    //   0x20D-0x212 — KUserSet::Get{Button,Pedal}FunctionId + FunctionIdToIndex.
    // См. PROGRESS §10: список методов, которые надо добавить.
    default: return 0;   // реф.: дефолт 0, проверок диапазона нет
    }
}

// --- продюсеры наборов ------------------------------------------------------

namespace {
inline void app(QList<_KeyVlaue> &l, KLcdProxy *p, quint16 k)
{
    l.append(_KeyVlaue{ k, quint16(p->GetKeyStatus(k)) });
}
}

QList<_KeyVlaue> KLcdProxy::GetSoftEndoMainViewParams()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x04); app(l, this, 0x05); app(l, this, 0x07); app(l, this, 0x01);
    if (KProjectSet::GetInstance().IsZoomEnable())        app(l, this, 0x06);
    if (KProjectSet::GetInstance().IsVideoRecordEnable()) app(l, this, 0x24);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetSoftEndoUserSet1Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x17); app(l, this, 0x00);
    if (KProjectSet::GetInstance().IsChbEnable()) app(l, this, 0x02);
    app(l, this, 0x201); app(l, this, 0x202); app(l, this, 0x203);
    app(l, this, 0x204); app(l, this, 0x205); app(l, this, 0x206);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetSoftEndoUserSet2Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x17); app(l, this, 0x00);
    if (KProjectSet::GetInstance().IsChbEnable()) app(l, this, 0x02);
    app(l, this, 0x207); app(l, this, 0x208); app(l, this, 0x209);
    if (KProjectSet::GetInstance().IsZoomEnable()) {
        app(l, this, 0x20A); app(l, this, 0x20B); app(l, this, 0x20C);
    }
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetSoftEndoUserSet3Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x17); app(l, this, 0x00);
    if (KProjectSet::GetInstance().IsChbEnable()) app(l, this, 0x02);
    app(l, this, 0x20D); app(l, this, 0x20E); app(l, this, 0x20F);
    app(l, this, 0x210); app(l, this, 0x211); app(l, this, 0x212);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetSoftEndoSystemSet1Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x101); app(l, this, 0x102); app(l, this, 0x103);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetHardEndoSystemSet1Params()
{
    // Реф.: чистый tail-call алиас — отдельная точка входа с тем же телом.
    return GetSoftEndoSystemSet1Params();
}

QList<_KeyVlaue> KLcdProxy::GetSoftEndoSystemSet2Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x104); app(l, this, 0x105); app(l, this, 0x106);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetHardEndMainViewParams()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x04); app(l, this, 0x17);
    if (KProjectSet::GetInstance().IsZoomEnable()) app(l, this, 0x06);
    app(l, this, 0x07); app(l, this, 0x18); app(l, this, 0x19); app(l, this, 0x01);
    if (KProjectSet::GetInstance().IsVideoRecordEnable()) app(l, this, 0x24);
    // ⚠️ КВИРК: ключ 0x0F добавляется НАПРЯМУЮ, В ОБХОД GetKeyStatus
    // (в switch'е его нет вовсе).
    if (KSystemStatus::GetInstance().PanelType() == 1)
        l.append(_KeyVlaue{ 0x0F, quint16(KSystemStatus::GetInstance().LightLevel()) });
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetHardEndoUserSet1Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x00);
    app(l, this, 0x201); app(l, this, 0x202); app(l, this, 0x203);
    app(l, this, 0x204); app(l, this, 0x205); app(l, this, 0x206);
    if (KProjectSet::GetInstance().IsZoomEnable()) {
        app(l, this, 0x20A); app(l, this, 0x20B); app(l, this, 0x20C);
    }
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetHardEndoUserSet2Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x00);
    app(l, this, 0x213); app(l, this, 0x214); app(l, this, 0x215); app(l, this, 0x216);
    // ⚠️ 0x217 ПРОПУЩЕН намеренно — сразу 0x218.
    app(l, this, 0x218);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetHardEndoSystemSet2Params()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x105); app(l, this, 0x106);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetHardEndoOperationModeParams()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x18);
    return l;
}

QList<_KeyVlaue> KLcdProxy::GetPanelLedSet()
{
    QList<_KeyVlaue> l;
    app(l, this, 0x08); app(l, this, 0x04); app(l, this, 0x05); app(l, this, 0x06);
    app(l, this, 0x07); app(l, this, 0x00); app(l, this, 0x01); app(l, this, 0x02);
    return l;
}

int KLcdProxy::AddLampLevel(int nParam)
{
    m_lastAct = QStringLiteral("AddLampLevel");
    KSystemStatus &st = KSystemStatus::GetInstance();
    // ⚠️ КВИРК: Add и Sub БАЙТ-ИДЕНТИЧНЫ (различаются только текстом лога);
    // ни прибавления, ни вычитания нет — param трактуется как АБСОЛЮТНЫЙ уровень.
    // Гейт SS[+0x44]: 1 -> яркость изображения, иначе уровень света.
    if (st.LowLight() == 1)
        st.SetImageBrightness(nParam);
    else {
        CheckIsLightAdjustEnable();
        st.SetLightLevel(nParam);
    }
    return 1;
}

int KLcdProxy::CancelResetUserParam(int nParam)
{
    m_lastAct = QStringLiteral("CancelResetUserParam");
    m_pUiMsgProxy->SendToMainCtrl(37, 0);
    return 1;
}

int KLcdProxy::CloseLamp(int nParam)
{
    m_lastAct = QStringLiteral("CloseLamp");
    (void)nParam;
    CheckIsLightAdjustEnable();
    KSystemStatus::GetInstance().SetLampStatus(0);
    return 1;
}

int KLcdProxy::ConnectOrDisconnecEndo(int nParam)
{
    m_lastAct = QStringLiteral("ConnectOrDisconnecEndo");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(1);
    return 1;
}

int KLcdProxy::FileView(int nParam)
{
    m_lastAct = QStringLiteral("FileView");
    m_pUiMsgProxy->FileView(nParam);
    return 1;
}

int KLcdProxy::GainSwitchLongPressAct(int nParam)
{
    m_lastAct = QStringLiteral("GainSwitchLongPressAct");
    (void)nParam;
    if (KSystemStatus::GetInstance().PanelType() == 0)
        m_pUiMsgProxy->SendToMainCtrl(7);
    return 1;
}

int KLcdProxy::GainSwitchShortPressAct(int nParam)
{
    m_lastAct = QStringLiteral("GainSwitchShortPressAct");
    (void)nParam;
    if (KSystemStatus::GetInstance().PanelType() == 1)
        m_pUiMsgProxy->SendToMainCtrl(7);
    return 1;
}

int KLcdProxy::OpenLamp(int nParam)
{
    m_lastAct = QStringLiteral("OpenLamp");
    (void)nParam;
    KSystemStatus &st = KSystemStatus::GetInstance();
    st.SetLampStatus(1);
    // if SS[+0x40]==1 (LowLight) -> SetLowLightMode(0) — поле lowLight есть.
    if (st.LowLight() == 1)
        st.SetLowLightMode(0);
    return 1;
}

int KLcdProxy::OpenOrCloseEliminatemoire(int nParam)
{
    m_lastAct = QStringLiteral("OpenOrCloseEliminatemoire");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(30);
    return 1;
}

int KLcdProxy::OpenOrCloseEndoInfoDialog(int nParam)
{
    m_lastAct = QStringLiteral("OpenOrCloseEndoInfoDialog");
    (void)nParam;
    m_pUiMsgProxy->PanelKeyEndoInfo();
    return 1;
}

int KLcdProxy::OpenOrCloseVersionDialog(int nParam)
{
    m_lastAct = QStringLiteral("OpenOrCloseVersionDialog");
    (void)nParam;
    m_pUiMsgProxy->PanelKeyVersion();
    return 1;
}

int KLcdProxy::PowerOff(int nParam)
{
    m_lastAct = QStringLiteral("PowerOff");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(0);
    return 1;
}

int KLcdProxy::RemoteButton0Act(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButton0Act");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x20d, nParam);
    return 1;
}

int KLcdProxy::RemoteButton1Act(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButton1Act");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x20e, nParam);
    return 1;
}

int KLcdProxy::RemoteButton2Act(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButton2Act");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x20f, nParam);
    return 1;
}

int KLcdProxy::RemoteButton3Act(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButton3Act");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x210, nParam);
    return 1;
}

int KLcdProxy::RemoteButtonALongPressAct(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButtonALongPressAct");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x213, nParam);
    return 1;
}

int KLcdProxy::RemoteButtonAShortPressAct(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButtonAShortPressAct");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x214, nParam);
    return 1;
}

int KLcdProxy::RemoteButtonBLongPressAct(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButtonBLongPressAct");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x215, nParam);
    return 1;
}

int KLcdProxy::RemoteButtonBShortPressAct(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButtonBShortPressAct");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x216, nParam);
    return 1;
}

int KLcdProxy::RemoteButtonMLongPressAct(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButtonMLongPressAct");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x217, nParam);
    return 1;
}

int KLcdProxy::RemoteButtonMShortPressAct(int nParam)
{
    m_lastAct = QStringLiteral("RemoteButtonMShortPressAct");
    // Реф.: 3-арг (43, код, param) + лог.
    m_pUiMsgProxy->SendToMainCtrl(43, 0x218, nParam);
    return 1;
}

int KLcdProxy::ResetUserParam(int nParam)
{
    m_lastAct = QStringLiteral("ResetUserParam");
    m_pUiMsgProxy->SendToMainCtrl(37, 1);
    return 1;
}

int KLcdProxy::SaveImage(int nParam)
{
    m_lastAct = QStringLiteral("SaveImage");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(4);
    return 1;
}

int KLcdProxy::SetColorBValue(int nParam)
{
    m_lastAct = QStringLiteral("SetColorBValue");
    m_pUiMsgProxy->SendToMainCtrl(26, 1, nParam);
    return 1;
}

int KLcdProxy::SetColorCValue(int nParam)
{
    m_lastAct = QStringLiteral("SetColorCValue");
    m_pUiMsgProxy->SendToMainCtrl(26, 2, nParam);
    return 1;
}

int KLcdProxy::SetColorEnhL1Value(int nParam)
{
    m_lastAct = QStringLiteral("SetColorEnhL1Value");
    m_pUiMsgProxy->SendToMainCtrl(25, 0, nParam);
    return 1;
}

int KLcdProxy::SetColorEnhL2Value(int nParam)
{
    m_lastAct = QStringLiteral("SetColorEnhL2Value");
    m_pUiMsgProxy->SendToMainCtrl(25, 1, nParam);
    return 1;
}

int KLcdProxy::SetColorEnhL3Value(int nParam)
{
    m_lastAct = QStringLiteral("SetColorEnhL3Value");
    m_pUiMsgProxy->SendToMainCtrl(25, 2, nParam);
    return 1;
}

int KLcdProxy::SetColorRValue(int nParam)
{
    m_lastAct = QStringLiteral("SetColorRValue");
    m_pUiMsgProxy->SendToMainCtrl(26, 0, nParam);
    return 1;
}

int KLcdProxy::SetConnerShape(int nParam)
{
    m_lastAct = QStringLiteral("SetConnerShape");
    // Реф.: KSystemSet::SetCornerShape(param) — метода пока нет; шлём только сигнал.
    KSystemStatus::GetInstance().ChangeSystemSet(0x104, nParam);
    return 1;
}

int KLcdProxy::SetImgEnhL1Value(int nParam)
{
    m_lastAct = QStringLiteral("SetImgEnhL1Value");
    m_pUiMsgProxy->SendToMainCtrl(24, 0, nParam);
    return 1;
}

int KLcdProxy::SetImgEnhL2Value(int nParam)
{
    m_lastAct = QStringLiteral("SetImgEnhL2Value");
    m_pUiMsgProxy->SendToMainCtrl(24, 1, nParam);
    return 1;
}

int KLcdProxy::SetImgEnhL3Value(int nParam)
{
    m_lastAct = QStringLiteral("SetImgEnhL3Value");
    m_pUiMsgProxy->SendToMainCtrl(24, 2, nParam);
    return 1;
}

int KLcdProxy::SetLanguage(int nParam)
{
    m_lastAct = QStringLiteral("SetLanguage");
    KSystemSet::GetInstance().SetLanguage(QString::number(nParam));
    KSystemStatus::GetInstance().ChangeSystemSet(0x101, nParam);
    return 1;
}

int KLcdProxy::SetResolution(int nParam)
{
    m_lastAct = QStringLiteral("SetResolution");
    KSystemStatus::GetInstance().ChangeSystemSet(0x105, nParam);
    return 1;
}

int KLcdProxy::SetVLSGroup(int nParam)
{
    m_lastAct = QStringLiteral("SetVLSGroup");
    // Реф.: KColdLightConfig::SetUserVLSConfig(param, ViewType) + ChangeSystemSet.
    KSystemStatus::GetInstance().ChangeSystemSet(0x103, nParam);
    return 1;
}

int KLcdProxy::SetVideoSplit(int nParam)
{
    m_lastAct = QStringLiteral("SetVideoSplit");
    KSystemStatus::GetInstance().ChangeSystemSet(0x106, nParam);
    return 1;
}

int KLcdProxy::SetZoomL1Value(int nParam)
{
    m_lastAct = QStringLiteral("SetZoomL1Value");
    m_pUiMsgProxy->SendToMainCtrl(27, 0, nParam);
    return 1;
}

int KLcdProxy::SetZoomL2Value(int nParam)
{
    m_lastAct = QStringLiteral("SetZoomL2Value");
    m_pUiMsgProxy->SendToMainCtrl(27, 1, nParam);
    return 1;
}

int KLcdProxy::SetZoomL3Value(int nParam)
{
    m_lastAct = QStringLiteral("SetZoomL3Value");
    m_pUiMsgProxy->SendToMainCtrl(27, 2, nParam);
    return 1;
}

int KLcdProxy::StartOrStopVideoRecord(int nParam)
{
    m_lastAct = QStringLiteral("StartOrStopVideoRecord");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(2);
    return 1;
}

int KLcdProxy::StartTrans(int nParam)
{
    m_lastAct = QStringLiteral("StartTrans");
    (void)nParam;
    // Реф.: ТОЛЬКО лог, без dispatch.
    LogPrintf("[APP][I]: ", "StartTrans");
    return 1;
}

int KLcdProxy::StartWhiteBalance(int nParam)
{
    m_lastAct = QStringLiteral("StartWhiteBalance");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(13);
    return 1;
}

int KLcdProxy::SubLampLevel(int nParam)
{
    m_lastAct = QStringLiteral("SubLampLevel");
    KSystemStatus &st = KSystemStatus::GetInstance();
    // ⚠️ КВИРК: Add и Sub БАЙТ-ИДЕНТИЧНЫ (различаются только текстом лога);
    // ни прибавления, ни вычитания нет — param трактуется как АБСОЛЮТНЫЙ уровень.
    // Гейт SS[+0x44]: 1 -> яркость изображения, иначе уровень света.
    if (st.LowLight() == 1)
        st.SetImageBrightness(nParam);
    else {
        CheckIsLightAdjustEnable();
        st.SetLightLevel(nParam);
    }
    return 1;
}

int KLcdProxy::SwitchAirPumpLevel(int nParam)
{
    m_lastAct = QStringLiteral("SwitchAirPumpLevel");
    // Реф.: ТОЛЬКО лог, без dispatch.
    LogPrintf("[APP][I]: ", "Switch air pump level: %d", nParam);
    return 1;
}

int KLcdProxy::SwitchAirPumpStatus(int nParam)
{
    m_lastAct = QStringLiteral("SwitchAirPumpStatus");
    m_pUiMsgProxy->SendToMainCtrl(48, nParam);
    return 1;
}

int KLcdProxy::SwitchAutoLightAdjust(int nParam)
{
    m_lastAct = QStringLiteral("SwitchAutoLightAdjust");
    (void)nParam;
    KSystemStatus::GetInstance().SetDimmingType(1);
    return 1;
}

int KLcdProxy::SwitchChbStatus(int nParam)
{
    m_lastAct = QStringLiteral("SwitchChbStatus");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(10);
    return 1;
}

int KLcdProxy::SwitchColorEnhLevel(int nParam)
{
    m_lastAct = QStringLiteral("SwitchColorEnhLevel");
    m_pUiMsgProxy->SendToMainCtrl(11, nParam);
    return 1;
}

int KLcdProxy::SwitchContrastType(int nParam)
{
    m_lastAct = QStringLiteral("SwitchContrastType");
    m_pUiMsgProxy->SendToMainCtrl(14, nParam);
    return 1;
}

int KLcdProxy::SwitchFreezeStatus(int nParam)
{
    m_lastAct = QStringLiteral("SwitchFreezeStatus");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(9);
    return 1;
}

int KLcdProxy::SwitchImgEnhLevel(int nParam)
{
    m_lastAct = QStringLiteral("SwitchImgEnhLevel");
    m_pUiMsgProxy->SendToMainCtrl(12, nParam);
    return 1;
}

int KLcdProxy::SwitchIrisType(int nParam)
{
    m_lastAct = QStringLiteral("SwitchIrisType");
    m_pUiMsgProxy->SendToMainCtrl(28, nParam);
    return 1;
}

int KLcdProxy::SwitchManuLightAdjust(int nParam)
{
    m_lastAct = QStringLiteral("SwitchManuLightAdjust");
    (void)nParam;
    KSystemStatus &st = KSystemStatus::GetInstance();
    st.SetDimmingType(0);
    st.SetLowLightMode(0);
    return 1;
}

int KLcdProxy::SwitchOperatingPattern(int nParam)
{
    m_lastAct = QStringLiteral("SwitchOperatingPattern");
    m_pUiMsgProxy->SendToMainCtrl(29, nParam);
    return 1;
}

int KLcdProxy::SwitchToneMode(int nParam)
{
    m_lastAct = QStringLiteral("SwitchToneMode");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(20, 255);
    return 1;
}

int KLcdProxy::SwitchVlsMode(int nParam)
{
    m_lastAct = QStringLiteral("SwitchVlsMode");
    // Реф.: пишет KSystemStatus, dispatch НЕТ.
    KSystemStatus::GetInstance().SetVlsMode(nParam);
    return 1;
}

int KLcdProxy::SwitchWindow(int nParam)
{
    m_lastAct = QStringLiteral("SwitchWindow");
    m_pUiMsgProxy->SendToMainCtrl(45, nParam);
    return 1;
}

int KLcdProxy::SwitchZoomLevel(int nParam)
{
    m_lastAct = QStringLiteral("SwitchZoomLevel");
    m_pUiMsgProxy->SendToMainCtrl(8, nParam);
    return 1;
}

int KLcdProxy::ToneAdd(int nParam)
{
    m_lastAct = QStringLiteral("ToneAdd");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(21);
    return 1;
}

int KLcdProxy::ToneSub(int nParam)
{
    m_lastAct = QStringLiteral("ToneSub");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(22);
    return 1;
}

int KLcdProxy::UnmountUsbDevice(int nParam)
{
    m_lastAct = QStringLiteral("UnmountUsbDevice");
    (void)nParam;
    m_pUiMsgProxy->SendToMainCtrl(39);
    return 1;
}

int KLcdProxy::SetEndoButton0Funcindex(int nParam)
{
    m_lastAct = QStringLiteral("SetEndoButton0Funcindex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x20D, nParam);
}

int KLcdProxy::SetEndoButton1Funcindex(int nParam)
{
    m_lastAct = QStringLiteral("SetEndoButton1Funcindex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x20E, nParam);
}

int KLcdProxy::SetEndoButton2Funcindex(int nParam)
{
    m_lastAct = QStringLiteral("SetEndoButton2Funcindex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x20F, nParam);
}

int KLcdProxy::SetEndoButton3Funcindex(int nParam)
{
    m_lastAct = QStringLiteral("SetEndoButton3Funcindex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x210, nParam);
}

int KLcdProxy::SetLeftFootSwitchFuncindex(int nParam)
{
    m_lastAct = QStringLiteral("SetLeftFootSwitchFuncindex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x211, nParam);
}

int KLcdProxy::SetRightFootSwitchFuncindex(int nParam)
{
    m_lastAct = QStringLiteral("SetRightFootSwitchFuncindex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x212, nParam);
}

int KLcdProxy::SetCameraButtomALongPressFuncIndex(int nParam)
{
    m_lastAct = QStringLiteral("SetCameraButtomALongPressFuncIndex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x213, nParam);
}

int KLcdProxy::SetCameraButtomAShortPressFuncIndex(int nParam)
{
    m_lastAct = QStringLiteral("SetCameraButtomAShortPressFuncIndex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x214, nParam);
}

int KLcdProxy::SetCameraButtomBLongPressFuncIndex(int nParam)
{
    m_lastAct = QStringLiteral("SetCameraButtomBLongPressFuncIndex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x215, nParam);
}

int KLcdProxy::SetCameraButtomBShortPressFuncIndex(int nParam)
{
    m_lastAct = QStringLiteral("SetCameraButtomBShortPressFuncIndex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x216, nParam);
}

int KLcdProxy::SetCameraButtomMLongPressFuncIndex(int nParam)
{
    m_lastAct = QStringLiteral("SetCameraButtomMLongPressFuncIndex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x217, nParam);
}

int KLcdProxy::SetCameraButtomMShortPressFuncIndex(int nParam)
{
    m_lastAct = QStringLiteral("SetCameraButtomMShortPressFuncIndex");
    // Реф.: все 12 — 12-байтовые tail-jump в общий SetButtonFunc(keyId, index).
    return SetButtonFunc(0x218, nParam);
}
