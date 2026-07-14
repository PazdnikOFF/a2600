#include "ui/KUIDesktop.h"
#include "ui/Theme.h"
#include "ui/KDisplayOption.h"
#include "ui/KImgList.h"
#include "sys/KSystem.h"
#include "sys/KUserSet.h"
#include "video/KViewSoftEndo.h"
#include "video/KVideoParam.h"

#include <QPainter>
#include <QKeyEvent>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QPixmap>

KUIDesktop::KUIDesktop(QWidget *parent) : QWidget(parent)
{
    // Разрешение берём у монитора (мультимонитор поддержан через KSystem/screens).
    QSize ui = KSystem::GetSystemResolution();
    if (ui.width() < 1920) ui = QSize(1920, 1080); // деградация для нестандартных экранов

    // Выбор layout-файла display/IMG*-UI<W><H>.ini под это разрешение и текущий
    // размер картинки эндоскопа (KDisplayOption::CurrentImageSize — из KSoftEndoParam
    // при подключении; по умолчанию 1280×960 = рабочий стол без эндоскопа, скрин 2).
    KDisplayOption &disp0 = KDisplayOption::Instance();
    disp0.SelectLayout(ui, disp0.CurrentImageSize());

    setFixedSize(ui);
    InitUiLayout();
}

void KUIDesktop::InitUiLayout()
{
    const KDisplayOption &disp = KDisplayOption::Instance();

    // Все области — из ini (секция [KUIDesktop]/[UI]/[KImgList]), не хардкод.
    const QRect rLogo    = disp.GetRect("KUIDesktop", "logo");
    const QRect rPatient = disp.GetRect("KUIDesktop", "patientinfo");
    const QRect rOsd     = disp.GetRect("KUIDesktop", "workmode");
    const QRect rImgList = disp.GetRect("KUIDesktop", "imglist");
    const QRect rVideo   = disp.getVideoRectForUI(0);

    // Логотип бренда
    auto *logo = new QLabel(this);
    logo->setGeometry(rLogo);
    logo->setAttribute(Qt::WA_TranslucentBackground);
    logo->setPixmap(QPixmap(theme::brand("LogoTitle.png"))
                        .scaled(rLogo.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // Вьювер видео в области IMAGE
    videoView_ = new KViewSoftEndo(this);
    videoView_->setGeometry(rVideo);

    // Панель пациента
    auto *patient = BuildPatientPanel();
    patient->setParent(this);
    patient->setGeometry(rPatient);

    // OSD-параметры (в области workmode)
    auto *osd = BuildOsdPanel();
    osd->setParent(this);
    osd->setGeometry(rOsd.isValid() ? rOsd : QRect(14, 300, 230, 470));

    // Список снимков (реф. KImgList) в области imglist
    imgList_ = new KImgList(this);
    imgList_->setGeometry(rImgList);
}

void KUIDesktop::paintEvent(QPaintEvent *)
{
    const KDisplayOption &disp = KDisplayOption::Instance();
    QPainter p(this);
    p.fillRect(rect(), Qt::black);                       // фон — чёрный

    const QRect rVideo  = disp.getVideoRectForUI(0);
    const QRect rTopMsg = disp.GetRect("KUIDesktop", "topmsg");
    const QRect rBotMsg = disp.GetRect("KUIDesktop", "bottommsg");
    if (rVideo.isValid()) {
        p.fillRect(rVideo, QColor(0, 0, 0));
        p.setPen(QColor(40, 48, 64));
        p.drawRect(rVideo.adjusted(0, 0, -1, -1));
    }
    if (rTopMsg.isValid()) p.fillRect(rTopMsg, QColor(10, 10, 10));
    if (rBotMsg.isValid()) p.fillRect(rBotMsg, QColor(10, 10, 10));
}

void KUIDesktop::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);   // SingleKeyAct/AltKeyAct — обработка клавиш
}

void KUIDesktop::ShowRigidEndoBtnGuide(bool) {}
void KUIDesktop::OpenWHB() {}
void KUIDesktop::RecLeftTime(int seconds)
{
    if (recLeftTime_)
        recLeftTime_->setText(QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0'))
                                               .arg(seconds % 60, 2, 10, QChar('0')));
}
void KUIDesktop::SetStopWatchState(bool) {}
void KUIDesktop::closeDesktop() { close(); }

QLabel *KUIDesktop::MakeIconLabel(const QString &assetRel, int w, int h)
{
    auto *l = new QLabel(this);
    l->setFixedSize(w, h);
    l->setAttribute(Qt::WA_TranslucentBackground);
    if (!assetRel.isEmpty())
        l->setPixmap(KDisplayOption::Instance().GetIconPixmap(assetRel)
                         .scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    return l;
}

QWidget *KUIDesktop::MakePatientField(const QString &iconRel, const QString &placeholder)
{
    const KDisplayOption &disp = KDisplayOption::Instance();
    auto *f = new QFrame;
    f->setFixedHeight(52);
    f->setStyleSheet("QFrame{border:1px solid rgb(59,59,59); border-radius:4px; background:transparent;}");
    auto *lay = new QHBoxLayout(f);
    lay->setContentsMargins(8, 4, 8, 4);
    lay->setSpacing(8);

    auto *icon = new QLabel(f);
    icon->setFixedSize(22, 22);
    icon->setStyleSheet("border:none;");
    icon->setPixmap(disp.GetIconPixmap(iconRel).scaled(22, 22, Qt::KeepAspectRatio,
                                                       Qt::SmoothTransformation));
    auto *text = new QLabel(placeholder, f);
    text->setStyleSheet("border:none; color:rgb(150,150,150);");

    auto *arrow = new QLabel(f);
    arrow->setFixedSize(16, 16);
    arrow->setStyleSheet("border:none;");
    arrow->setPixmap(disp.GetIconPixmap("combobox/down_arrow.png")
                         .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    lay->addWidget(icon);
    lay->addWidget(text, 1);
    lay->addWidget(arrow);
    return f;
}

QWidget *KUIDesktop::BuildPatientPanel()
{
    auto *w = new QWidget;
    w->setStyleSheet("background:transparent;");
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);
    lay->addWidget(MakePatientField("icon/patient/patient_name.png", "Name"));
    lay->addWidget(MakePatientField("icon/patient/patient_sex.png",  "Gender"));
    lay->addWidget(MakePatientField("icon/patient/patient_age.png",  "Age"));
    lay->addStretch(1);
    return w;
}

QWidget *KUIDesktop::BuildOsdPanel()
{
    auto *w = new QWidget;
    w->setStyleSheet("background:transparent; color:rgb(180,180,180);");
    auto *grid = new QGridLayout(w);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(9);

    auto addRow = [&](int r, const QString &k, const QString &v) {
        auto *lk = new QLabel(k, w);
        lk->setStyleSheet("border:none; color:rgb(150,150,150);");
        auto *lv = new QLabel(v, w);
        lv->setStyleSheet("border:none; color:rgb(210,210,210);");
        grid->addWidget(lk, r, 0, Qt::AlignLeft);
        grid->addWidget(lv, r, 1, Qt::AlignLeft);
    };

    // Конфиг osd.ini (KUserSet) → держатель KVideoParam → OSD (реф. InitVideoParam).
    KUserSet::Instance().InitVideoParamConfig();
    KVideoParam &vp = KVideoParam::Instance();
    vp.InitFromUserConf(KUserSet::Instance().GetVideoParamConfig());
    auto pick = [](int v, std::initializer_list<const char*> opts) {
        const int n = int(opts.size());
        return QString(v >= 0 && v < n ? *(opts.begin() + v) : *opts.begin());
    };

    int r = 0;
    addRow(r++, "IRIS:",    pick(vp.IrisMode(), {"PEAK", "AVE", "AUTO"}));
    addRow(r++, "Zoom:",    QString::number(1.0 + 0.2 * vp.ZoomLevel(), 'f', 1) + "X");
    addRow(r++, "Img Enh:", "A" + QString::number(vp.ImageEnhLevel()));
    addRow(r++, "Col Enh:", QString::number(vp.ColorEnhLevel()));

    auto *rbcK = new QLabel("Col RBC:", w);
    rbcK->setStyleSheet("border:none; color:rgb(150,150,150);");
    grid->addWidget(rbcK, r++, 0, 1, 2, Qt::AlignLeft);
    auto *rbcRow = new QWidget(w);
    rbcRow->setStyleSheet("border:none;");
    auto *rbcLay = new QHBoxLayout(rbcRow);
    rbcLay->setContentsMargins(0, 0, 0, 0);
    rbcLay->setSpacing(10);
    for (const QString &opt : {"15", "0", "-15"}) {
        auto *rb = new QRadioButton(opt, rbcRow);
        if (opt == "15") rb->setChecked(true);
        rbcLay->addWidget(rb);
    }
    rbcLay->addStretch(1);
    grid->addWidget(rbcRow, r++, 0, 1, 2);

    addRow(r++, "AGC:",        "On");    // TODO: из отдельного конфига света
    addRow(r++, "Light Mode:", "EWL");   // TODO: из конфига источника света (Dccu)
    addRow(r++, "Contrast:",   pick(vp.ContrastLevel(), {"L", "M", "H"}));
    grid->setRowStretch(r, 1);
    return w;
}
