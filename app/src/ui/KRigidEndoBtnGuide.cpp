#include "KRigidEndoBtnGuide.h"

#include <QLabel>
#include <QPixmap>

#include "ui/Theme.h"

KRigidEndoBtnGuide::KRigidEndoBtnGuide(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x69d768: QWidget(parent,0) → setupUi → InitWidgets (фикс. геометрия,
    // фон-пиксмап, тексты функций из device).
    setupUi();
}

void KRigidEndoBtnGuide::setupUi()
{
    setObjectName(QStringLiteral("KRigidEndoBtnGuide"));
    setMinimumSize(540, 370);   // реф. контент-канва (resize 730×513, фикс. по InitWidgets)

    // Фон — диаграмма эндоскопа (theme::asset; локальная сверка).
    QLabel *bg = new QLabel(this);
    bg->setObjectName(QStringLiteral("label_background"));
    bg->setGeometry(0, 0, 540, 370);
    bg->setAlignment(Qt::AlignCenter);
    QPixmap pm(theme::asset(QStringLiteral("black/icon/rigidendo_btn_guide.png")));
    if (!pm.isNull())
        bg->setPixmap(pm.scaled(540, 370, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    bg->lower();

    // Подпись функции кнопки: имя (device→фолбэк TR_EEOMenu) + суффикс TR_S/TR_L.
    auto funcLbl = [&](const char *name, int x, int y, int w, Qt::Alignment al, const QString &suffix) {
        QLabel *l = new QLabel(this);
        l->setObjectName(QString::fromLatin1(name));
        l->setGeometry(x, y, w, 20);
        l->setAlignment(al);
        l->setText(tr("TR_EEOMenu") + QStringLiteral(" ") + suffix);   // реф. GetEndoBtnFuncText — device
        return l;
    };
    // Кнопка A (лево, к изображению — вправо), короткое/долгое.
    funcLbl("label_A_short", 0, 67, 210, Qt::AlignRight, tr("TR_S"));
    funcLbl("label_A_long", 0, 89, 210, Qt::AlignRight, tr("TR_L"));
    // Кнопка B (право, к изображению — влево).
    funcLbl("label_B_short", 330, 67, 210, Qt::AlignLeft, tr("TR_S"));
    funcLbl("label_B_long", 330, 89, 210, Qt::AlignLeft, tr("TR_L"));
    // Кнопка M (мультифункция, центр, во всю ширину).
    funcLbl("label_M_short", 0, 254, 540, Qt::AlignCenter, tr("TR_S"));
    funcLbl("label_M_long", 0, 274, 540, Qt::AlignCenter, tr("TR_L"));

    // Статичные подписи.
    QLabel *lblAbbr = new QLabel(tr("TR_SSFSPLSFLPress"), this);
    lblAbbr->setObjectName(QStringLiteral("label_abbreviation_explain"));
    lblAbbr->setGeometry(0, 296, 540, 20); lblAbbr->setAlignment(Qt::AlignCenter);
    QLabel *lblExit = new QLabel(tr("TR_PABOTCTCTWindow"), this);
    lblExit->setObjectName(QStringLiteral("label_exit_tip"));
    lblExit->setGeometry(0, 334, 540, 20); lblExit->setAlignment(Qt::AlignCenter);
}
