#pragma once

#include "ui/KDialog.h"

// Диалог машинного контроля/лицензии (реф. KControlInfo : KDialog, ctor @0x707d48,
// Ui_KControlInfo::setupUi @0x709098). UI-порт. SetKStyle(W460), титул TR_TTInformation.
// Две группы (Processor/Endoscope): для каждой SN / дата (EDate) / остаток срока-использований /
// число поддерживаемых устройств + кнопки View (совпадающие устройства) и Activate (активация
// лицензии); снизу — Exit. Чистые layout'ы, setStyleSheet нет.
//
// DEVICE в порт не тянется: InitGroupProcessorCtrl/InitGroupEndoCtrl/ReleaseProcessorCtrl/
// ReleaseEndoCtrl/CheckLicense/ShowProcessor/ShowEndos (KControlProc/DES/EEPROM — лицензии,
// серийники, счётчики) — заглушки; поля-значения пусты. Exit→close.
class KControlInfo : public KDialog
{
    Q_OBJECT
public:
    explicit KControlInfo(QWidget *parent = nullptr);

private:
    void setupUi();
};
