#include "KAddPrinterDriverDlg.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

KAddPrinterDriverDlg::KAddPrinterDriverDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x792618: KDialog(modal=false) → setupUi → SetTitle(TR_ADriver) →
    // setStyleSheet(тёмный) → RegisterSignalConnect → Initialize (device: KPrinterManager map).
    setupUi();
    SetTitle(tr("TR_ADriver"));
}

void KAddPrinterDriverDlg::setupUi()
{
    setObjectName(QStringLiteral("KAddPrinterDriverDlg"));
    resize(1044, 636);

    QWidget *host = ContentArea();
    host->setStyleSheet(QStringLiteral("QWidget{background-color: rgba(26,26,26,255);}"));
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout_3"));
    root->setContentsMargins(0, 0, 0, 0);

    // Строка поиска.
    QHBoxLayout *hSearch = new QHBoxLayout();
    QLabel *lblSearch = new QLabel(tr("TR_DSearch"), host);
    lblSearch->setObjectName(QStringLiteral("m_pDriverSearchLabel"));
    lblSearch->setMaximumHeight(40);
    hSearch->addWidget(lblSearch);
    QLineEdit *edSearch = new QLineEdit(host);
    edSearch->setObjectName(QStringLiteral("m_pDriverSearchLedit"));
    edSearch->setFixedHeight(40);
    edSearch->setMaxLength(128);   // реф. maxLen 128
    hSearch->addWidget(edSearch);
    root->addLayout(hSearch);

    // Две колонки-списка.
    QHBoxLayout *hLists = new QHBoxLayout();
    auto listCol = [&](const QString &labelText, const char *labelName,
                       const char *listName, int w) {
        QVBoxLayout *v = new QVBoxLayout();
        QLabel *l = new QLabel(labelText, host);
        l->setObjectName(QString::fromLatin1(labelName));
        l->setFrameShape(QFrame::Panel);   // реф. setFrameShape(Panel)
        l->setMaximumHeight(40);
        v->addWidget(l);
        QListWidget *lw = new QListWidget(host);   // device-populated (KPrinterManager)
        lw->setObjectName(QString::fromLatin1(listName));
        lw->setFixedSize(w, 400);
        lw->setFocusPolicy(Qt::ClickFocus);
        v->addWidget(lw);
        hLists->addLayout(v);
        return lw;
    };
    listCol(tr("TR_Mnfctrr"), "m_pMakerLabel", "m_pMakerListWidget", 400);     // производитель
    listCol(tr("TR_PDriver"), "m_pPrinterLabel", "m_pPrinterListWidget", 600); // драйвер
    root->addLayout(hLists);

    root->addStretch(1);   // реф. verticalSpacer

    // Ряд кнопок.
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->addStretch(1);
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), host);
    btnOk->setObjectName(QStringLiteral("m_pOkBtn"));
    btnOk->setEnabled(false);   // реф. Initialize: OK disabled до выбора
    hBtn->addWidget(btnOk);
    hBtn->addStretch(1);
    QPushButton *btnInstall = new QPushButton(tr("TR_Install From Disk"), host);
    btnInstall->setObjectName(QStringLiteral("m_pInstallBtn"));
    btnInstall->setMinimumWidth(180);   // реф. minW 180
    hBtn->addWidget(btnInstall);        // реф. OnInstallBtnClicked (PPD с диска) — device
    hBtn->addStretch(1);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("m_pCancelBtn"));
    hBtn->addWidget(btnCancel);
    hBtn->addStretch(1);
    root->addLayout(hBtn);

    connect(btnOk, &QPushButton::clicked, this, &QWidget::close);       // реф. OnOkBtnClicked (accept)
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnCancelBtnClicked (reject)
    // maker-list(repopulate)/Install(PPD с диска)/search-filter — DEVICE/данные, не подключаем.
}
