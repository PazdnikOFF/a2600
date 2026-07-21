#include "KDeviceInfo.h"

#include "ui/Theme.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

KDeviceInfo::KDeviceInfo(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x7396c0: setupUi → SetKStyle(2) → title TR_Svce → GetAndShowPublicQRCode →
    // connect(btn_exit→close).
    setupUi();
    SetKStyle(KDLG_W460);         // реф. SetKStyle(2)
    SetTitle(tr("TR_Svce"));      // реф. перекрывает "Form"
}

void KDeviceInfo::setupUi()
{
    setObjectName(QStringLiteral("KDeviceInfo"));
    resize(460, 768);

    QWidget *host = ContentArea();
    QGridLayout *grid = new QGridLayout(host);
    grid->setObjectName(QStringLiteral("gridLayout"));
    grid->setVerticalSpacing(30);
    grid->setContentsMargins(20, 40, 20, 30);

    // (0,0) верхняя распорка
    grid->addItem(new QSpacerItem(20, 167, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 0);

    // (1,0) подпись (обфусцированный, но подлинный TR-ключ)
    QLabel *label = new QLabel(host);
    label->setObjectName(QStringLiteral("label"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setText(tr("TR_WUPNSTSNTRNalfunction"));
    grid->addWidget(label, 1, 0);

    // (2,0) центрированная QR-рамка (реф. KQRCode 300×300; у нас QLabel с publicSign-картинкой).
    QHBoxLayout *h2 = new QHBoxLayout();
    h2->setObjectName(QStringLiteral("horizontalLayout_2"));
    h2->addStretch(1);
    QLabel *frame_qr = new QLabel(host);
    frame_qr->setObjectName(QStringLiteral("frame_publicSignQRCode"));
    frame_qr->setFixedSize(300, 300);
    frame_qr->setFrameShape(QFrame::StyledPanel);
    frame_qr->setFrameShadow(QFrame::Raised);
    frame_qr->setAlignment(Qt::AlignCenter);
    // Реф. GetAndShowPublicQRCode: publicSignCH/EN.jpg по языку; у нас EN.
    QPixmap qr(theme::asset(QStringLiteral("black/publicSignEN.jpg")));
    if (!qr.isNull())
        frame_qr->setPixmap(qr.scaled(298, 298, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    h2->addWidget(frame_qr);
    h2->addStretch(1);
    grid->addLayout(h2, 2, 0);

    // (3,0) распорка
    grid->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0);

    // (4,0) центрированная кнопка Exit 120×120
    QHBoxLayout *h1 = new QHBoxLayout();
    h1->setObjectName(QStringLiteral("horizontalLayout"));
    h1->addStretch(1);
    QPushButton *btn_exit = new QPushButton(host);
    btn_exit->setObjectName(QStringLiteral("btn_exit"));
    btn_exit->setText(tr("TR_Ext"));
    btn_exit->setFixedSize(120, 120);
    connect(btn_exit, &QPushButton::clicked, this, &QWidget::close);
    h1->addWidget(btn_exit);
    h1->addStretch(1);
    grid->addLayout(h1, 4, 0);
}
