#pragma once

#include <QList>
#include <QObject>
#include <QString>

// Семантический слой панели 8" (реф. KLcdProxy, X-2600).
//
// РЕВЕРС: обычный QObject, sizeof 24 (vptr + d_ptr + указатель на KUiMsgProxy).
// ⚠️ РАБОТЫ С ЖЕЛЕЗОМ НЕТ ВООБЩЕ — это СЕМАНТИЧЕСКИЙ слой НАД транспортом,
// а не транспорт. Единственные PLT-вызовы во всём диапазоне класса — QObject,
// QString, QMetaObject::tr, new/delete. Пометка «связь с панелью (Cortex-M)»
// в ROADMAP Фазы E была неверна (третий такой случай).
// ⚠️ Сигналов и слотов класс НЕ ОБЪЯВЛЯЕТ ВООБЩЕ (methodCount == 0);
// Q_OBJECT существует только ради qt_metacast.
//
// Синглтон отдаёт свободная функция Get_KLcdProxy() — ⚠️ с ПОДЧЁРКИВАНИЕМ,
// в отличие от GetEndoScope()/GetCamera().

// Пара «ключ-значение» для панели (реф. _KeyVlaue — ОПЕЧАТКА ВЕНДОРА сохранена).
// sizeof 4; значение — результат GetKeyStatus, УСЕЧЁННЫЙ до 16 бит.
struct _KeyVlaue {
    quint16 key;
    quint16 value;
};

class KLcdProxy : public QObject
{
    Q_OBJECT
public:
    KLcdProxy();

    // Диспетчер события панели. Реф.: линейный поиск по таблице из 82 записей
    // (граница ЗАШИТА как 0x52, а не sizeof/24); совпадение по ПАРЕ (key, act),
    // причём act сравнивается как ОДИН БАЙТ, хотя хранится как quint16.
    // ⚠️ Промах → 2 (без лога и без обработчика по умолчанию); попадание →
    // значение обработчика (все они возвращают 1).
    // ⚠️ Расширение знака параметра проверяет ТОЛЬКО БИТ 7: 0x180 станет 0x80.
    int KeyEventAct(quint16 usKey, quint8 ucAct, int nParam);

    // Единственный источник значений для панели. Реф.: сбалансированный
    // switch, ДЕФОЛТ 0, никаких проверок диапазона.
    // ⚠️ Ключи 0x04-0x08 и 0x17 отдают уровень +1 (панель ждёт 1-based,
    // а конфиг хранит 0-based).
    // ⚠️ Ключ 0x104 при GetEndoShapeType() != 0 отдаёт СЕНТИНЕЛ 0xFFFE.
    // ⚠️ Ключ 0x217 НАМЕРЕННО пропущен (индекс 4 = ButtomM/LongPress —
    // write-only мёртвый конфиг), всегда 0.
    int GetKeyStatus(quint16 usKey);

    // Продюсеры наборов для экранов панели.
    QList<_KeyVlaue> GetSoftEndoMainViewParams();
    QList<_KeyVlaue> GetSoftEndoUserSet1Params();
    QList<_KeyVlaue> GetSoftEndoUserSet2Params();
    QList<_KeyVlaue> GetSoftEndoUserSet3Params();
    QList<_KeyVlaue> GetSoftEndoSystemSet1Params();
    // Реф.: ЧИСТЫЙ tail-call алиас Soft-варианта — отдельная точка входа.
    QList<_KeyVlaue> GetHardEndoSystemSet1Params();
    QList<_KeyVlaue> GetSoftEndoSystemSet2Params();
    QList<_KeyVlaue> GetHardEndMainViewParams();
    QList<_KeyVlaue> GetHardEndoUserSet1Params();
    QList<_KeyVlaue> GetHardEndoUserSet2Params();
    QList<_KeyVlaue> GetHardEndoSystemSet2Params();
    QList<_KeyVlaue> GetHardEndoOperationModeParams();
    QList<_KeyVlaue> GetPanelLedSet();

    // ⚠️ ИМЯ ЛЖЁТ: это НЕ предикат. Тело — «если GetEndoType() == 13, послать
    // SigLightAdjustDisabel». Возвращаемое значение в реф. мусорное, и ВСЕ
    // четыре вызывающих его игнорируют. Реализовано как void.
    void CheckIsLightAdjustEnable();
    // ⚠️ «Formate» — опечатка вендора. Промах по списку → 0 (неизвестный
    // формат молча выглядит как yyyy/MM/dd).
    int GetDateFormateIndex();

    // --- обработчики таблицы (все возвращают 1) -----------------------------
    int AddLampLevel(int nParam);
    int CancelResetUserParam(int nParam);
    int CloseLamp(int nParam);
    int ConnectOrDisconnecEndo(int nParam);
    int FileView(int nParam);
    int GainSwitchLongPressAct(int nParam);
    int GainSwitchShortPressAct(int nParam);
    int LeftFootSwitchAct(int nParam);
    int LockScreenAct(int nParam);
    int OpenLamp(int nParam);
    int OpenOrCloseEliminatemoire(int nParam);
    int OpenOrCloseEndoInfoDialog(int nParam);
    int OpenOrCloseVersionDialog(int nParam);
    int PowerOff(int nParam);
    int RemoteButton0Act(int nParam);
    int RemoteButton1Act(int nParam);
    int RemoteButton2Act(int nParam);
    int RemoteButton3Act(int nParam);
    int RemoteButtonALongPressAct(int nParam);
    int RemoteButtonAShortPressAct(int nParam);
    int RemoteButtonBLongPressAct(int nParam);
    int RemoteButtonBShortPressAct(int nParam);
    int RemoteButtonMLongPressAct(int nParam);
    int RemoteButtonMShortPressAct(int nParam);
    int ResetUserParam(int nParam);
    int RightFootSwitchAct(int nParam);
    int SaveImage(int nParam);
    int SetCameraButtomALongPressFuncIndex(int nParam);
    int SetCameraButtomAShortPressFuncIndex(int nParam);
    int SetCameraButtomBLongPressFuncIndex(int nParam);
    int SetCameraButtomBShortPressFuncIndex(int nParam);
    int SetCameraButtomMLongPressFuncIndex(int nParam);
    int SetCameraButtomMShortPressFuncIndex(int nParam);
    int SetColorBValue(int nParam);
    int SetColorCValue(int nParam);
    int SetColorEnhL1Value(int nParam);
    int SetColorEnhL2Value(int nParam);
    int SetColorEnhL3Value(int nParam);
    int SetColorRValue(int nParam);
    int SetConnerShape(int nParam);
    int SetEndoButton0Funcindex(int nParam);
    int SetEndoButton1Funcindex(int nParam);
    int SetEndoButton2Funcindex(int nParam);
    int SetEndoButton3Funcindex(int nParam);
    int SetImgEnhL1Value(int nParam);
    int SetImgEnhL2Value(int nParam);
    int SetImgEnhL3Value(int nParam);
    int SetLanguage(int nParam);
    int SetLeftFootSwitchFuncindex(int nParam);
    int SetResolution(int nParam);
    int SetRightFootSwitchFuncindex(int nParam);
    int SetTimeFormat(int nParam);
    int SetVLSGroup(int nParam);
    int SetVideoSplit(int nParam);
    int SetZoomL1Value(int nParam);
    int SetZoomL2Value(int nParam);
    int SetZoomL3Value(int nParam);
    int StartOrStopVideoRecord(int nParam);
    int StartTrans(int nParam);
    int StartWhiteBalance(int nParam);
    int SubLampLevel(int nParam);
    int SwitchAirPumpLevel(int nParam);
    int SwitchAirPumpStatus(int nParam);
    int SwitchAutoLightAdjust(int nParam);
    int SwitchChbStatus(int nParam);
    int SwitchColorEnhLevel(int nParam);
    int SwitchContrastType(int nParam);
    int SwitchFreezeStatus(int nParam);
    int SwitchImgEnhLevel(int nParam);
    int SwitchIrisType(int nParam);
    int SwitchManuLightAdjust(int nParam);
    int SwitchOperatingPattern(int nParam);
    int SwitchToneMode(int nParam);
    int SwitchVlsMode(int nParam);
    int SwitchWindow(int nParam);
    int SwitchZoomLevel(int nParam);
    int ToneAdd(int nParam);
    int ToneSub(int nParam);
    int UnmountUsbDevice(int nParam);

    // Не из реф. — шов для self-test: имя последнего сработавшего обработчика.
    QString LastAct() const { return m_lastAct; }
    void ClearLastAct() { m_lastAct.clear(); }

private:
    // Общая точка назначения функции кнопке (реф. SetButtonFunc(int,int)).
    int SetButtonFunc(int nKeyId, int nIndex);

    class KUiMsgProxy *m_pUiMsgProxy = nullptr;
    QString m_lastAct;   // только для self-test
};

// Реф. свободная функция-синглтон (⚠️ с подчёркиванием).
KLcdProxy *Get_KLcdProxy();
