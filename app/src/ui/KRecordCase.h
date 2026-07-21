#pragma once

#include "ui/KDialog.h"

// Диалог записи автотест-кейса (реф. KRecordCase : KDialog, ctor @0x7340f8,
// Ui_KRecordCase::setupUi @0x735180). UI-порт. Немодальный, 320×240, SetKStyle(W460),
// титул tr("Record Case"). Тексты — англ. TR-ключи (не китайские). Форма: сетка
// Module Name/Case Name (QLineEdit, maxLen63) + чекбокс Record TimeStamp (checked) +
// кнопка Start Record. Абсолютная геометрия для кнопки/чекбокса, grid для формы.
// Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: RecordCaseName (запись кейса в /home/root/system/autotest/
// casefile/), ctor Mkdir — заглушки. Без OK/Cancel (закрытие — X титул-бара).
class KRecordCase : public KDialog
{
    Q_OBJECT
public:
    explicit KRecordCase(QWidget *parent = nullptr);

private:
    void setupUi();
};
