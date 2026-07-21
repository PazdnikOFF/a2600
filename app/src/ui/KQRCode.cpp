#include "KQRCode.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>

KQRCode::KQRCode(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x745df0: QFrame(parent,0) → setupUi. Без SetKStyle/SetTitle/connects.
    setupUi();
}

void KQRCode::setupUi()
{
    setObjectName(QStringLiteral("KQRCode"));
    resize(307, 300);
    setMinimumSize(300, 300);
    setFrameShape(QFrame::StyledPanel);   // реф. shape=6
    setFrameShadow(QFrame::Raised);        // реф. shadow=0x20

    QGridLayout *g = new QGridLayout(this);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setHorizontalSpacing(0);
    g->setVerticalSpacing(10);
    g->setContentsMargins(0, 0, 0, 0);

    // Строка QR-изображения (центрирована спейсерами).
    QHBoxLayout *h2 = new QHBoxLayout();
    h2->setObjectName(QStringLiteral("horizontalLayout_2"));
    h2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    QLabel *pic = new QLabel(this);
    pic->setObjectName(QStringLiteral("label_QRCodePic"));
    pic->setFixedSize(260, 260);        // реф. min=max 260×260
    pic->setScaledContents(true);       // реф. setScaledContents(true)
    pic->setStyleSheet(QStringLiteral("border:1px solid rgb(83,83,83);"));   // покажем QR-слот (реф. пусто, device)
    h2->addWidget(pic);
    h2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    g->addLayout(h2, 0, 0);

    // Строка подписи (центрирована спейсерами).
    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    QLabel *info = new QLabel(this);
    info->setObjectName(QStringLiteral("label_QRCodeInfo"));
    info->setMinimumSize(100, 30);
    info->setAlignment(Qt::AlignCenter);
    info->setStyleSheet(QStringLiteral("font-size:18px;"));   // реф. стиль; текст — device
    h->addWidget(info);
    h->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    g->addLayout(h, 1, 0);
}
