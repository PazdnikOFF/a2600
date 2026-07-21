#include "KParamSetBtn.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

KParamSetBtn::KParamSetBtn(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x685030: QFrame(parent,0) → setupUi → frame_page hidden → connects
    // (custom1/2/3/pre/next/default/save/exit) + GetSystemStatus (device, гейтинг custom1).
    setupUi();
}

void KParamSetBtn::setupUi()
{
    setObjectName(QStringLiteral("KParamSetBtn"));
    resize(420, 114);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setSpacing(0);
    v->setContentsMargins(0, 0, 0, 0);

    // Ряд кастом-функций.
    m_frameCustom = new QFrame(this);
    m_frameCustom->setObjectName(QStringLiteral("frame_custom"));
    QHBoxLayout *h2 = new QHBoxLayout(m_frameCustom);
    h2->setObjectName(QStringLiteral("horizontalLayout_2"));
    auto mkBtn = [](QWidget *p, const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, p);
        b->setObjectName(QString::fromLatin1(name));
        return b;
    };
    m_custom1 = mkBtn(m_frameCustom, "btn_custom1", QString());   // текст — SetBtnCustomFuncName
    m_custom2 = mkBtn(m_frameCustom, "btn_custom2", QString());
    m_custom3 = mkBtn(m_frameCustom, "btn_custom3", QString());
    h2->addWidget(m_custom1); h2->addWidget(m_custom2); h2->addWidget(m_custom3);
    v->addWidget(m_frameCustom);

    // Ряд навигации по страницам (скрыт по умолчанию).
    m_framePage = new QFrame(this);
    m_framePage->setObjectName(QStringLiteral("frame_page"));
    QHBoxLayout *hp = new QHBoxLayout(m_framePage);
    hp->setObjectName(QStringLiteral("horizontalLayout"));
    m_prePage = mkBtn(m_framePage, "btn_pre_page", QStringLiteral("<"));
    m_labelPage = new QLabel(QStringLiteral("1/2"), m_framePage);
    m_labelPage->setObjectName(QStringLiteral("label_page"));
    m_labelPage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_nextPage = mkBtn(m_framePage, "btn_next_page", QStringLiteral(">"));
    hp->addWidget(m_prePage); hp->addWidget(m_labelPage); hp->addWidget(m_nextPage);
    m_framePage->setVisible(false);   // реф. скрыт до SetTotalPageNum(>1)
    v->addWidget(m_framePage);

    // Ряд Default/Save/Exit.
    QFrame *frameBtn = new QFrame(this);
    frameBtn->setObjectName(QStringLiteral("frame_btn"));
    QHBoxLayout *h3 = new QHBoxLayout(frameBtn);
    h3->setObjectName(QStringLiteral("horizontalLayout_3"));
    QPushButton *btnDefault = mkBtn(frameBtn, "btn_default", tr("TR_Dflt"));
    QPushButton *btnSave = mkBtn(frameBtn, "btn_save", tr("TR_Sve"));
    QPushButton *btnExit = mkBtn(frameBtn, "btn_exit", tr("TR_Ext"));
    h3->addWidget(btnDefault); h3->addWidget(btnSave); h3->addWidget(btnExit);
    v->addWidget(frameBtn);

    // Связки — реф. слоты эмитят публичные сигналы.
    connect(m_custom1, &QPushButton::clicked, this, [this]() { emit ClickBtnCustom(1); });
    connect(m_custom2, &QPushButton::clicked, this, [this]() { emit ClickBtnCustom(2); });
    connect(m_custom3, &QPushButton::clicked, this, [this]() { emit ClickBtnCustom(3); });
    connect(btnDefault, &QPushButton::clicked, this, &KParamSetBtn::ClickBtnDeafault);
    connect(btnSave, &QPushButton::clicked, this, &KParamSetBtn::ClickBtnSave);
    connect(btnExit, &QPushButton::clicked, this, &KParamSetBtn::ClickBtnExit);
    connect(m_nextPage, &QPushButton::clicked, this, [this]() {
        if (m_currentPage >= m_totalPageNum) return;
        ++m_currentPage;
        m_prePage->setEnabled(true);
        m_nextPage->setEnabled(m_currentPage < m_totalPageNum);
        updateLabelPageShow();
        emit PageChanged(m_currentPage);
    });
    connect(m_prePage, &QPushButton::clicked, this, [this]() {
        if (m_currentPage <= 1) return;
        --m_currentPage;
        m_nextPage->setEnabled(true);
        m_prePage->setEnabled(m_currentPage > 1);
        updateLabelPageShow();
        emit PageChanged(m_currentPage);
    });
    // GetSystemStatus → SystemStatusChangeAct (гейтинг custom1) — DEVICE, не подключаем.
}

void KParamSetBtn::updateLabelPageShow()
{
    m_labelPage->setText(QStringLiteral("%1/%2").arg(m_currentPage).arg(m_totalPageNum));
}

void KParamSetBtn::SetTotalPageNum(int n)
{
    m_totalPageNum = n;
    m_currentPage = 1;
    m_framePage->setVisible(n > 1);   // реф. page-nav только при n>1
    m_prePage->setEnabled(false);
    m_nextPage->setEnabled(n > 1);
    updateLabelPageShow();
}

void KParamSetBtn::SetBtnCustomFuncName(const QString &a, const QString &b, const QString &c)
{
    // Реф.: elide по ширине кнопки + tooltip с полным текстом.
    QPushButton *btns[3] = {m_custom1, m_custom2, m_custom3};
    const QString texts[3] = {a, b, c};
    for (int i = 0; i < 3; ++i) {
        QFontMetrics fm(btns[i]->font());
        btns[i]->setText(fm.elidedText(texts[i], Qt::ElideRight, btns[i]->width() - 8));
        btns[i]->setToolTip(texts[i]);
    }
}

void KParamSetBtn::HideCustom3Btn()
{
    m_custom3->setVisible(false);   // реф. HideCustom3Btn
}

void KParamSetBtn::SetBtnCustomVisible(bool v)
{
    m_frameCustom->setVisible(v);   // реф. весь ряд custom
}
