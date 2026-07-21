#pragma once

#include <QFrame>
#include <QString>

class QSpinBox;
class QPushButton;
class QLabel;

// Конфиг OSD-спина (реф. _KOsdSpinConfig, 0x20 байт): заголовок + границы/шаг/дефолт +
// id сообщений устройства (msgId/ctxId — в порте не используются, замена на сигнал).
struct KOsdSpinConfig {
    QString title;
    int min = 0;
    int max = 100;
    int step = 1;
    int def = 0;
    int msgId = 0;   // device routing (не портируется)
    int ctxId = 0;   // device routing (не портируется)
};

// OSD-спинбокс (реф. иерархия QFrame→KFrame→KOsdSpinBase→KOsdSpin, ctor @0x484050).
// КОНТЕЙНЕР (не подкласс QSpinBox): держит внутренний QSpinBox (native-кнопки скрыты) +
// свои кнопки «-»/«+» + заголовок-метку в QGridLayout. UI-порт РЕАЛЬНОГО кастом-виджета.
// Промежуточные KFrame/KOsdSpinBase СПЛЮЩЕНЫ в QFrame (они добавляют device-навигацию
// аппаратными клавишами — DEVICE-STUB, в порт не тянется). setupUi @0x484330: фрейм фикс-
// ширина 250, grid-поля (9,0,9,0); label_title(col0) / spacer(col1) / btn_sub «-» 30×30
// NoFocus(col2) / spin_value min-width 120 NoFocus AlignCenter NoButtons(col3) / btn_add «+»
// 30×30 NoFocus(col4). Кнопки — литералы «+»/«-», без иконок. min/max/step/default — из
// конфига (InitWidget @0x483f98). RefreshSpinStatus @0x483ef0 — enable/disable ±кнопок на
// границах. SetIntValue @0x484250 — тихий сеттер (disconnect/setValue/reconnect).
//
// DEVICE-STUB: реф. ValueChangedAct шлёт KUiMsgProxy::SendToMainCtrl(msgId,ctxId,value) —
// заменено на Qt-сигнал valueChanged(int). Аппаратная навигация KOsdSpinBase опущена.
class KOsdSpin : public QFrame
{
    Q_OBJECT
public:
    explicit KOsdSpin(const KOsdSpinConfig &cfg, QWidget *parent = nullptr);

    int Value() const { return m_cur; }
    void SetIntValue(int v);   // реф. @0x484250 — тихий сеттер (без notify)

signals:
    void valueChanged(int value);   // порт: замена device SendToMainCtrl

private slots:
    void ClickAddBtnAct();   // реф.: cur += step; RefreshSpinStatus
    void ClickSubBtnAct();   // реф.: cur -= step; RefreshSpinStatus
    void ValueChangedAct(int value);

private:
    void setupUi(const KOsdSpinConfig &cfg);
    void InitWidget();          // реф. @0x483f98
    void RefreshSpinStatus();   // реф. @0x483ef0

    KOsdSpinConfig m_cfg;
    QSpinBox *m_spin = nullptr;
    QPushButton *m_btnAdd = nullptr;
    QPushButton *m_btnSub = nullptr;
    QLabel *m_title = nullptr;
    int m_cur = 0;   // +0x78 в реф.
};
