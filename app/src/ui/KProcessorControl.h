#pragma once

#include "ui/KDialog.h"

// Диалог управления процессором/машинного контроля (реф. KProcessorControl : KDialog,
// ctor @0x73b870, Ui_KProcessorControl::setupUi @0x73cb28). UI-порт. Открывается из
// KUserSrvSet(Processor). Немодальный, 640×480, SetKStyle(W460), титул TR_PControl.
// Две группы лицензионного контроля: (groupBox TR_TControl) срок/остаток дней + кнопки
// открыть-снять-контроль/импорт-авторизации; (groupBox_matchEndo TR_CEControl) число
// эндоскопов + View + открыть-снять/импорт. Снизу Exit. Только метки+кнопки, stock Qt.
// Смешанные подписи: TR-ключи + китайские литералы (到期时间/剩余天数/开启/解除管控/镜体数量).
//
// DEVICE в порт не тянется: ChangeTimeCtlState/ImportDelayLicense/ChangeEndoMatchCtlState/
// ImportMatchEndoLicense/ViewEndoScopeList, KControlINI::ReadMcTime/ReadMcEndo (значения
// срока/остатка/числа), EndoScope/Camera-статусы — заглушки. Exit→close.
class KProcessorControl : public KDialog
{
    Q_OBJECT
public:
    explicit KProcessorControl(QWidget *parent = nullptr);

private:
    void setupUi();
};
