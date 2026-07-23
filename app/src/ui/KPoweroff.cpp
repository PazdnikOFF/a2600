#include "KPoweroff.h"
#include "KDisplayOption.h"

#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QSpacerItem>
#include <QTimer>

KPoweroff::KPoweroff(QWidget *parent)
    : QDialog(parent)
{
    // Реф. ctor @0x626620: setupUi заинлайнен, порядок сохранён.
    setObjectName(QStringLiteral("KPoweroff"));
    resize(1280, 1024);

    m_grid = new QGridLayout(this);
    m_grid->setObjectName(QStringLiteral("gridLayout"));
    m_grid->setContentsMargins(m_grid->contentsMargins().left(), m_grid->contentsMargins().top(),
                               m_grid->contentsMargins().right(), 100);   // реф. (-1,-1,-1,100)

    m_grid->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 0);

    m_labelLogo = new QLabel(this);
    m_labelLogo->setObjectName(QStringLiteral("label_logo"));
    m_labelLogo->setMinimumSize(580, 120);
    m_labelLogo->setMaximumSize(580, 120);
    m_labelLogo->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);   // реф. 0x84
    m_labelLogo->setText(QString());
    m_grid->addWidget(m_labelLogo, 1, 0, 1, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName(QStringLiteral("progressBar"));
    m_progressBar->setMinimumSize(580, 30);
    m_progressBar->setMaximumSize(580, 30);
    m_progressBar->setValue(0);
    m_grid->addWidget(m_progressBar, 2, 0, 1, 1);

    m_labelProgress = new QLabel(this);
    m_labelProgress->setObjectName(QStringLiteral("label_progress"));
    m_labelProgress->setMinimumSize(580, 60);
    m_labelProgress->setMaximumSize(580, 60);
    m_labelProgress->setText(tr("TR_SDown."));   // реф. строка @0x887078 — с точкой на конце
    m_grid->addWidget(m_labelProgress, 3, 0, 1, 1);

    m_grid->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 0);

    setWindowTitle(tr("TR_Dlg"));
    InitDialogParam();
}

void KPoweroff::InitDialogParam()
{
    // Реф. @0x6263f8.
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    move(0, 0);
    // Реф.: resize по KSystemSet::GetUIResolution() — device-seam, оставляем размер из ctor.

    const QPixmap logo(KDisplayOption::GetThemeQssPath(QStringLiteral("Power_Logo.png")));
    if (!logo.isNull())
        m_labelLogo->setPixmap(logo);

    m_progressBar->setMaximum(20);        // реф. 20 тиков
    m_progressBar->setTextVisible(false);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &KPoweroff::RepaintProgressBar);
    m_timer->start(50);                   // реф. 50 мс
}

bool KPoweroff::CheckProcStatus() const
{
    // Реф. @0x626300: find_pid_by_name("X2000Video"/"X2000Simulator"/"X2000") <= 0 по всем
    // трём. Чистый device-seam (скан /proc прибора) — off-device считаем, что всё завершено.
    return true;
}

void KPoweroff::RepaintProgressBar()
{
    // Реф. @0x626358: счётчик тиков, на 20-м — команда выключения, после — ожидание процессов.
    ++m_count;
    if (m_count == 20)
        emit powerOffRequested();          // реф. SendToMainCtrl(0) + лог «Power off.»
    if (m_count > 20) {
        if (!CheckProcStatus())
            return;                        // процессы живы — бар НЕ двигаем
        m_timer->stop();
        close();
    }
    m_progressBar->setValue(m_count);
}

void SystemPoweroff()
{
    // Реф. @0x626d18: new(nothrow) KPoweroff(nullptr) → exec() → delete.
    KPoweroff dlg;
    dlg.exec();
}
