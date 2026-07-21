#include "KMessageBox.h"

#include <QAbstractButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
// Реф. стиль кнопок месседж-бокса (drawDialog): тёмный градиент 100×25, radius 7.
const char *kBtnStyle =
    "background: qlineargradient(spread:pad, x1:0,y1:1, x2:0,y2:0,"
    "stop:0 rgba(44,44,44,255), stop:1 rgba(100,100,100,255));"
    "border-radius:7px; color: rgb(186,182,192); width:100px; height:25px;";
KMessageBox *g_pDlg = nullptr;   // реф. глобал pDlg (активный месседж-бокс)
} // namespace

KMessageBox::KMessageBox(QWidget *parent)
    : QDialog(parent)
{
    // Реф. @0x689298: хром + пустой QMessageBox.
    setupUi();
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_msgbox = new QMessageBox(this);
    m_msgbox->setStyleSheet(QStringLiteral("QDialog{background-color: transparent;}"));
    drawDialog();
    connect(btn_close, &QToolButton::clicked, this, &QWidget::close);
    connect(m_msgbox, &QMessageBox::finished, this, &KMessageBox::clickDlgBtn);
}

KMessageBox::KMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text,
                         QMessageBox::StandardButtons buttons,
                         QMessageBox::StandardButton defBtn, QWidget *parent)
    : QDialog(parent)
{
    // Реф. @0x6897e0: хром → newMessageBox.
    setupUi();
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    newMessageBox(icon, title, text, buttons, defBtn);
    connect(btn_close, &QToolButton::clicked, this, &QWidget::close);
}

void KMessageBox::setupUi()
{
    // Реф. Ui_KMessageBox::setupUi @0x68a3f8. Безрамочный хром: фон-рамка → титул-бар.
    setObjectName(QStringLiteral("KMessageBox"));
    resize(130, 107);   // стартовый; растёт под контент

    QVBoxLayout *vTop = new QVBoxLayout(this);
    vTop->setContentsMargins(0, 0, 0, 0);

    QFrame *frame_back = new QFrame(this);
    frame_back->setObjectName(QStringLiteral("umessage_frame_back"));
    frame_back->setFrameShape(QFrame::StyledPanel);
    frame_back->setFrameShadow(QFrame::Raised);
    frame_back->setStyleSheet(QStringLiteral(
        "QFrame#umessage_frame_back{background-color:rgb(26,26,26);"
        "border:1px solid rgba(83,83,83,255);}"));
    vTop->addWidget(frame_back);

    vLayout_box = new QVBoxLayout(frame_back);
    vLayout_box->setContentsMargins(0, 0, 0, 0);

    // --- титул-бар ---
    QFrame *upbar = new QFrame(frame_back);
    upbar->setObjectName(QStringLiteral("umessage_frame_upbar"));
    upbar->setFrameShape(QFrame::StyledPanel);
    upbar->setFrameShadow(QFrame::Raised);
    upbar->setStyleSheet(QStringLiteral(
        "QFrame#umessage_frame_upbar{background-color:rgba(15,18,24,255);border:0px;}"));
    QHBoxLayout *hcap = new QHBoxLayout(upbar);
    hcap->setContentsMargins(6, 0, 0, 0);
    label_dlgtitle = new QLabel(upbar);
    label_dlgtitle->setObjectName(QStringLiteral("label_dlgtitle"));
    label_dlgtitle->setStyleSheet(QStringLiteral("color: white;background-color: transparent;"));
    label_dlgtitle->setText(tr("TR_Wng"));   // "Warning" по умолчанию
    hcap->addWidget(label_dlgtitle);
    hcap->addStretch(1);
    btn_close = new QToolButton(upbar);
    btn_close->setObjectName(QStringLiteral("btn_close"));
    btn_close->setFocusPolicy(Qt::ClickFocus);
    btn_close->setAutoRaise(true);
    btn_close->setStyleSheet(QStringLiteral(
        "QToolButton{border-width:0px;background:transparent;color:rgb(200,200,200);"
        "font-size:16px;}"));
    btn_close->setText(QString(QChar(0x2715)));   // ✕ (реф. close.png — device)
    hcap->addWidget(btn_close);
    vLayout_box->addWidget(upbar);

    setWindowTitle(tr("TR_Dlg"));
}

void KMessageBox::newMessageBox(QMessageBox::Icon icon, const QString &title,
                                const QString &text, QMessageBox::StandardButtons buttons,
                                QMessageBox::StandardButton defBtn)
{
    // Реф. @0x689600: заголовок = title в титул-бар; создать QMessageBox; drawDialog.
    label_dlgtitle->setText(title);
    m_result = 0;
    m_defaultButton = defBtn;
    m_msgbox = new QMessageBox(icon, title, text, buttons, this);
    m_msgbox->setStyleSheet(QStringLiteral("QDialog{background-color: transparent;}"));
    drawDialog();
    connect(m_msgbox, &QMessageBox::finished, this, &KMessageBox::clickDlgBtn);
}

void KMessageBox::drawDialog()
{
    // Реф. @0x688cb0: встроить m_msgbox под титул-баром + локализовать/перестилить кнопки.
    vLayout_box->addWidget(m_msgbox);
    vLayout_box->setAlignment(m_msgbox, Qt::AlignHCenter | Qt::AlignBottom);

    const auto btns = m_msgbox->buttons();
    for (QAbstractButton *b : btns) {
        // Переопределение текста стандартных кнопок (реф. TR).
        QMessageBox::StandardButton sb = m_msgbox->standardButton(b);
        if (sb == QMessageBox::Yes)         b->setText(tr("TR_Yes"));
        else if (sb == QMessageBox::No)     b->setText(tr("TR_No"));
        else if (sb == QMessageBox::Ok)     b->setText(tr("TR_OK"));
        else if (sb == QMessageBox::Cancel) b->setText(tr("TR_Ccl"));
        b->setStyleSheet(QString::fromLatin1(kBtnStyle));
        b->setMinimumWidth(100);
    }
    if (m_defaultButton == 0) {
        if (!btns.isEmpty())
            btns.last()->setFocus();
    } else {
        m_msgbox->setDefaultButton(static_cast<QMessageBox::StandardButton>(m_defaultButton));
    }
}

void KMessageBox::clickDlgBtn(int code)
{
    // Реф. @0x688ca8: запомнить код нажатой кнопки и закрыть.
    m_result = code;
    close();
}

int KMessageBox::exec()
{
    // Реф. @0x689c58: модально; возврат m_result (код кнопки) если ≠0, иначе QDialog::exec.
    setWindowModality(Qt::ApplicationModal);
    setModal(true);
    show();
    activateWindow();
    const int r = QDialog::exec();
    return m_result != 0 ? m_result : r;
}

void KMessageBox::SetText(const QString &text)
{
    if (m_msgbox)
        m_msgbox->setText(text);
}

void KMessageBox::SetIconPixmap(const QPixmap &pm)
{
    if (m_msgbox)
        m_msgbox->setIconPixmap(pm);
}

// --- статические хелперы (реф. единый паттерн) ---
static int showHelper(QWidget *parent, const QString &title, const QString &text,
                      QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defBtn)
{
    KMessageBox *dlg = new KMessageBox(QMessageBox::Information, title, text, buttons, defBtn, parent);
    g_pDlg = dlg;
    // Реф. setIconPixmap(":icon/messagebox/warning.png") — device-ассет; у нас встроенная иконка.
    dlg->SetIconPixmap(QMessageBox::standardIcon(QMessageBox::Warning));
    const int r = dlg->exec();
    delete dlg;
    g_pDlg = nullptr;
    return r;
}

int KMessageBox::information(QWidget *p, const QString &t, const QString &x,
                             QMessageBox::StandardButtons b, QMessageBox::StandardButton d)
{ return showHelper(p, t, x, b, d); }
int KMessageBox::question(QWidget *p, const QString &t, const QString &x,
                          QMessageBox::StandardButtons b, QMessageBox::StandardButton d)
{ return showHelper(p, t, x, b, d); }
int KMessageBox::warning(QWidget *p, const QString &t, const QString &x,
                         QMessageBox::StandardButtons b, QMessageBox::StandardButton d)
{ return showHelper(p, t, x, b, d); }
int KMessageBox::confirm(QWidget *p, const QString &t, const QString &x,
                         QMessageBox::StandardButtons b, QMessageBox::StandardButton d)
{ return showHelper(p, t, x, b, d); }
