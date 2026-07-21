#include "KSystemTemperature.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

KSystemTemperature::KSystemTemperature(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5f9658: KDialog(modal=false) → setupUi → SetKStyle(2) → title 温度监测 →
    // QTimer(1с)→UpdateTemperature (device).
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(QString::fromUtf8("温度监测"));
}

void KSystemTemperature::setupUi()
{
    setObjectName(QStringLiteral("KSystemTemperature"));
    resize(400, 336);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(20, 40, 20, 40);   // реф. margins

    // Сетка температур.
    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("frame"));
    QGridLayout *g = new QGridLayout(frame);
    g->setVerticalSpacing(12);
    auto tempRow = [&](int r, const QString &cap, const char *capName, const char *valName) {
        QLabel *c = new QLabel(cap, frame); c->setObjectName(QString::fromLatin1(capName));
        g->addWidget(c, r, 0);
        QLabel *v = new QLabel(frame); v->setObjectName(QString::fromLatin1(valName));
        v->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        g->addWidget(v, r, 1);   // значение — device
    };
    tempRow(0, QString::fromUtf8("4EV温度："), "label_4ev_temp_title", "label_4ev_temp");
    tempRow(1, QString::fromUtf8("白灯温度："), "label_wl_temp_title", "label_wl_temp");
    tempRow(2, QString::fromUtf8("UV灯温度："), "label_uv_temp_title", "label_uv_temp");
    root->addWidget(frame);

    root->addStretch(1);   // реф. verticalSpacer

    // Кнопка Exit (центрирована спейсерами).
    QFrame *frameBtn = new QFrame(host);
    frameBtn->setObjectName(QStringLiteral("frame_btn"));
    QHBoxLayout *h = new QHBoxLayout(frameBtn);
    h->addStretch(1);
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), frameBtn);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btnExit->setMinimumWidth(120);
    h->addWidget(btnExit);
    h->addStretch(1);
    root->addWidget(frameBtn);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. btn_exit не подключён — подключаем
}
