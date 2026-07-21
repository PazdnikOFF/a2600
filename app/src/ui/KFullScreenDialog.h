#pragma once

#include "KDialog.h"
#include "kernel/KObject.h"

// Полноэкранный диалог-база (реф. KFullScreenDialog @ctor 0x3e95f0). УСТАНОВЛЕНО реверсом:
// множественное наследование KDialog + KObject. Ctor(parent, id): KDialog(parent,false) +
// KObject(id,nullptr) + SetKStyle(KDLG_FULLSCREEN). int-арг — НЕ размер/z-order, а
// GLOBAL-ID объекта на in-process шине сообщений (KObject::InitObject: id∈[0,4999) уникален,
// либо -1=K_OBJ_LOCAL не регистрируется). Класс существует РАДИ KObject-идентичности:
// подклассы (KPatientManagmentUi id=2000) через неё делают SubscribeMsg/PostMsg/PublishMsg.
// Своих членов/хрома/сигналов НЕ добавляет — фуллскрин-вид от KDialog::SetKStyle(FULLSCREEN).
// 100% PORT (Qt + in-process реестр KObject). Заменяет прежнюю подстановку KDialog+SetKStyle.
class KFullScreenDialog : public KDialog, public KObject
{
    Q_OBJECT
public:
    explicit KFullScreenDialog(QWidget *parent = nullptr, int id = -1);
};
