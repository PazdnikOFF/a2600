#include "KUpdatePrepare.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QSpacerItem>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

KUpdatePrepare::KUpdatePrepare(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x6e2130: KDialog(parent,false) → setupUi → SetKStyle(1) → title TR_Ugde →
    // initWidget → connect(btn_update.clicked → StartUpdate).
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);      // реф. SetKStyle(1)
    SetTitle(tr("TR_Ugde"));         // реф. перекрывает TR_Dlg
    initWidget();
    timer = new QTimer(this);
    connect(btn_update, &QToolButton::clicked, this, &KUpdatePrepare::StartUpdate);
    connect(timer, &QTimer::timeout, this, &KUpdatePrepare::UpdateProgressDec);
}

void KUpdatePrepare::setupUi()
{
    // Реф. Ui_KUpdatePrepare::setupUi @0x6e2410. Одна QGridLayout, разрежённые строки,
    // спейсеры центрируют/разносят. Абсолютной геометрии и setStyleSheet НЕТ.
    setObjectName(QStringLiteral("KUpdatePrepare"));
    resize(800, 600);

    // Контент кладём в область под титул-баром KDialog (реф. подкласс — на сам диалог).
    QWidget *host = ContentArea();
    QGridLayout *grid = new QGridLayout(host);
    grid->setObjectName(QStringLiteral("gridLayout"));

    grid->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding),
                  1, 1, 1, 1);

    frame_update = new QFrame(this);
    frame_update->setObjectName(QStringLiteral("frame_update"));
    frame_update->setMinimumSize(300, 50);
    frame_update->setFrameShape(QFrame::StyledPanel);
    frame_update->setFrameShadow(QFrame::Raised);
    QHBoxLayout *hbox = new QHBoxLayout(frame_update);
    hbox->setObjectName(QStringLiteral("horizontalLayout_2"));
    hbox->setContentsMargins(0, 0, 0, 0);
    btn_update = new QToolButton(frame_update);
    btn_update->setObjectName(QStringLiteral("btn_update"));
    btn_update->setMinimumSize(150, 40);
    btn_update->setMaximumSize(150, 40);       // фикс 150×40
    btn_update->setText(tr("TR_Ugde"));
    hbox->addWidget(btn_update);
    grid->addWidget(frame_update, 2, 1, 1, 1);

    grid->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum),
                  2, 0, 1, 1);   // horizontalSpacer_2 (слева)
    grid->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum),
                  2, 2, 1, 1);   // horizontalSpacer (справа)

    label_msg = new QLabel(this);
    label_msg->setObjectName(QStringLiteral("label_msg"));
    label_msg->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    label_msg->setText(QString());   // пусто; заполняется в рантайме
    grid->addWidget(label_msg, 6, 1, 1, 1);

    progress_rar = new QProgressBar(this);
    progress_rar->setObjectName(QStringLiteral("progress_rar"));
    progress_rar->setMaximumSize(600, QWIDGETSIZE_MAX);   // ширина ≤ 600
    progress_rar->setValue(24);                           // реф. setValue(24) в setupUi
    progress_rar->setFormat(QStringLiteral("%p%"));
    grid->addWidget(progress_rar, 7, 1, 1, 1);

    grid->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding),
                  9, 1, 1, 1);
    // Титул ставит ctor через KDialog::SetTitle (реф. перекрывает TR_Dlg на TR_Ugde).
}

void KUpdatePrepare::initWidget()
{
    // Реф. @0x6e0d78: прячет прогресс-бар, value 0, max 40.
    progress_rar->setVisible(false);
    progress_rar->setValue(0);
    progress_rar->setMaximum(40);
}

void KUpdatePrepare::StartUpdate()
{
    // Реф. @0x6e1f70: определить путь пакета на USB → StartDecompress; ошибка → сообщение +
    // reconnect кнопки. USB-детект/путь пакета — DEVICE; здесь превью-заглушка идёт по успеху.
    label_msg->setText(tr("TR_UPreparing."));
    StartDecompress();
}

void KUpdatePrepare::StartDecompress()
{
    // Реф. @0x6e1b08: ExecRarCmd (unrar пакета) + QTimer::start. unrar/fs — DEVICE; здесь
    // только таймер прогресса для превью.
    progress_rar->setVisible(true);
    timer->start(200);
}

void KUpdatePrepare::UpdateProgressDec()
{
    // Реф. @0x6e15a0: инкремент прогресса распаковки.
    int v = progress_rar->value() + 1;
    if (v >= progress_rar->maximum()) {
        v = progress_rar->maximum();
        timer->stop();
    }
    progress_rar->setValue(v);
}
