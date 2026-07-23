#include "KOsdSpin02.h"

#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

// Стили спина — дословно из .rodata (длины совпадают с `mov w1` реф.).
static const char *kSpinFocused = "QSpinBox { background-color: rgb(85,85,85); color: rgb(0,205,209); }";     // @0x869190 (68)
static const char *kSpinNormal  = "QSpinBox { background-color: rgb(85,85,85); color: rgb(221,221,221); }";   // @0x8691d8 (70)

KOsdSpin02::KOsdSpin02(const KOsdSpinConfig &cfg, QWidget *parent)
    : KOsdSpinBase(parent)
    , m_cfg(cfg)
    , m_cur(cfg.def)   // реф.: +0x5c инициализируется значением cfg+0x14 (def)
{
    // Реф. ctor @0x484e18 — setupUi заинлайнен, порядок сохранён.
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("KOsdSpin02"));
    resize(250, 32);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    setMinimumSize(250, 32);
    setMaximumSize(250, 32);

    m_grid = new QGridLayout(this);
    m_grid->setObjectName(QStringLiteral("gridLayout"));
    m_grid->setHorizontalSpacing(6);
    m_grid->setVerticalSpacing(0);
    m_grid->setContentsMargins(9, 0, 9, 0);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("label_title"));
    m_grid->addWidget(m_title, 0, 0, 1, 1);

    m_spin = new QSpinBox(this);
    m_spin->setObjectName(QStringLiteral("spin_value"));
    m_spin->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_spin->setMinimumSize(180, 0);
    m_spin->setFocusPolicy(Qt::NoFocus);
    m_spin->setAutoFillBackground(false);
    m_spin->setStyleSheet(QString());          // реф.: пустая строка (len 0)
    m_spin->setWrapping(false);
    m_spin->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);   // реф. 0x84
    m_spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);    // реф. 0 — стрелки ВИДНЫ
    m_grid->addWidget(m_spin, 0, 1, 1, 1);

    m_title->setText(QString());   // реф. retranslateUi: пустой текст до InitWidget

    InitWidget();
    connect(m_spin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KOsdSpin02::ValueChangedAct);
}

void KOsdSpin02::InitWidget()
{
    // Реф. @0x484da0: пустой заголовок просто НЕ ставится (метка остаётся пустой).
    if (!m_cfg.title.isEmpty())
        m_title->setText(m_cfg.title);
    m_spin->setMinimum(m_cfg.min);
    m_spin->setMaximum(m_cfg.max);
    m_spin->setSingleStep(m_cfg.step);
    m_spin->setValue(m_cfg.def);
}

void KOsdSpin02::Focused()
{
    KFrame::Focused();                                            // реф. сначала база
    m_spin->setStyleSheet(QString::fromLatin1(kSpinFocused));
}

void KOsdSpin02::Selected()
{
    KFrame::Selected();
    m_spin->setStyleSheet(QString::fromLatin1(kSpinNormal));
}

void KOsdSpin02::UnSelected()
{
    KFrame::UnSelected();
    // Реф. ставит ТУ ЖЕ строку, что и Selected (проверено: один адрес @0x8691d8).
    m_spin->setStyleSheet(QString::fromLatin1(kSpinNormal));
}

void KOsdSpin02::ConfirmKeyActImpl(int index)
{
    // Реф. @0x485448: тело пустое (`ret`) — Confirm только переключает захват фокуса.
    Q_UNUSED(index);
}

void KOsdSpin02::AddValue()
{
    // Реф. @0x4854a8: клип СВЕРХУ по max, затем setValue и подсветка Focused.
    m_cur += m_cfg.step;
    if (m_cur > m_cfg.max)
        m_cur = m_cfg.max;
    m_spin->setValue(m_cur);
    Focused();
}

void KOsdSpin02::SubValue()
{
    // Реф. @0x485450: клип СНИЗУ по min.
    m_cur -= m_cfg.step;
    if (m_cur < m_cfg.min)
        m_cur = m_cfg.min;
    m_spin->setValue(m_cur);
    Focused();
}

void KOsdSpin02::SetIntValue(int v)
{
    // Реф. @0x485500: тихий сеттер ЧЕРЕЗ ФЛАГ (без disconnect, в отличие от KOsdSpin),
    // причём m_cur ОБНОВЛЯЕТСЯ (в KOsdSpin — нет).
    m_notify = false;
    m_cur = v;
    m_spin->setValue(v);
    m_notify = true;
}

void KOsdSpin02::ValueChangedAct(int value)
{
    // Реф. @0x485538: гейт m_notify, затем SendToMainCtrl(cfg+0x1c, cfg+0x18, value).
    // У нас поля конфига названы ctxId(+0x1c)/msgId(+0x18) — device-seam заменён сигналом.
    if (!m_notify)
        return;
    emit valueChanged(value);
}
