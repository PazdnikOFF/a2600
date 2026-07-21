#pragma once

#include "ui/KDialog.h"

// Экран «публичного знака / сертификации» (реф. KDeviceInfo : KDialog, ctor @0x7396c0,
// Ui_KDeviceInfo::setupUi @0x7398c8). UI-порт. НЕ инфо-железа: по центру статичная
// регуляторная QR-картинка (publicSignCH/EN.jpg по языку) в рамке 300×300, сверху подпись,
// снизу кнопка Exit 120×120. Диалог 460×768, SetKStyle(W460), титул TR_Svce. setStyleSheet нет.
//
// Реф. QR-рамка — кастомный класс KQRCode (QFrame-наследник, ещё НЕ портирован) → у нас QLabel
// с картинкой publicSign (помечено). Выбор языка картинки (KSystemSet::GetSystemLanguage) —
// упрощён до EN. btn_exit→close.
class KDeviceInfo : public KDialog
{
    Q_OBJECT
public:
    explicit KDeviceInfo(QWidget *parent = nullptr);

private:
    void setupUi();
};
