#pragma once

#include <QFrame>
#include <QString>

// OSD-оверлей живого видео жёсткого эндоскопа (реф. KViewHardEndo : QFrame, ctor @0x464a70,
// Ui_KViewHardEndo::setupUi @0x4650b8). UI-порт (reference-Ui оверлей; ОТЛИЧАЕТСЯ от нашего
// упрощённого video/KViewSoftEndo — тот paintEvent-вьювер кадров). Встраиваемый QFrame
// (реф. 891×596), абсолютная геометрия оверлеев (позиционируются в runtime InitShareWidget).
// Состав: label_video (область видео) + frame_time(rec-время «00:00:00»/иконка/системное
// время) + frame_connect(gridLayout_4: статус/USB «U1»/«U2» font14) + frame_lefttime
// (gridLayout_3: TR_RTime + остаток, серый) + frame_osd(хост OSD-меню) + widget_endobtnguide
// (KRigidEndoBtnGuide — уже портирован, показывает реальный ассет). Только QFrame/QLabel/grid.
//
// DEVICE в порт не тянется: содержимое всех меток (rec/часы/USB/остаток/видео/OSD-параметры)
// через KUiMsgProxy (InitMsgConnectStatus) — заглушки-плейсхолдеры; live-сигналы не подключены.
class KRigidEndoBtnGuide;
class KViewHardEndo : public QFrame
{
    Q_OBJECT
public:
    explicit KViewHardEndo(QWidget *parent = nullptr);

    // Реф. KViewHardEndo::DisplayMsg(QString) — вывод сообщения поверх видео
    // (зовётся из KViewBase::ShowExportErrorMsg @0x45bdd0). Текст сохраняется
    // и доступен наружу: реальная OSD-метка — device-часть.
    void DisplayMsg(const QString &msg);
    QString LastMsg() const { return m_lastMsg; }

private:
    void setupUi();

    QString m_lastMsg;
};
