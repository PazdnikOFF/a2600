#include "KExamDetailInfoUi.h"
#include "KImgPushButton.h"
#include "sys/KEnvConfig.h"

#include <QDir>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

KExamDetailInfoUi::KExamDetailInfoUi(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x495c98: KFullScreenDialog(parent,2004) → setupUi → SetTitle(TR_Dtls) →
    // ResizeExamDetail → SubscribeMsg(0x2f07/0x2f0b) (device) → connects.
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);
    SetTitle(tr("TR_Dtls"));
}

void KExamDetailInfoUi::setupUi()
{
    setObjectName(QStringLiteral("KExamDetailInfoUi"));
    resize(1280, 960);

    QWidget *host = ContentArea();
    QHBoxLayout *root = new QHBoxLayout(host);
    root->setObjectName(QStringLiteral("horizontalLayout"));
    root->setContentsMargins(0, 45, 0, 0);
    root->setSpacing(0);

    // ===================== Левая колонка действий =====================
    QWidget *left = new QWidget(host);
    left->setObjectName(QStringLiteral("widget_left_opts"));
    left->setFixedWidth(260);
    QVBoxLayout *vL = new QVBoxLayout(left);
    vL->setContentsMargins(9, 18, 9, 18);
    auto bigBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, left);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedSize(212, 212);   // реф. 212²
        vL->addWidget(b);
        return b;
    };
    bigBtn("btn_report", tr("TR_Rprt"));           // реф. OpenReportEdit (device)
    bigBtn("btn_export", tr("TR_Expt"));           // реф. OnBtnExportClicked (device)
    bigBtn("btn_dicom_upload", tr("TR_Upld"));     // реф. OnBtnUploadClicked (device)
    bigBtn("btn_delete", tr("TR_Del"));            // реф. OpenDeleteDlg (device)
    vL->addStretch(1);
    QLabel *lblVol = new QLabel(left);             // реф. label_disk_vol (объём диска, device)
    lblVol->setObjectName(QStringLiteral("label_disk_vol"));
    lblVol->setFixedSize(250, 250);
    lblVol->setAlignment(Qt::AlignCenter);
    vL->addWidget(lblVol);
    vL->addStretch(1);
    QPushButton *btnExit = bigBtn("btn_exit", tr("TR_Ext"));
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnBtnExitClicked→close
    root->addWidget(left);

    // ===================== Правая колонка =====================
    QVBoxLayout *vR = new QVBoxLayout();
    vR->setObjectName(QStringLiteral("verticalLayout_right_contents"));

    // Шапка: имя пациента + дата.
    QHBoxLayout *hTop = new QHBoxLayout();
    hTop->setContentsMargins(12, 9, 12, 9);
    QLabel *lblName = new QLabel(tr("TR_Nme:"), host);
    lblName->setObjectName(QStringLiteral("label_patient_name")); lblName->setMinimumWidth(300);
    hTop->addWidget(lblName);
    QLabel *lblDate = new QLabel(tr("TR_EmDate:"), host);
    lblDate->setObjectName(QStringLiteral("label_exam_date"));
    hTop->addWidget(lblDate);
    hTop->addStretch(1);
    vR->addLayout(hTop);

    // Таблица файлов обследования (реф. widget_tableView — device-модель).
    QTableView *table = new QTableView(host);
    table->setObjectName(QStringLiteral("widget_tableView"));
    table->horizontalHeader()->setStretchLastSection(true);
    vR->addWidget(table, 1);

    // Нижний play-bar.
    QHBoxLayout *hBot = new QHBoxLayout();
    hBot->setContentsMargins(12, 9, 12, 9);
    QLabel *lblTotal = new QLabel(tr("TR_Ttl:"), host); lblTotal->setObjectName(QStringLiteral("label_total_num"));
    hBot->addWidget(lblTotal);
    QLabel *lblHint = new QLabel(tr("TR_SCAPage"), host); lblHint->setObjectName(QStringLiteral("label_hint"));
    hBot->addWidget(lblHint);
    hBot->addStretch(1);
    // Пагинация. АПГРЕЙД: реальный KImgPushButton (был QPushButton с глифами). Реверс: setupUi
    // создаёт 4× KImgPushButton (виджет 48×48), InitWidget зовёт InitButtons(normal, hover,
    // checked=hover, disable, QSize(48,32), false). База = GetReadOnlyBaseDir + PAGE_TURNING_ICON_DIR
    // ("mainapp/application/qss/icon/pageturning/"). Стем btn_pre = page_front (НЕ page_pre!).
    const QString pgBase = QDir(QString::fromStdString(KEnvConfig::GetInstance().GetReadOnlyBaseDir()))
                               .absoluteFilePath(QStringLiteral("mainapp/application/qss/icon/pageturning/"));
    auto pageBtn = [&](const char *name, const QString &stem) {
        KImgPushButton *b = new KImgPushButton(host);
        b->setObjectName(QString::fromLatin1(name));
        b->InitButtons(pgBase + "page_" + stem + "_normal.png", pgBase + "page_" + stem + "_hover.png",
                       pgBase + "page_" + stem + "_hover.png", pgBase + "page_" + stem + "_disable.png",
                       QSize(48, 32), false);   // реф. QSize(48,32), bool=false
        hBot->addWidget(b);
        return b;
    };
    pageBtn("btn_head", QStringLiteral("head"));   // реф. OnBtnHeadClicked (device-модель)
    pageBtn("btn_pre", QStringLiteral("front"));   // реф. стем page_front_* (не page_pre)
    QLineEdit *edPage = new QLineEdit(host);
    edPage->setObjectName(QStringLiteral("edit_page"));
    edPage->setFixedSize(48, 48);
    edPage->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("[0-9]{0,5}")), edPage));
    hBot->addWidget(edPage);
    QLabel *lblTotalPage = new QLabel(QStringLiteral("/1"), host);   // реф. литерал «/1»
    lblTotalPage->setObjectName(QStringLiteral("label_total_page"));
    lblTotalPage->setFixedSize(48, 48); lblTotalPage->setAlignment(Qt::AlignCenter);
    hBot->addWidget(lblTotalPage);
    pageBtn("btn_next", QStringLiteral("next"));
    pageBtn("btn_tail", QStringLiteral("tail"));
    hBot->addStretch(1);
    vR->addLayout(hBot);

    root->addLayout(vR, 1);
}
