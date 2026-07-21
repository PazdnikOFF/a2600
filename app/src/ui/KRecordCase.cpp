#include "KRecordCase.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

KRecordCase::KRecordCase(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x7340f8: KDialog(modal=false) → setupUi → SetKStyle(2) → title
    // tr("Record Case") → maxLen63 обоих полей → Mkdir (device) → connect Start→RecordCaseName.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("Record Case"));       // реф. перекрывает setupUi-титул "Dialog"
}

void KRecordCase::setupUi()
{
    setObjectName(QStringLiteral("KRecordCase"));

    QWidget *host = ContentArea();
    host->setMinimumSize(320, 240);   // реф. resize 320×240, абсолютная геометрия

    // Форма Module/Case (реф. layoutWidget 30,50,271,61 → gridLayout).
    QWidget *formHost = new QWidget(host);
    formHost->setObjectName(QStringLiteral("layoutWidget"));
    formHost->setGeometry(30, 50, 271, 61);
    QGridLayout *g = new QGridLayout(formHost);
    g->setContentsMargins(0, 0, 0, 0);
    QLabel *lblMod = new QLabel(tr("Module Name:"), formHost);
    lblMod->setObjectName(QStringLiteral("label_2"));
    g->addWidget(lblMod, 0, 0);
    QLineEdit *edMod = new QLineEdit(formHost);
    edMod->setObjectName(QStringLiteral("edt_moudlename"));   // реф. опечатка сохранена
    edMod->setMaxLength(63);
    g->addWidget(edMod, 0, 1);
    QLabel *lblCase = new QLabel(tr("Case Name:"), formHost);
    lblCase->setObjectName(QStringLiteral("label"));
    g->addWidget(lblCase, 1, 0);
    QLineEdit *edCase = new QLineEdit(formHost);
    edCase->setObjectName(QStringLiteral("edt_casename"));
    edCase->setMaxLength(63);
    g->addWidget(edCase, 1, 1);

    // Чекбокс Record TimeStamp (реф. 60,130,190,22, checked).
    QCheckBox *ck = new QCheckBox(tr("Record TimeStamp"), host);
    ck->setObjectName(QStringLiteral("ck_timestamp"));
    ck->setGeometry(60, 130, 190, 22);
    ck->setChecked(true);

    // Кнопка Start Record (реф. 106,180,110,22).
    QPushButton *btn = new QPushButton(tr("Start Record"), host);
    btn->setObjectName(QStringLiteral("bt_startrecord"));
    btn->setGeometry(106, 180, 110, 22);
    // btn → RecordCaseName (запись кейса в ФС) — DEVICE, не подключаем.
}
