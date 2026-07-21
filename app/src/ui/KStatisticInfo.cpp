#include "KStatisticInfo.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace {
QFrame *mkLineH(QWidget *p)   // реф. KLineH → QFrame HLine
{
    QFrame *f = new QFrame(p);
    f->setFrameShape(QFrame::HLine);
    f->setFrameShadow(QFrame::Sunken);
    return f;
}
} // namespace

KStatisticInfo::KStatisticInfo(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x7437a0: QFrame(parent,0) → setupUi → InitWidget (device) →
    // connect(btn_EndoInfo, clicked, ClickOpenScopeInfo→emit OpenScopeInfo).
    setupUi();
}

void KStatisticInfo::setupUi()
{
    setObjectName(QStringLiteral("KStatisticInfo"));
    resize(667, 279);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);

    QGridLayout *g = new QGridLayout(this);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setColumnMinimumWidth(0, 20);   // реф. col0 — отступ
    g->setColumnStretch(1, 1);

    int r = 0;
    auto header = [&](const QString &text) {
        QHBoxLayout *h = new QHBoxLayout();
        QLabel *l = new QLabel(text, this);
        l->setStyleSheet(QStringLiteral("font-weight:bold;"));
        h->addWidget(l);
        h->addWidget(mkLineH(this), 1);
        g->addLayout(h, r, 0, 1, 3);
        ++r;
    };
    auto pair = [&](const QString &cap, const char *valName, const QString &init) {
        g->addWidget(new QLabel(cap, this), r, 1);
        QLabel *v = new QLabel(init, this);
        v->setObjectName(QString::fromLatin1(valName));
        v->setMinimumWidth(140); v->setMaximumWidth(140);   // реф. фикс. 140px
        g->addWidget(v, r, 2);
        ++r;
    };

    // Секция 1: процессор.
    header(tr("TR_IProcessor"));
    pair(QStringLiteral("SN:"), "label_ProcessorSN", QString());        // device
    pair(tr("TR_RSpace:"), "m_lb_remain_space", QStringLiteral("0.0"));
    pair(tr("TR_ATFVideo:"), "m_lb_video_time", QStringLiteral("0.0"));
    pair(tr("TR_ANOSPictures:"), "m_lb_pic_num", QStringLiteral("0.0"));

    // Секция 2: источник света.
    header(tr("TR_LSource"));
    pair(QStringLiteral("SN:"), "label_LightSourceSN", QString());       // device
    pair(tr("TR_AWTOCMLamp:"), "label_cur_num", QStringLiteral("0.0"));  // device (наработка тек.)
    pair(tr("TR_AWTOAMLamp:"), "label_all_num", QStringLiteral("0.0"));  // device (наработка всего)

    // Секция 3: эндоскоп.
    header(tr("TR_EInfo"));
    pair(QStringLiteral("SN:"), "label_ScopeSN", QString());             // device
    pair(tr("TR_FOUse:"), "label_Frequency", QString());                // device

    // Кнопка «информация об устройстве».
    QFrame *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("frame"));
    QHBoxLayout *hBtn = new QHBoxLayout(frame);
    hBtn->setContentsMargins(0, 0, 0, 0);
    QPushButton *btnEndo = new QPushButton(tr("TR_DInformaion"), frame);
    btnEndo->setObjectName(QStringLiteral("btn_EndoInfo"));
    btnEndo->setMinimumWidth(120);
    btnEndo->setFocusPolicy(Qt::TabFocus);
    hBtn->addWidget(btnEndo);
    hBtn->addStretch(1);
    g->addWidget(frame, r, 1, 1, 2);
    ++r;
    // Реф. ClickOpenScopeInfo → emit OpenScopeInfo() (сигнал наружу).
    connect(btnEndo, &QPushButton::clicked, this, &KStatisticInfo::OpenScopeInfo);

    g->setRowStretch(r, 1);   // реф. нижний вертикальный спейсер
}
