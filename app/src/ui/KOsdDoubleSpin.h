#pragma once

#include <QFrame>
#include <QString>

class QDoubleSpinBox;
class QPushButton;
class QLabel;

// Конфиг double-OSD-спина (реф. _KOsdDoubleSpinConfig, 0x30 байт, все double кроме ids).
// decimals в реф.-структуре явно не выделен — по device-кодированию value×10 (1 знак)
// принимаем decimals=1 по умолчанию (переопределяемо).
struct KOsdDoubleSpinConfig {
    QString title;
    double min = 0.0;
    double max = 100.0;
    double step = 0.1;
    double def = 0.0;
    int decimals = 1;   // инференс из device-кодирования ×10
    int msgId = 0;      // device routing (не портируется)
    int ctxId = 0;      // device routing (не портируется)
};

// OSD-спинбокс с плавающей точкой (реф. KOsdDoubleSpin @ctor 0x47ff98). СИБЛИНГ KOsdSpin —
// идентичный контейнер/сетка (QFrame, фрейм 250, ±кнопки 30×30 NoFocus, заголовок+spacer),
// но внутренний виджет — QDoubleSpinBox «doubleSpinBox» c setLayoutDirection(LeftToRight) +
// setDecimals(n). Промежуточные KFrame/KOsdSpinBase сплющены в QFrame (device hw-навигация).
// SetDoubleValue @silent — тихий сеттер. DEVICE-STUB: ValueChangedAct слал
// SendToMainCtrl (int)(value*10+offset) → заменён на Qt-сигнал valueChanged(double).
class KOsdDoubleSpin : public QFrame
{
    Q_OBJECT
public:
    explicit KOsdDoubleSpin(const KOsdDoubleSpinConfig &cfg, QWidget *parent = nullptr);

    double Value() const { return m_cur; }
    void SetDoubleValue(double v);   // тихий сеттер (без notify)

signals:
    void valueChanged(double value);   // порт: замена device SendToMainCtrl

private slots:
    void ClickAddBtnAct();
    void ClickSubBtnAct();
    void ValueChangedAct(double value);

private:
    void setupUi(const KOsdDoubleSpinConfig &cfg);
    void InitWidget();
    void RefreshSpinStatus();

    KOsdDoubleSpinConfig m_cfg;
    QDoubleSpinBox *m_spin = nullptr;
    QPushButton *m_btnAdd = nullptr;
    QPushButton *m_btnSub = nullptr;
    QLabel *m_title = nullptr;
    double m_cur = 0.0;   // +0x88 в реф.
};
