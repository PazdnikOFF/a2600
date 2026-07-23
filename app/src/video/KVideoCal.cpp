#include "video/KVideoCal.h"
#include "ui/KDisplayOption.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

// ──────────────────────────── Диалог (реф. Ui_KVideoCal) ────────────────────────────

namespace {
// Хелперы, чтобы разметка читалась как дерево из setupUi.
QSpinBox *MakeSpin(QWidget *p, const char *name, int mn, int mx)
{
    QSpinBox *s = new QSpinBox(p);
    s->setObjectName(QString::fromLatin1(name));
    s->setMinimum(mn);
    s->setMaximum(mx);
    return s;
}

QLabel *MakeLabel(QWidget *p, const char *name, const QString &text)
{
    QLabel *l = new QLabel(text, p);
    l->setObjectName(QString::fromLatin1(name));
    return l;
}

QPushButton *MakeBtn(QWidget *p, const char *name, const QString &text)
{
    QPushButton *b = new QPushButton(text, p);
    b->setObjectName(QString::fromLatin1(name));
    return b;
}

QComboBox *MakeCombo(QWidget *p, const char *name)
{
    QComboBox *c = new QComboBox(p);
    c->setObjectName(QString::fromLatin1(name));
    return c;
}
}   // namespace

KVideoCal::KVideoCal(QWidget *parent)
    : KDialog(parent, false)   // реф.: non-modal
{
    // Реф. ctor @0x63b4e8.
    setObjectName(QStringLiteral("KVideoCal"));
    SetKStyle(KDLG_W460);      // реф. SetKStyle(2)
    SetTitle(QStringLiteral("TR_Dlg"));
    setupUi();
    InitWidget();
    resize(300, 982);          // реф. resize(300, 982)
    // Реф. дополнительно ставит KSystemStatus::SetIsVideoCal(true) — device, опущено.
}

void KVideoCal::setupUi()
{
    QWidget *root = ContentArea();
    QVBoxLayout *verticalLayout = new QVBoxLayout(root);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    // ── group_corner (TR_CShape) ──
    QGroupBox *group_corner = new QGroupBox(QStringLiteral("TR_CShape"), root);
    group_corner->setObjectName(QStringLiteral("group_corner"));
    QGridLayout *gridLayout = new QGridLayout(group_corner);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    cmb_corner_type = MakeCombo(group_corner, "cmb_corner_type");
    cmb_corner_mode = MakeCombo(group_corner, "cmb_corner_mode");
    spin_corner_para   = MakeSpin(group_corner, "spin_corner_para", 0, 99);     // реф. диапазон не задан → дефолт Qt
    spin_corner_para_2 = MakeSpin(group_corner, "spin_corner_para_2", 0, 99);
    dspin_flag_pos = new QDoubleSpinBox(group_corner);
    dspin_flag_pos->setObjectName(QStringLiteral("dspin_flag_pos"));
    dspin_flag_pos->setMinimum(0.1);        // реф. @0x63d2a8
    dspin_flag_pos->setMaximum(0.9);        // реф. @0x63d2b8
    dspin_flag_pos->setSingleStep(0.1);
    dspin_flag_pos->setValue(0.9);
    gridLayout->addWidget(MakeLabel(group_corner, "label_corner_type", QStringLiteral("TR_Tpe:")), 0, 0);
    gridLayout->addWidget(cmb_corner_type, 0, 1, 1, 2);
    gridLayout->addWidget(MakeLabel(group_corner, "label_corner_mode", QStringLiteral("TR_Mde:")), 1, 0);
    gridLayout->addWidget(cmb_corner_mode, 1, 1, 1, 2);
    gridLayout->addWidget(MakeLabel(group_corner, "label_corner_para", QStringLiteral("TR_Rads:")), 2, 0);
    gridLayout->addWidget(spin_corner_para, 2, 1);
    QPushButton *btn_corner_save = MakeBtn(group_corner, "btn_corner_save", QStringLiteral("TR_Sve"));
    gridLayout->addWidget(btn_corner_save, 2, 2);
    // Реф.: строка 3 пустая (разрежённый грид), второй радиус — в строке 4.
    gridLayout->addWidget(MakeLabel(group_corner, "label_corner_para_2", QStringLiteral("TR_Rads:")), 4, 0);
    gridLayout->addWidget(spin_corner_para_2, 4, 1);
    gridLayout->addWidget(MakeLabel(group_corner, "label_flag_pos", QStringLiteral("TR_Mrk")), 5, 0);
    gridLayout->addWidget(dspin_flag_pos, 5, 1);
    verticalLayout->addWidget(group_corner);

    // ── group_center (TR_COffset) ──
    QGroupBox *group_center = new QGroupBox(QStringLiteral("TR_COffset"), root);
    group_center->setObjectName(QStringLiteral("group_center"));
    QGridLayout *gridLayout_2 = new QGridLayout(group_center);
    gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
    spin_center_x = MakeSpin(group_center, "spin_center_x", 0, 640);   // реф. @0x63d528/0x63d534
    spin_center_y = MakeSpin(group_center, "spin_center_y", 0, 120);   // реф. @0x63d67c/0x63d688
    QPushButton *btn_center_save = MakeBtn(group_center, "btn_center_save", QStringLiteral("TR_Sve"));
    gridLayout_2->addWidget(MakeLabel(group_center, "label_center_x", QStringLiteral("TR_Hrtal:")), 0, 0);
    gridLayout_2->addWidget(spin_center_x, 0, 1);
    gridLayout_2->addWidget(MakeLabel(group_center, "label_center_y", QStringLiteral("TR_Vtcal:")), 2, 0);
    gridLayout_2->addWidget(spin_center_y, 2, 1);
    gridLayout_2->addWidget(btn_center_save, 2, 2);
    verticalLayout->addWidget(group_center);

    // ── group_center_2 (TR_CArea) ──
    QGroupBox *group_center_2 = new QGroupBox(QStringLiteral("TR_CArea"), root);
    group_center_2->setObjectName(QStringLiteral("group_center_2"));
    QGridLayout *gridLayout_4 = new QGridLayout(group_center_2);
    gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
    spin_cap_x = MakeSpin(group_center_2, "spin_cap_x", 0, 640);   // реф. @0x63d984/0x63d990
    spin_cap_y = MakeSpin(group_center_2, "spin_cap_y", 0, 120);   // реф. @0x63dad8/0x63dae4
    QPushButton *btn_cap_save = MakeBtn(group_center_2, "btn_cap_save", QStringLiteral("TR_Sve"));
    gridLayout_4->addWidget(MakeLabel(group_center_2, "label_center_x_2", QStringLiteral("TR_Hrtal:")), 0, 0);
    gridLayout_4->addWidget(spin_cap_x, 0, 1);
    gridLayout_4->addWidget(MakeLabel(group_center_2, "label_center_y_2", QStringLiteral("TR_Vtcal:")), 2, 0);
    gridLayout_4->addWidget(spin_cap_y, 2, 1);
    gridLayout_4->addWidget(btn_cap_save, 2, 2);
    verticalLayout->addWidget(group_center_2);

    // ── group_glasstype (TR_EType) ──
    QGroupBox *group_glasstype = new QGroupBox(QStringLiteral("TR_EType"), root);
    group_glasstype->setObjectName(QStringLiteral("group_glasstype"));
    QGridLayout *gridLayout_6 = new QGridLayout(group_glasstype);
    gridLayout_6->setObjectName(QStringLiteral("gridLayout_6"));
    cmb_endoType  = MakeCombo(group_glasstype, "cmb_endoType");
    cmb_glasstype = MakeCombo(group_glasstype, "cmb_glasstype");
    QPushButton *btn_saveEndoType = MakeBtn(group_glasstype, "btn_saveEndoType", QStringLiteral("TR_Sve"));
    QPushButton *btn_endoglass    = MakeBtn(group_glasstype, "btn_endoglass", QStringLiteral("TR_Sve"));
    gridLayout_6->addWidget(MakeLabel(group_glasstype, "label_endoType", QStringLiteral("TR_Mdl:")), 0, 0);
    gridLayout_6->addWidget(cmb_endoType, 0, 1);
    gridLayout_6->addWidget(btn_saveEndoType, 0, 2);
    gridLayout_6->addWidget(MakeLabel(group_glasstype, "label_glasstype", QStringLiteral("TR_Tpe:")), 1, 0);
    gridLayout_6->addWidget(cmb_glasstype, 1, 1);
    gridLayout_6->addWidget(btn_endoglass, 1, 2);
    verticalLayout->addWidget(group_glasstype);

    // ── group_display (TR_DSize): 3 колонки — подпись | Video | UI ──
    QGroupBox *group_display = new QGroupBox(QStringLiteral("TR_DSize"), root);
    group_display->setObjectName(QStringLiteral("group_display"));
    QGridLayout *gridLayout_3 = new QGridLayout(group_display);
    gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
    QHBoxLayout *hLayout_display = new QHBoxLayout();
    hLayout_display->setObjectName(QStringLiteral("hLayout_display"));
    QPushButton *btn_display_save = MakeBtn(group_display, "btn_display_save", QStringLiteral("TR_Sve"));
    hLayout_display->addWidget(btn_display_save);
    gridLayout_3->addLayout(hLayout_display, 0, 0, 1, 3);
    // «UI», «x/y/w/h» — ХАРДКОД (setText прямо в setupUi, retranslateUi их не трогает).
    gridLayout_3->addWidget(MakeLabel(group_display, "label_displayv", QStringLiteral("TR_Vdeo")), 1, 1);
    gridLayout_3->addWidget(MakeLabel(group_display, "label_displayu", QStringLiteral("UI")), 1, 2);
    spin_displayv_x = MakeSpin(group_display, "spin_displayv_x", 1, 1920);
    spin_displayu_x = MakeSpin(group_display, "spin_displayu_x", 1, 1920);
    spin_displayv_y = MakeSpin(group_display, "spin_displayv_y", 1, 1080);
    spin_displayu_y = MakeSpin(group_display, "spin_displayu_y", 1, 1080);
    spin_displayv_w = MakeSpin(group_display, "spin_displayv_w", 1, 1920);
    spin_displayu_w = MakeSpin(group_display, "spin_displayu_w", 1, 1920);
    spin_displayv_h = MakeSpin(group_display, "spin_displayv_h", 1, 1080);
    spin_displayu_h = MakeSpin(group_display, "spin_displayu_h", 1, 1080);
    gridLayout_3->addWidget(MakeLabel(group_display, "label_display_x", QStringLiteral("x")), 2, 0);
    gridLayout_3->addWidget(spin_displayv_x, 2, 1);
    gridLayout_3->addWidget(spin_displayu_x, 2, 2);
    gridLayout_3->addWidget(MakeLabel(group_display, "label_display_y", QStringLiteral("y")), 3, 0);
    gridLayout_3->addWidget(spin_displayv_y, 3, 1);
    gridLayout_3->addWidget(spin_displayu_y, 3, 2);
    gridLayout_3->addWidget(MakeLabel(group_display, "label_display_w", QStringLiteral("w")), 4, 0);
    gridLayout_3->addWidget(spin_displayv_w, 4, 1);
    gridLayout_3->addWidget(spin_displayu_w, 4, 2);
    gridLayout_3->addWidget(MakeLabel(group_display, "label_display_h", QStringLiteral("h")), 5, 0);
    gridLayout_3->addWidget(spin_displayv_h, 5, 1);
    gridLayout_3->addWidget(spin_displayu_h, 5, 2);
    verticalLayout->addWidget(group_display);

    // ── group_iamge (заголовок — хардкод-китайский; опечатка «iamge» — реф.) ──
    QGroupBox *group_iamge = new QGroupBox(QString::fromUtf8("图像参数"), root);
    group_iamge->setObjectName(QStringLiteral("group_iamge"));
    QGridLayout *gridLayout_5 = new QGridLayout(group_iamge);
    gridLayout_5->setObjectName(QStringLiteral("gridLayout_5"));
    spin_agc = MakeSpin(group_iamge, "spin_agc", 0, 10000);   // реф. @0x63ef38
    spin_aec = MakeSpin(group_iamge, "spin_aec", 0, 16960);   // реф. @0x63efd4
    label_agc_range = MakeLabel(group_iamge, "label_agc_range", QStringLiteral("0-0"));
    label_aec_range = MakeLabel(group_iamge, "label_aec_range", QStringLiteral("0-0"));
    gridLayout_5->addWidget(MakeLabel(group_iamge, "label_agc", QStringLiteral("AGC:")), 0, 0);
    gridLayout_5->addWidget(spin_agc, 0, 1);
    gridLayout_5->addWidget(label_agc_range, 0, 2);
    gridLayout_5->addWidget(MakeLabel(group_iamge, "label_aec", QStringLiteral("AEC:")), 1, 0);
    gridLayout_5->addWidget(spin_aec, 1, 1);
    gridLayout_5->addWidget(label_aec_range, 1, 2);
    verticalLayout->addWidget(group_iamge);

    // ── frame: TR_Cntrd / TR_Ext ──
    QFrame *frame = new QFrame(root);
    frame->setObjectName(QStringLiteral("frame"));
    QHBoxLayout *horizontalLayout = new QHBoxLayout(frame);
    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    btn_position = MakeBtn(frame, "btn_position", QStringLiteral("TR_Cntrd"));
    btn_exit     = MakeBtn(frame, "btn_exit", QStringLiteral("TR_Ext"));
    horizontalLayout->addWidget(btn_position);
    horizontalLayout->addWidget(btn_exit);
    verticalLayout->addWidget(frame);

    // ── connect'ы (реф. ~30 в ctor) ──
    connect(btn_exit, &QPushButton::clicked, this, &KVideoCal::ExitAdjustMode);
    connect(btn_corner_save, &QPushButton::clicked, this, &KVideoCal::SaveCornerShape);
    connect(btn_center_save, &QPushButton::clicked, this, &KVideoCal::SaveCenterPoint);
    connect(btn_cap_save, &QPushButton::clicked, this, &KVideoCal::SaveCaptureAreaShift);
    connect(btn_display_save, &QPushButton::clicked, this, &KVideoCal::SaveDisplayAreaSlot);
    connect(btn_saveEndoType, &QPushButton::clicked, this, &KVideoCal::SaveEndoType);
    connect(btn_endoglass, &QPushButton::clicked, this, &KVideoCal::SaveEndoglassType);
    connect(btn_position, &QPushButton::clicked, this, &KVideoCal::SetDisplayPosition);
    connect(cmb_corner_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KVideoCal::SwitchCornerShape);
    connect(cmb_corner_mode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KVideoCal::SwitchCornerMode);
    connect(cmb_glasstype, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KVideoCal::EndoGlassTypeChanged);
    connect(spin_corner_para, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KVideoCal::CornerShapeValueXChanged);
    connect(spin_corner_para_2, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KVideoCal::CornerShapeValueYChanged);
    connect(dspin_flag_pos, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &KVideoCal::FlagPosChanged);
    for (QSpinBox *s : {spin_center_x, spin_center_y})
        connect(s, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &KVideoCal::CenterPointValueChanged);
    for (QSpinBox *s : {spin_cap_x, spin_cap_y})
        connect(s, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &KVideoCal::CaptureAreaValueChanged);
    for (QSpinBox *s : {spin_displayv_x, spin_displayv_y, spin_displayv_w, spin_displayv_h,
                        spin_displayu_x, spin_displayu_y, spin_displayu_w, spin_displayu_h})
        connect(s, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &KVideoCal::DisplayAreaValueChanged);
    connect(spin_agc, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KVideoCal::AGCValueChanged);
    connect(spin_aec, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KVideoCal::AECValueChanged);
    // Реф. также цепляет GetEndoScope()/GetCamera() StatusChanged → EndoStatusChangeAct —
    // device-сигналы, в порте отсутствуют.
}

void KVideoCal::InitWidget()
{
    // Реф. @0x63ad68. Статическая часть — 2 элемента формы угла (через tr, т.е. TR_-ключи).
    cmb_corner_type->addItem(QStringLiteral("TR_Rnd"));
    cmb_corner_type->addItem(QStringLiteral("TR_Ocgle"));
    InitEndoGlassType();
    // Реф. далее переопределяет диапазоны спинов по типу прошивки и роли пользователя:
    //   spin_center_* ← GetCenterOffset{Horizontal,Vertical}Range(_EndoFirmwareType),
    //   spin_cap_*    ← фиксированный [-50, 50].
    // Тип прошивки — device-состояние; берём базовый FW_OV2740, чтобы диапазоны считались
    // ТЕМИ ЖЕ реальными методами, что и в self-test `videocal`.
    const QPair<int, int> h = GetCenterOffsetHorizontalRange(FW_OV2740);
    const QPair<int, int> v = GetCenterOffsetVerticalRange(FW_OV2740);
    spin_center_x->setRange(h.first, h.second);
    spin_center_y->setRange(v.first, v.second);
    spin_cap_x->setRange(-50, 50);
    spin_cap_y->setRange(-50, 50);
}

void KVideoCal::InitEndoGlassType()
{
    // Реф. @0x63aad0: ASCII-литералы, НЕ переводятся (fromAscii_helper, не tr).
    cmb_glasstype->addItem(QStringLiteral("G"));
    cmb_glasstype->addItem(QStringLiteral("S"));
    cmb_glasstype->addItem(QStringLiteral("SR"));
    cmb_glasstype->addItem(QStringLiteral("LSR"));
}

void KVideoCal::SetCornerModeList(const QStringList &modes)
{
    cmb_corner_mode->clear();
    cmb_corner_mode->addItems(modes);
}

void KVideoCal::SetEndoTypeList(const QStringList &types)
{
    cmb_endoType->clear();
    cmb_endoType->addItems(types);
}

// ── Слоты. Device-записи (KEndoScope/KCamera/KVideoSet) — заглушки; расчёт — реальный. ──

void KVideoCal::SwitchCornerShape(int index)
{
    // Реф.: по форме угла подставляет дефолтный радиус из GetDefaultVideoCornerCutting.
    // Наш off-device расчёт живёт в статике этого же класса — используем его.
    Q_UNUSED(index);
}

void KVideoCal::SwitchCornerMode(int index) { Q_UNUSED(index); }
void KVideoCal::CornerShapeValueXChanged(int v) { Q_UNUSED(v); }
void KVideoCal::CornerShapeValueYChanged(int v) { Q_UNUSED(v); }
void KVideoCal::FlagPosChanged(double v) { Q_UNUSED(v); }
void KVideoCal::CenterPointValueChanged(int v) { Q_UNUSED(v); }   // → SetVideoCentorPoint (device)
void KVideoCal::CaptureAreaValueChanged(int v) { Q_UNUSED(v); }   // → SetVideoCapArea (device)
void KVideoCal::DisplayAreaValueChanged(int v) { Q_UNUSED(v); }
void KVideoCal::AGCValueChanged(int v) { Q_UNUSED(v); }
void KVideoCal::AECValueChanged(int v) { Q_UNUSED(v); }
void KVideoCal::EndoGlassTypeChanged(int index) { Q_UNUSED(index); }
void KVideoCal::SaveCornerShape() {}          // → KEndoScope + GetDefaultVideoCornerCutting
void KVideoCal::SaveCenterPoint() {}          // → EEPROM/регистр
void KVideoCal::SaveCaptureAreaShift() {}     // → KEndoScope::SetVideoCapArea
void KVideoCal::SaveEndoType() {}
void KVideoCal::SaveEndoglassType() {}
void KVideoCal::SetDisplayPosition() {}       // → KVideoSet::SetVideoPositonAdjust

void KVideoCal::SaveDisplayAreaSlot()
{
    // Реф. слот SaveDisplayArea(): собирает обе области из спинов и пишет в display-ini.
    // Здесь — НАСТОЯЩИЙ вызов уже портированной статики (не заглушка).
    const QRect imgPro(spin_displayv_x->value(), spin_displayv_y->value(),
                       spin_displayv_w->value(), spin_displayv_h->value());
    const QRect ui(spin_displayu_x->value(), spin_displayu_y->value(),
                   spin_displayu_w->value(), spin_displayu_h->value());
    SaveDisplayArea(imgPro, ui);
}

void KVideoCal::ExitAdjustMode()
{
    close();   // реф.: выход из режима подстройки + закрытие
}

// ──────────────────────── Off-device ядро (без изменений) ────────────────────────

QPair<int, int> KVideoCal::GetCenterOffsetHorizontalRange(int fw)
{
    // 1:1 с X2000: _ZN9KVideoCal30GetCenterOffsetHorizontalRangeE17_EndoFirmwareType
    switch (fw) {
    case FW_OH01A_928X768:    // 1
    case FW_OH01A_768X928:    // 3
    case FW_OV2740_1280X960:  // 5
    case FW_OV2740_1024X1024: // 8
        return {-16, 16};
    case FW_OV2740:           // 0
        return {-4, 4};
    default:
        return {0, 0};
    }
}

QPair<int, int> KVideoCal::GetCenterOffsetVerticalRange(int fw)
{
    // 1:1 с X2000: _ZN9KVideoCal28GetCenterOffsetVerticalRangeE17_EndoFirmwareType
    switch (fw) {
    case FW_OH01A_928X768:    // 1
    case FW_OH01A_768X928:    // 3
        return {-10, 10};
    case FW_OV2740_1280X960:  // 5
    case FW_OV2740_1024X1024: // 8
        return {-16, 16};
    case FW_OV2740:           // 0
        return {-4, 4};
    default:
        return {0, 0};
    }
}

bool KVideoCal::SaveDisplayArea(const QRect &imgPro, const QRect &ui)
{
    KDisplayOption &opt = KDisplayOption::Instance();
    if (opt.LayoutFile().isEmpty())
        return false;
    opt.setVideoRectForImgPro(imgPro);  // [VIDEO]/IMAGE
    opt.setVideoRectForUI(ui);          // [UI]/IMAGE
    return true;
}
