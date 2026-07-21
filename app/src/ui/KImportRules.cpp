#include "KImportRules.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

KImportRules::KImportRules(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x8188c8: KDialog(parent,false) → заинлайненный setupUi → connectSlotsByName
    // (no-op) → InitConnect → InitWidget (заполнение combo с USB — device, опущено).
    setupUi();
    connect(cb_rules, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(ReadFileToTextEdit(QString)));
    connect(btn_import, &QPushButton::clicked, this, &KImportRules::OnClickImport);
}

void KImportRules::setupUi()
{
    // Реф.: абсолютная геометрия внутри диалога 588×420. Виджеты кладём в ContentArea() под
    // титул-баром KDialog; высота диалога +35 под титул. setStyleSheet НЕ вызывается (реф.).
    setObjectName(QStringLiteral("KImportRules"));
    resize(588, 455);

    QWidget *host = ContentArea();

    label = new QLabel(host);
    label->setObjectName(QStringLiteral("label"));
    label->setGeometry(70, 10, 461, 20);
    // Реф. подсказка (UTF-8): «положите файлы правил в папку rules на USB; можно
    // отредактировать и импортировать».
    label->setText(QString::fromUtf8(
        "\xe5\x9c\xa8U\xe7\x9b\x98rules\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9\xe4\xb8\x8b"
        "\xe6\x94\xbe\xe5\x85\xa5\xe8\xa7\x84\xe5\x88\x99\xe6\x96\x87\xe4\xbb\xb6\xef\xbc\x8c"
        "\xe5\x8f\xaf\xe7\xbc\x96\xe8\xbe\x91\xe5\x90\x8e\xe5\xaf\xbc\xe5\x85\xa5\xe8\xa7\x84\xe5\x88\x99"));

    cb_rules = new QComboBox(host);
    cb_rules->setObjectName(QStringLiteral("cb_rules"));
    cb_rules->setGeometry(80, 40, 411, 22);

    textEdit = new QTextEdit(host);
    textEdit->setObjectName(QStringLiteral("textEdit"));
    textEdit->setGeometry(80, 90, 411, 261);

    btn_import = new QPushButton(host);
    btn_import->setObjectName(QStringLiteral("btn_import"));
    btn_import->setGeometry(260, 360, 75, 23);
    btn_import->setText(tr("Import"));

    // Реф. использует сырой setWindowTitle("Dialog") (uic-плейсхолдер) — SetKStyle/SetTitle НЕ
    // зовёт. У нас титул-бар KDialog показываем с тем же плейсхолдером.
    SetTitle(tr("Dialog"));
}

void KImportRules::ReadFileToTextEdit(const QString & /*name*/)
{
    // Реф.: прочитать выбранный файл правил с USB в textEdit (KUsbDevice) — DEVICE. Заглушка.
}

void KImportRules::OnClickImport()
{
    // Реф.: записать textEdit в system/autotest/logcheck/rulesfile (+USB) — DEVICE. Заглушка.
}
