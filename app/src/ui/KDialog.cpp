#include "KDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

KDialog::KDialog(QWidget *parent, bool /*subscribeStatus*/)
    : QDialog(parent)
{
    // Реф. @0x68ad60: безрамочное окно поверх остальных.
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    buildChrome();
}

void KDialog::buildChrome()
{
    // Внешний layout диалога — только фон-рамка KFBackground во весь диалог.
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setSpacing(0);
    outer->setContentsMargins(0, 0, 0, 0);

    m_background = new QFrame(this);
    m_background->setObjectName(QStringLiteral("KFBackground"));
    m_background->setFrameShape(QFrame::StyledPanel);
    m_background->setFrameShadow(QFrame::Raised);
    // Реф. qss KFBackground: тёмный фон + серая рамка.
    m_background->setStyleSheet(QStringLiteral(
        "QFrame#KFBackground{background-color:rgba(26,26,26,255);"
        "border:1px solid rgba(83,83,83,255);}"));
    outer->addWidget(m_background);

    QVBoxLayout *bg = new QVBoxLayout(m_background);
    bg->setSpacing(0);
    bg->setContentsMargins(0, 0, 0, 0);

    // --- титул-бар KFUpbar (высота 35) ---
    m_upbar = new QFrame(m_background);
    m_upbar->setObjectName(QStringLiteral("KFUpbar"));
    m_upbar->setFrameShape(QFrame::StyledPanel);
    m_upbar->setFrameShadow(QFrame::Raised);
    m_upbar->setMinimumSize(0, 35);
    m_upbar->setMaximumHeight(35);
    m_upbar->setStyleSheet(QStringLiteral(
        "QFrame#KFUpbar{background-color:rgba(15,18,24,255);border:0px solid transparent;}"));
    QHBoxLayout *bar = new QHBoxLayout(m_upbar);
    bar->setSpacing(0);
    bar->setContentsMargins(15, 0, 15, 0);   // боковые поля 15

    m_titleLabel = new QLabel(m_upbar);
    m_titleLabel->setObjectName(QStringLiteral("label_title"));
    m_titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    // Реф. шрифт заголовка: Source Han Sans CN 19pt, светлый.
    m_titleLabel->setStyleSheet(QStringLiteral(
        "QLabel#label_title{color:rgb(220,220,220);"
        "font-family:'Source Han Sans CN';font-size:19px;background:transparent;border:none;}"));
    bar->addWidget(m_titleLabel);
    bar->addStretch(1);

    m_btnClose = new QToolButton(m_upbar);
    m_btnClose->setObjectName(QStringLiteral("btn_close"));
    m_btnClose->setAutoRaise(true);
    m_btnClose->setFocusPolicy(Qt::ClickFocus);
    m_btnClose->setStyleSheet(QStringLiteral("border-width:0px;\nbackground:transparent;"));
    m_btnClose->setText(QStringLiteral("✕"));   // × — реф. иконка close.png (device asset)
    m_btnClose->setStyleSheet(QStringLiteral(
        "QToolButton#btn_close{border-width:0px;background:transparent;color:rgb(200,200,200);"
        "font-size:18px;}"));
    connect(m_btnClose, &QToolButton::clicked, this, &QWidget::close);
    bar->addWidget(m_btnClose);

    bg->addWidget(m_upbar);

    // --- область контента подкласса (под титулом) ---
    m_content = new QWidget(m_background);
    m_content->setObjectName(QStringLiteral("KDlgContent"));
    m_content->setStyleSheet(QStringLiteral("QWidget#KDlgContent{background:transparent;}"));
    bg->addWidget(m_content, 1);
}

void KDialog::SetTitle(const QString &title)
{
    // Реф. @0x68b408: и системный windowTitle, и текст титул-лейбла.
    setWindowTitle(title);
    if (m_titleLabel)
        m_titleLabel->setText(title);
}

void KDialog::SetKStyle(_KDLG_STYLE style)
{
    // Реф. @0x68b910: пресеты фикс. ширины окна (спец: 1=fullscreen/без рамки).
    m_style = style;
    switch (style) {
    case KDLG_FULLSCREEN:
        if (m_background)   // «без рамки»
            m_background->setStyleSheet(QStringLiteral(
                "QFrame#KFBackground{background-color:rgba(26,26,26,255);border:0px;}"));
        break;
    case KDLG_W460:  setFixedWidth(460);  break;
    case KDLG_W320:  setFixedWidth(320);  break;
    case KDLG_W480:  setFixedWidth(480);  break;
    case KDLG_W640:  setFixedWidth(640);  break;
    case KDLG_W700:  setFixedWidth(700);  break;
    case KDLG_W1024: setFixedWidth(1024); break;
    case KDLG_DEFAULT:
    default: break;
    }
}

void KDialog::SetBtnCloseVisible(bool v)
{
    m_btnCloseVisible = v;
    if (m_btnClose)
        m_btnClose->setVisible(v);
}

void KDialog::keyPressEvent(QKeyEvent *event)
{
    // Реф. @0x68bb60: Esc закрывает, если close виден и разрешён.
    if (event->key() == Qt::Key_Escape && m_btnCloseVisible && m_btnCloseEnable) {
        close();
        return;
    }
    QDialog::keyPressEvent(event);
}
