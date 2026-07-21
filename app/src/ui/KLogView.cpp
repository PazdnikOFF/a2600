#include "KLogView.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

KLogView::KLogView(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x712f40: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_LView →
    // роль-гейт btn_fullscreen (device) → connects → eventFilter → UpdateFileNameList/OpenNewFile.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_LView"));
}

void KLogView::setupUi()
{
    setObjectName(QStringLiteral("KLogView"));
    resize(800, 600);

    QWidget *host = ContentArea();
    QGridLayout *g = new QGridLayout(host);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setContentsMargins(9, 56, 9, 9);   // реф. (9,56,-1,-1)

    // Просмотрщик текста лога.
    QVBoxLayout *vLog = new QVBoxLayout();
    vLog->setObjectName(QStringLiteral("verticalLayout_log"));
    vLog->setContentsMargins(6, 6, 6, 6);
    QTextBrowser *browser = new QTextBrowser(host);
    browser->setObjectName(QStringLiteral("textBrowser"));
    browser->setFrameShadow(QFrame::Sunken);
    browser->setLineWidth(2);   // реф. lineWidth 2
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    vLog->addWidget(browser);   // содержимое — device (лог-файлы)
    g->addLayout(vLog, 0, 0);

    // Ряд навигации.
    QHBoxLayout *hKey = new QHBoxLayout();
    hKey->setObjectName(QStringLiteral("horizontalLayout_key"));
    hKey->setSpacing(40);
    hKey->setContentsMargins(60, 0, 60, 0);
    auto navBtn = [&](const char *name, const QString &text, int maxW) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        if (maxW > 0) b->setMaximumWidth(maxW);
        b->setFocusPolicy(Qt::TabFocus);
        hKey->addWidget(b);
        return b;
    };
    QPushButton *btnPrePage = navBtn("pushButton_prePage", QStringLiteral("<<"), 100);
    QPushButton *btnNextPage = navBtn("pushButton_nextPage", QStringLiteral(">>"), 100);
    QPushButton *btnFull = navBtn("btn_fullscreen", tr("TR_FScreen"), 0);
    btnFull->setFixedWidth(130);   // реф. min=max 130; в реф. видима только для роли 4 (device)
    // objectName↔глиф как в бинарнике (перепутаны): nextLog=«|<<», preLog=«>>|».
    navBtn("pushButton_nextLog", QStringLiteral("|<<"), 100);   // реф. NextLogView (смена файла — device)
    navBtn("pushButton_preLog", QStringLiteral(">>|"), 100);    // реф. PreLogView
    g->addLayout(hKey, 1, 0);

    // Пейджинг-кнопки → скролл (чистый UI; реф. Pre/NextPageView листают текст лога).
    connect(btnPrePage, &QPushButton::clicked, browser, [browser]() {
        browser->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    });
    connect(btnNextPage, &QPushButton::clicked, browser, [browser]() {
        browser->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    });
    // btn_fullscreen/nextLog/preLog — FullScreenView/смена лог-файла (файловая логика) — заглушки.
}
