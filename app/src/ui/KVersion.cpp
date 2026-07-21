#include "KVersion.h"

#include "sys/KVersionConfig.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>

KVersion::KVersion(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x6ebd90: setupUi → SetKStyle(2) → title TR_Vson → ShowSoftwareVersion →
    // ShowSoftwareVersionCheck (check-иконки, только сервис-роль — device, опущено) → connect.
    setupUi();
    SetKStyle(KDLG_W460);         // реф. SetKStyle(2)
    SetTitle(tr("TR_Vson"));      // реф. перекрывает TR_Dlg
    showSoftwareVersion();
}

namespace {
struct VerRow { const char *base; const char *cap; const char *key; bool hasCheck; };
// row-порядок как в реф. (papp05 отсутствует; 0-3 без колонки check).
const VerRow kRows[] = {
    {"product",  "TR_PModel:", "",       false},
    {"release",  "TR_Rlse:",   "",       false},
    {"complete", "TR_Cmplte:", "@complete", false},
    {"app",      "TR_App:",    "@app",   false},
    {"kernel",   "TR_Krnl:",   "@kernel", true},
    {"hmi",      "TR_HMI:",    "hmi",    true},
    {"panel",    "TR_Pnl:",    "panel",  true},
    {"pap",      "PAP:",       "pap",    true},
    {"pas",      "PAS:",       "pas",    true},
    {"papp00",   "PAPP00:",    "papp00", true},
    {"papp01",   "PAPP01:",    "papp01", true},
    {"papp02",   "PAPP02:",    "papp02", true},
    {"papp03",   "PAPP03:",    "papp03", true},
    {"papp04",   "PAPP04:",    "papp04", true},
    {"papp06",   "PAPP06:",    "papp06", true},
    {"papp07",   "PAPP07:",    "papp07", true},
    {"papp80",   "PAPP80:",    "papp80", true},
    {"lcd",      "LCD:",       "lcd",    true},
    {"cam",      "CAM:",       "camera", true},
};
} // namespace

void KVersion::setupUi()
{
    setObjectName(QStringLiteral("KVersion"));
    resize(296, 769);   // реф. 296×734; +35 под титул (ширину задаёт W460)

    QWidget *host = ContentArea();
    QGridLayout *g2 = new QGridLayout(host);
    g2->setObjectName(QStringLiteral("gridLayout_2"));

    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("frame"));
    g2->addWidget(frame, 0, 0);

    QGridLayout *g = new QGridLayout(frame);
    g->setObjectName(QStringLiteral("gridLayout"));

    int row = 0;
    for (const VerRow &r : kRows) {
        if (r.hasCheck) {
            QLabel *chk = new QLabel(frame);
            chk->setObjectName(QString::fromLatin1(r.base) + QStringLiteral("_check"));
            chk->setFixedWidth(20);   // место под иконку статуса
            g->addWidget(chk, row, 0);
        }
        QLabel *cap = new QLabel(frame);
        cap->setObjectName(QString::fromLatin1(r.base) + QStringLiteral("_cap"));
        cap->setText(tr(r.cap));
        g->addWidget(cap, row, 1);
        QLabel *val = new QLabel(frame);
        val->setObjectName(QStringLiteral("label_") + QString::fromLatin1(r.base));
        val->setStyleSheet(QStringLiteral("color: rgb(166, 176, 187);"));   // реф. стиль значения
        g->addWidget(val, row, 2);
        ++row;
    }

    // --- кнопка Exit ---
    QFrame *frame1 = new QFrame(host);
    frame1->setObjectName(QStringLiteral("frame1"));
    g2->addWidget(frame1, 2, 0, 1, 2);
    QHBoxLayout *hb = new QHBoxLayout(frame1);
    hb->setObjectName(QStringLiteral("horizontalLayout"));
    QPushButton *btn_exit = new QPushButton(frame1);
    btn_exit->setObjectName(QStringLiteral("btn_exit"));
    btn_exit->setText(tr("TR_Ext"));
    hb->addWidget(btn_exit);
    connect(btn_exit, &QPushButton::clicked, this, &QWidget::close);

    g2->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0);
}

void KVersion::showSoftwareVersion()
{
    // Реф. @0x6e9050: значения версий (col2) из KVersionConfig. product/release тянутся из
    // KSystemSet::GetProductModel/GetProductRelaseVersion (у нас нет — плейсхолдер "—").
    KVersionConfig &cfg = KVersionConfig::GetInstance();
    for (const VerRow &r : kRows) {
        QLabel *val = findChild<QLabel *>(QStringLiteral("label_") + QString::fromLatin1(r.base));
        if (!val)
            continue;
        QString v;
        const QString key = QString::fromLatin1(r.key);
        if (key.isEmpty())            v = QString(QChar(0x2014));   // "—" (product/release)
        else if (key == "@complete")  v = cfg.GetCompleteVersion();
        else if (key == "@app")       v = cfg.GetAppSoftwareVersion();
        else if (key == "@kernel")    v = cfg.GetKernelVersion();
        else                          v = cfg.GetVersion(key);
        if (v.isEmpty())
            v = QString(QChar(0x2014));
        val->setText(v);
    }
}
