#pragma once

#include <QFrame>

// Панель статистики устройства (реф. KStatisticInfo : QFrame — НЕ диалог!, ctor @0x7437a0,
// Ui_KStatisticInfo::setupUi @0x744258). UI-порт. Встраиваемый QFrame-панель (реф. 667×279,
// StyledPanel/Sunken); порт как самостоятельный QFrame (базовый класс совпал).
// Плоская read-only сетка меток из 3 секций: (1) TR_IProcessor — SN/остаток места/время
// видео/число снимков; (2) TR_LSource — SN/наработка лампы тек./всего; (3) TR_EInfo —
// SN эндоскопа/частота + кнопка btn_EndoInfo (эмитит сигнал OpenScopeInfo). Value-метки
// фикс. 140px. Кастом KLineH×3→QFrame(HLine). Нет таблиц/комбо/дат.
//
// DEVICE в порт не тянется: InitWidget (KSystemSet::GetProcessorSN, KEndoScope::GetEndoInfo/
// GetEepromData, GetMainCtrlThread::GetLamp*UsedTime) — заглушки; значения-плейсхолдеры.
// btn_EndoInfo→OpenScopeInfo (сигнал наружу) — реализован.
class KStatisticInfo : public QFrame
{
    Q_OBJECT
public:
    explicit KStatisticInfo(QWidget *parent = nullptr);

signals:
    void OpenScopeInfo();   // реф. публичный сигнал (idx 0), эмитится по btn_EndoInfo

private:
    void setupUi();
};
