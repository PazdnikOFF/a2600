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
    // Реф. смотрит признак жёсткого эндоскопа в KSystemStatus.
    if (KSystemStatus::GetInstance().VlsMode() == 1)
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

    // Назначения функций кнопкам ручки эндоскопа (плоский индекс 0..5,
    // = button*2 + press, A=0/B=1/M=2, Long=0/Short=1).
    case 0x213: case 0x214: case 0x215: case 0x216:
        return KUserOsdSet::GetInstance().GetButtonFunctionId(int(usKey) - 0x213);
    // ⚠️ 0x217 (индекс 4 = ButtomM/LongPress) НАМЕРЕННО пропущен: этот слот
    // конфига write-only и читателем osd.ini никогда не заполняется.
    case 0x217: return 0;
    case 0x218:
        return KUserOsdSet::GetInstance().GetButtonFunctionId(5);

    // ⚠️ КЛЮЧИ, ИСТОЧНИКОВ КОТОРЫХ У НАС ПОКА НЕТ, — падают в дефолт 0
    // (совпадает с поведением реф. для неизвестного ключа, но НЕ с его
    // поведением для этих ключей). Нужны, чтобы закрыть:
    //   0x00, 0x01 — KSystemStatus[+0x2C]/[+0x28] (Agc и соседнее поле;
    //                семантика второго НЕ УСТАНОВЛЕНА реверсом);
    //   0x07, 0x08 — KVideoParam Iris/Tone (у нас IrisMode есть в KVideoParam,
    //                но KVideoSet-обёртки нет);
    //   0x19       — DemoireStatus (есть в KVideoParam, нет в KVideoSet);
    //   0x101      — KSystemSet::GetSystemLanguage (у нас Language() -> QString,
    //                а реф. отдаёт enum-индекс: соответствие НЕ УСТАНОВЛЕНО);
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
    (void)nParam;
    m_lastAct = QStringLiteral("AddLampLevel");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::CancelResetUserParam(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("CancelResetUserParam");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::CloseLamp(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("CloseLamp");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::ConnectOrDisconnecEndo(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("ConnectOrDisconnecEndo");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::FileView(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("FileView");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::GainSwitchLongPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("GainSwitchLongPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::GainSwitchShortPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("GainSwitchShortPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::OpenLamp(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("OpenLamp");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::OpenOrCloseEliminatemoire(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("OpenOrCloseEliminatemoire");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::OpenOrCloseEndoInfoDialog(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("OpenOrCloseEndoInfoDialog");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::OpenOrCloseVersionDialog(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("OpenOrCloseVersionDialog");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::PowerOff(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("PowerOff");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButton0Act(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButton0Act");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButton1Act(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButton1Act");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButton2Act(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButton2Act");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButton3Act(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButton3Act");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButtonALongPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButtonALongPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButtonAShortPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButtonAShortPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButtonBLongPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButtonBLongPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButtonBShortPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButtonBShortPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButtonMLongPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButtonMLongPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::RemoteButtonMShortPressAct(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("RemoteButtonMShortPressAct");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::ResetUserParam(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("ResetUserParam");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SaveImage(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SaveImage");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetColorBValue(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetColorBValue");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetColorCValue(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetColorCValue");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetColorEnhL1Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetColorEnhL1Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetColorEnhL2Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetColorEnhL2Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetColorEnhL3Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetColorEnhL3Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetColorRValue(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetColorRValue");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetConnerShape(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetConnerShape");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetImgEnhL1Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetImgEnhL1Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetImgEnhL2Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetImgEnhL2Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetImgEnhL3Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetImgEnhL3Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetLanguage(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetLanguage");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetResolution(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetResolution");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetVLSGroup(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetVLSGroup");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetVideoSplit(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetVideoSplit");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetZoomL1Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetZoomL1Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetZoomL2Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetZoomL2Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SetZoomL3Value(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SetZoomL3Value");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::StartOrStopVideoRecord(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("StartOrStopVideoRecord");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::StartTrans(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("StartTrans");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::StartWhiteBalance(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("StartWhiteBalance");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SubLampLevel(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SubLampLevel");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchAirPumpLevel(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchAirPumpLevel");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchAirPumpStatus(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchAirPumpStatus");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchAutoLightAdjust(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchAutoLightAdjust");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchChbStatus(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchChbStatus");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchColorEnhLevel(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchColorEnhLevel");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchContrastType(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchContrastType");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchFreezeStatus(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchFreezeStatus");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchImgEnhLevel(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchImgEnhLevel");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchIrisType(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchIrisType");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchManuLightAdjust(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchManuLightAdjust");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchOperatingPattern(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchOperatingPattern");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchToneMode(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchToneMode");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchVlsMode(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchVlsMode");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchWindow(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchWindow");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::SwitchZoomLevel(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("SwitchZoomLevel");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::ToneAdd(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("ToneAdd");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::ToneSub(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("ToneSub");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
    return 1;
}

int KLcdProxy::UnmountUsbDevice(int nParam)
{
    (void)nParam;
    m_lastAct = QStringLiteral("UnmountUsbDevice");
    // ⚠️ КОНКРЕТНЫЙ КОД СООБЩЕНИЯ SendToMainCtrl ДЛЯ ЭТОГО ОБРАБОТЧИКА
    // НЕ УСТАНОВЛЕН реверсом (одностроч. функция 0x20-0x44 байта; шаблон
    // вызова виден, аргумент — нет). Маршрутизация проверяется тестом,
    // отправка кода будет добавлена после точечного дизасма.
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
