#include "KUnusedImgPlayBar.h"

#include <QGridLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>

KUnusedImgPlayBar::KUnusedImgPlayBar(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x4ef598: QWidget(parent,0) → setupUi → validator → InitWidget →
    // RefreshPageNums (device) → InitConnect → CheckFirstOrLastPage.
    setupUi();
    resize(1614, 52);   // реф. resize
}

void KUnusedImgPlayBar::setupUi()
{
    setObjectName(QStringLiteral("KUnusedImgPlayBar"));

    QGridLayout *g = new QGridLayout(this);
    g->setObjectName(QStringLiteral("gridLayout"));

    QLabel *lblTotal = new QLabel(tr("TR_TotalNum"), this);
    lblTotal->setObjectName(QStringLiteral("label_total_num"));
    g->addWidget(lblTotal, 0, 0);

    auto pageBtn = [&](const char *name, const QString &glyph, int col) {
        QPushButton *b = new QPushButton(glyph, this);   // реф. KImgPushButton → QPushButton
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedSize(48, 32);   // реф. min=max 48×32
        g->addWidget(b, 0, col);
        return b;
    };
    pageBtn("btn_head", QStringLiteral("|<"), 1);   // реф. ClickBtnHead (device-модель)
    pageBtn("btn_pre", QStringLiteral("<"), 2);
    QLineEdit *edPage = new QLineEdit(this);
    edPage->setObjectName(QStringLiteral("edit_page"));
    edPage->setFixedSize(48, 32);
    edPage->setValidator(new QIntValidator(1, 99999, edPage));   // реф. numeric-validator
    g->addWidget(edPage, 0, 3);
    QLabel *lblTotPage = new QLabel(QStringLiteral("/1"), this);   // реф. литерал «/1»
    lblTotPage->setObjectName(QStringLiteral("label_total_page"));
    lblTotPage->setFixedSize(48, 32);
    lblTotPage->setAlignment(Qt::AlignCenter);
    g->addWidget(lblTotPage, 0, 4);
    pageBtn("btn_next", QStringLiteral(">"), 5);
    pageBtn("btn_tail", QStringLiteral(">|"), 6);
    g->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 7);
    // Навигация ClickBtn*/OneditingFinished → KUnusedImgModel — DEVICE, не подключаем.
}
