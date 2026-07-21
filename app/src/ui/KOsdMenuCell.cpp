#include "KOsdMenuCell.h"
#include "Theme.h"

#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QMouseEvent>
#include <QPixmap>

KOsdMenuCell::KOsdMenuCell(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x4812a8: setupUi → label_flag = GetOsdIconPixmap("flag_select.png").
    setupUi();
    m_labelFlag->setPixmap(osdIcon(QStringLiteral("flag_select.png"))
                               .scaled(10, 10, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QPixmap KOsdMenuCell::osdIcon(const QString &name) const
{
    // DEVICE-STUB KDisplayOption::GetOsdIconPixmap → грузим из темы.
    return QPixmap(theme::asset(QStringLiteral("black/osd/") + name));
}

void KOsdMenuCell::setupUi()
{
    // Реф. Ui_KOsdMenuCell::setupUi @0x481838: grid 3×2.
    setObjectName(QStringLiteral("KOsdMenuCell"));

    QGridLayout *g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setVerticalSpacing(0);

    // frame_flag (0,1, rowspan 3): полоска-индикатор «выбрано».
    m_frameFlag = new QFrame(this);
    m_frameFlag->setObjectName(QStringLiteral("frame_flag"));
    QVBoxLayout *vf = new QVBoxLayout(m_frameFlag);
    vf->setContentsMargins(0, 0, 0, 0); vf->setSpacing(0);
    m_labelFlag = new QLabel(m_frameFlag);
    m_labelFlag->setObjectName(QStringLiteral("label_flag"));
    m_labelFlag->setFixedSize(10, 10);
    vf->addWidget(m_labelFlag);
    g->addWidget(m_frameFlag, 0, 1, 3, 1);

    // frame_icon (1,0): круглая иконка-глиф со спейсерами по бокам.
    m_frameIcon = new QFrame(this);
    m_frameIcon->setObjectName(QStringLiteral("frame_icon"));
    QHBoxLayout *hi = new QHBoxLayout(m_frameIcon);
    hi->setContentsMargins(0, 0, 0, 0);
    hi->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    m_labelIcon = new QLabel(m_frameIcon);
    m_labelIcon->setObjectName(QStringLiteral("label_icon"));
    m_labelIcon->setMinimumSize(52, 52);
    m_labelIcon->setMaximumSize(76, 76);
    m_labelIcon->setScaledContents(true);
    m_labelIcon->setAlignment(Qt::AlignCenter);
    hi->addWidget(m_labelIcon);
    hi->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    g->addWidget(m_frameIcon, 1, 0);

    // label_tag (2,0): подпись под иконкой.
    m_labelTag = new QLabel(this);
    m_labelTag->setObjectName(QStringLiteral("label_tag"));
    m_labelTag->setAlignment(Qt::AlignCenter);
    g->addWidget(m_labelTag, 2, 0);

    UnSelect();   // начальное состояние — normal
}

void KOsdMenuCell::SetTitle(const QString &title)
{
    m_labelTag->setText(title);   // реф. @0x481100
}

void KOsdMenuCell::SetIcons(const QString &selected, const QString &unselected, const QString &greyed)
{
    m_iconSel = selected;
    m_iconUnsel = unselected;
    m_iconGrey = greyed;
    UpdateUI();
}

void KOsdMenuCell::Select()
{
    // Реф. @0x4813f0.
    const QString bg = theme::asset(QStringLiteral("black/osd/button_select.png"));
    setStyleSheet(QStringLiteral("QFrame#KOsdMenuCell{background-image:url(%1);"
                                 "border-radius:26px;} QLabel{color:rgb(221,221,221);}").arg(bg));
    setFixedWidth(160);
    if (!m_iconGrey.isEmpty() || !m_iconSel.isEmpty())
        m_labelIcon->setPixmap(osdIcon(m_greyed ? m_iconGrey : m_iconSel));
    m_labelIcon->setFixedSize(76, 76);
    m_selected = true;
    m_frameFlag->setVisible(showFlag() && !m_greyed);
}

void KOsdMenuCell::UnSelect()
{
    // Реф. @0x481680.
    const QString bg = theme::asset(QStringLiteral("black/osd/button_normal.png"));
    setStyleSheet(QStringLiteral("QFrame#KOsdMenuCell{background-image:url(%1);"
                                 "border-radius:19px;} QLabel{color:rgb(221,221,221);}").arg(bg));
    setFixedWidth(110);
    if (!m_iconUnsel.isEmpty())
        m_labelIcon->setPixmap(osdIcon(m_iconUnsel));
    m_labelIcon->setFixedSize(52, 52);
    m_selected = false;
    m_frameFlag->setVisible(false);
}

void KOsdMenuCell::Focus()
{
    // Реф. @0x481128: фон select, подпись cyan если не greyed.
    const QString bg = theme::asset(QStringLiteral("black/osd/button_select.png"));
    const QString color = m_greyed ? QStringLiteral("rgb(221,221,221)")
                                   : QStringLiteral("rgb(0,205,209)");
    setStyleSheet(QStringLiteral("QFrame#KOsdMenuCell{background-image:url(%1);"
                                 "border-radius:26px;} QLabel{color:%2;}").arg(bg, color));
}

void KOsdMenuCell::UpdateUI()
{
    if (IsSelected())
        Select();
    else
        UnSelect();
}

void KOsdMenuCell::mouseReleaseEvent(QMouseEvent *e)
{
    QFrame::mouseReleaseEvent(e);
    if (rect().contains(e->pos()))
        emit clicked();
}
