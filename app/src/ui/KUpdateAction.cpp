#include "KUpdateAction.h"

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpacerItem>
#include <QTimer>
#include <QWidget>

KUpdateAction::KUpdateAction(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x6e6170: setupUi → SetKStyle(1) → title TR_Ugde → new KUpdateConf (device) →
    // initWidget → connect'ы → UpdateCheck. KUpdateConf/UpdateCheck (device) опущены.
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);   // реф. SetKStyle(1)
    SetTitle(tr("TR_Ugde"));      // реф. перекрывает TR_Dlg
    initWidget();

    QTimer *timer = new QTimer(this);
    connect(btn_start, &QPushButton::clicked, this, &KUpdateAction::StartUpdate);
    connect(btn_poweroff, &QPushButton::clicked, this, &KUpdateAction::ClickBtnPoweroff);
    connect(timer, &QTimer::timeout, this, &KUpdateAction::UpdateProgressUpd);
}

void KUpdateAction::setupUi()
{
    // Реф. Ui_KUpdateAction::setupUi @0x6e71e0. Корневой QGridLayout; два вложенных QFrame
    // (frame_item со списком разделов, frame_btn с кнопками). setStyleSheet НЕТ.
    setObjectName(QStringLiteral("KUpdateAction"));
    resize(800, 839);

    QWidget *host = ContentArea();
    QGridLayout *grid = new QGridLayout(host);
    grid->setObjectName(QStringLiteral("gridLayout"));

    // --- frame_item (2,1,2,1): список разделов «чекбокс + версия» ---
    frame_item = new QFrame(host);
    frame_item->setObjectName(QStringLiteral("frame_item"));
    frame_item->setFrameShape(QFrame::StyledPanel);
    frame_item->setFrameShadow(QFrame::Raised);
    QGridLayout *grid2 = new QGridLayout(frame_item);
    grid2->setObjectName(QStringLiteral("gridLayout_2"));

    // (objectName-суффикс, текст чекбокса, строка). row 2 пуст (зазор); PAPP05 отсутствует.
    struct Sec { const char *suf; const char *text; int row; };
    const Sec sections[] = {
        {"app", "TR_App", 0}, {"hmi", "TR_HMI", 1}, {"pap", "PAP", 3},
        {"papp00", "PAPP00", 4}, {"papp01", "PAPP01", 5}, {"papp02", "PAPP02", 6},
        {"papp03", "PAPP03", 7}, {"papp04", "PAPP04", 8}, {"papp06", "PAPP06", 9},
        {"papp07", "PAPP07", 10}, {"papp80", "PAPP80", 11}, {"lcd", "LCD", 12},
    };
    for (const Sec &s : sections) {
        QCheckBox *chk = new QCheckBox(frame_item);
        chk->setObjectName(QStringLiteral("chk_") + QLatin1String(s.suf));
        // TR_App/TR_HMI — переводимые; PAP/PAPPxx/LCD — литералы (реф. так же).
        chk->setText(tr(s.text));
        chk->setMinimumWidth(100);
        grid2->addWidget(chk, s.row, 0, 1, 1);
        QLabel *ver = new QLabel(frame_item);
        ver->setObjectName(QStringLiteral("label_") + QLatin1String(s.suf));
        ver->setMinimumWidth(15);   // текст версии — device (пусто)
        grid2->addWidget(ver, s.row, 1, 1, 1);
    }
    grid->addWidget(frame_item, 2, 1, 2, 1);

    // --- сообщения / прогресс ---
    label_msg = new QLabel(host);
    label_msg->setObjectName(QStringLiteral("label_msg"));
    label_msg->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    grid->addWidget(label_msg, 9, 1, 1, 1);

    progress_upd = new QProgressBar(host);
    progress_upd->setObjectName(QStringLiteral("progress_upd"));
    progress_upd->setMaximumWidth(600);
    progress_upd->setValue(24);
    grid->addWidget(progress_upd, 10, 1, 1, 1);

    // --- frame_btn (11,1): кнопки Start / PowerOff ---
    frame_btn = new QFrame(host);
    frame_btn->setObjectName(QStringLiteral("frame_btn"));
    frame_btn->setFrameShape(QFrame::StyledPanel);
    frame_btn->setFrameShadow(QFrame::Raised);
    QHBoxLayout *hbtn = new QHBoxLayout(frame_btn);
    hbtn->setObjectName(QStringLiteral("horizontalLayout_2"));
    hbtn->setSpacing(0);
    hbtn->setContentsMargins(0, 0, 0, 0);
    btn_start = new QPushButton(frame_btn);
    btn_start->setObjectName(QStringLiteral("btn_start"));
    btn_start->setFixedWidth(150);
    btn_start->setText(tr("TR_Strt"));
    hbtn->addWidget(btn_start);
    btn_poweroff = new QPushButton(frame_btn);
    btn_poweroff->setObjectName(QStringLiteral("btn_poweroff"));
    btn_poweroff->setFixedWidth(150);
    btn_poweroff->setText(tr("TR_POff"));
    hbtn->addWidget(btn_poweroff);
    grid->addWidget(frame_btn, 11, 1, 1, 1);

    label_updmsg = new QLabel(host);
    label_updmsg->setObjectName(QStringLiteral("label_updmsg"));
    label_updmsg->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    grid->addWidget(label_updmsg, 12, 1, 1, 1);

    // --- распорки (реф. addItem): центрируют/разносят контент ---
    grid->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 1, 1, 2);
    grid->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 3, 0, 1, 1);
    grid->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 3, 3, 1, 1);
    grid->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 8, 1, 1, 2);
    grid->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 14, 1, 1, 2);
}

void KUpdateAction::initWidget()
{
    // Реф. @0x6e2cb8: стартово скрыты прогресс и poweroff; версии/чек-состояния/видимость
    // PAPP/LCD наполняются с устройства (KUpdateConf::GetUpdateVersion, KProjectSet::IsShowPAPP,
    // GetSystemStatus) — DEVICE, здесь не воспроизводим (в превью показываем все разделы).
    progress_upd->setVisible(false);
    btn_poweroff->setVisible(false);
}

void KUpdateAction::StartUpdate()
{
    // Реф.: запись выбранных разделов через KUpdateConf → KHalClass::execUpdateAction — DEVICE.
    progress_upd->setVisible(true);
}

void KUpdateAction::ClickBtnPoweroff()
{
    // Реф.: выключение прибора — DEVICE. Заглушка.
}

void KUpdateAction::UpdateProgressUpd()
{
    // Реф.: тик прогресса прошивки. Заглушка.
}
