#pragma once

#include <QObject>
#include <QString>
#include <QVector>

// Шина сообщений «панель/OSD → главный контроллер» (реф. KUiMsgProxy, X-2600).
//
// РЕВЕРС: ВСЕ используемые KLcdProxy члены — это Qt-СИГНАЛЫ (тело = только
// QMetaObject::activate). Поэтому здесь заглушка того же вида, что и
// kernel/KThreadPoolMsg: сигналы объявлены, а для self-test ведётся журнал
// отправок. Настоящие получатели — device/UI-слой.
class KUiMsgProxy : public QObject
{
    Q_OBJECT
public:
    // Запись журнала (не из реф.): сколько аргументов реально передано.
    struct Sent { int argc; int a; int b; int c; };

    static QVector<Sent> TakeSent();
    static void ClearSent();
    static QStringList TakeDisplayed();

    // Не сигналы, а обёртки-регистраторы: в реф. это три перегрузки-СИГНАЛА
    // SendToMainCtrl(int) / (int,int) / (int,int,int) — 48 мест вызова.
    void SendToMainCtrl(int a);
    void SendToMainCtrl(int a, int b);
    void SendToMainCtrl(int a, int b, int c);
    void FileView(int v);
    void PanelKeyVersion();
    void PanelKeyEndoInfo();
    void UpdateRigidEndoBtnGuide();
    void UpdateFlexEndoBtnGuide();
    void DisplayMsg(const QString &msg);
    void SigLightAdjustDisabel();
};

// Реф. свободная функция-акцессор.
KUiMsgProxy *GetKUiMsgProxy();
