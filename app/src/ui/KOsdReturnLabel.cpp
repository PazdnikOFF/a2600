#include "KOsdReturnLabel.h"
#include "KDisplayOption.h"
#include "KOsdSubMenu.h"

#include <QHBoxLayout>
#include <QLabel>

KOsdReturnLabel::KOsdReturnLabel(QWidget *parent)
    : KFrame(parent)
{
    // Реф. ctor @0x482440 (setupUi заинлайнен; parent приходит позже через AddItem).
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("KOsdReturnLabel"));
    resize(250, 32);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    setMinimumSize(250, 32);
    setMaximumSize(250, 32);

    m_layout = new QHBoxLayout(this);
    m_layout->setObjectName(QStringLiteral("horizontalLayout"));
    m_layout->setSpacing(9);
    m_layout->setContentsMargins(9, 0, 0, 0);

    m_labelIcon = new QLabel(this);
    m_labelIcon->setObjectName(QStringLiteral("label_icon"));
    m_labelIcon->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_layout->addWidget(m_labelIcon);

    // Реф. retranslateUi: именно ТЕКСТ-плейсхолдер, пиксмап ставит только Selected/UnSelected.
    m_labelIcon->setText(tr("icon"));
}

QString KOsdReturnLabel::GetNormalImgPath()
{
    return QStringLiteral("return_normal.png");   // реф. @0x4823d0 (len 17)
}

QString KOsdReturnLabel::GetSelectImgPath()
{
    return QStringLiteral("return_select.png");   // реф. @0x482408 (len 17)
}

void KOsdReturnLabel::Selected()
{
    KFrame::Selected();
    m_labelIcon->setPixmap(KDisplayOption::Instance().GetOsdIconPixmap(GetSelectImgPath()));
}

void KOsdReturnLabel::UnSelected()
{
    KFrame::UnSelected();
    m_labelIcon->setPixmap(KDisplayOption::Instance().GetOsdIconPixmap(GetNormalImgPath()));
}

void KOsdReturnLabel::ConfirmKeyAct(int index)
{
    // Реф. @0x4828b0: аргумент ИГНОРИРУЕТСЯ, зовётся KDialog::close() меню-владельца
    // (невиртуально ⇒ KOsdSubMenu действительно наследник KDialog).
    Q_UNUSED(index);
    if (KOsdSubMenu *menu = GetLocatedMenu())
        menu->close();
}
