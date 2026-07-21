#include "KLayOut.h"
#include "Theme.h"

#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QIcon>
#include <QPixmap>

KLayOut::KLayOut(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x68bd20: setupUi → btn_close hidden → 2-state иконка → connect clicked→clickBtnClose.
    setupUi();
    m_btnClose->setVisible(false);   // реф.: скрыт по умолчанию

    // Реф. KDisplayOption::GetIcon("icon/close.png","icon/close_check.png") — 2 состояния.
    QIcon ic;
    ic.addPixmap(QPixmap(theme::asset(QStringLiteral("black/icon/close.png"))), QIcon::Normal, QIcon::Off);
    ic.addPixmap(QPixmap(theme::asset(QStringLiteral("black/icon/close_check.png"))), QIcon::Active, QIcon::On);
    m_btnClose->setIcon(ic);

    connect(m_btnClose, &QToolButton::clicked, this, &KLayOut::clickBtnClose);
}

void KLayOut::setupUi()
{
    // Реф. Ui_KLayOut::setupUi @0x68c048.
    setObjectName(QStringLiteral("KLayOut"));
    resize(386, 300);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setSpacing(0);
    v->setContentsMargins(0, 0, 0, 0);

    // Фон-панель (objectName-стилизуемый QFrame).
    m_background = new QFrame(this);
    m_background->setObjectName(QStringLiteral("KFBackground"));
    m_background->setFrameShape(QFrame::StyledPanel);
    m_background->setFrameShadow(QFrame::Raised);
    v->addWidget(m_background);

    QVBoxLayout *v2 = new QVBoxLayout(m_background);
    v2->setObjectName(QStringLiteral("verticalLayout_2"));
    v2->setSpacing(0);
    v2->setContentsMargins(0, 0, 0, 0);

    // Титул-полоса.
    m_upbar = new QFrame(m_background);
    m_upbar->setObjectName(QStringLiteral("KFUpbar"));
    m_upbar->setMinimumSize(0, 35);
    m_upbar->setFrameShape(QFrame::StyledPanel);
    m_upbar->setFrameShadow(QFrame::Raised);
    v2->addWidget(m_upbar);

    QHBoxLayout *h = new QHBoxLayout(m_upbar);
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->setContentsMargins(15, 0, 15, 0);
    m_labelTitle = new QLabel(m_upbar);
    m_labelTitle->setObjectName(QStringLiteral("label_title"));
    h->addWidget(m_labelTitle);
    m_btnClose = new QToolButton(m_upbar);
    m_btnClose->setObjectName(QStringLiteral("btn_close"));
    m_btnClose->setFocusPolicy(Qt::ClickFocus);
    m_btnClose->setStyleSheet(QStringLiteral("border-width: 0px;\nbackground: transparent;"));
    m_btnClose->setIconSize(QSize(32, 32));
    m_btnClose->setAutoRaise(true);
    h->addWidget(m_btnClose);

    // Пустое тело — спейсер прижимает upbar кверху.
    v2->addItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void KLayOut::setTitle(const QString &title)
{
    m_labelTitle->setText(title);
}

void KLayOut::setNoFrame()
{
    m_background->setStyleSheet(QStringLiteral("border: 0px"));
}

void KLayOut::setNoCloseBtn(bool noBtn)
{
    m_btnClose->setVisible(!noBtn);
}
