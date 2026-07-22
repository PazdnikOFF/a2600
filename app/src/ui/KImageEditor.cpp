#include "KImageEditor.h"

#include <QBrush>
#include <QColor>
#include "KImageEditorGraphicsView.h"
#include <QGraphicsView>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

KImageEditor::KImageEditor(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x4a1cc8: KFullScreenDialog(parent,-1) → setupUi → override текстов кнопок
    // (+хоткеи) → SetTitle(TR_IProcessing) → чёрный фон холста.
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);        // реф. полноэкранная страница 1920×1080
    SetTitle(tr("TR_IProcessing"));
}

void KImageEditor::setupUi()
{
    setObjectName(QStringLiteral("KImageEditor"));

    QWidget *host = ContentArea();
    QGridLayout *g = new QGridLayout(host);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setHorizontalSpacing(0);
    g->setContentsMargins(0, 46, 0, 6);   // реф. margins (0,46,0,6)

    // Холст АПГРЕЙД: реальный KImageEditorGraphicsView (был QGraphicsView-заглушка).
    KImageEditorGraphicsView *view = new KImageEditorGraphicsView(host);
    view->setObjectName(QStringLiteral("graphicsView"));
    view->setFocusPolicy(Qt::NoFocus);
    g->addWidget(view, 0, 0);

    // Имя файла под холстом (device).
    QLabel *lblFile = new QLabel(host);
    lblFile->setObjectName(QStringLiteral("label_file_name"));
    lblFile->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    g->addWidget(lblFile, 1, 0);

    // Правая панель управления.
    QWidget *right = new QWidget(host);
    right->setObjectName(QStringLiteral("widget_right"));
    QVBoxLayout *vR = new QVBoxLayout(right);
    vR->setObjectName(QStringLiteral("verticalLayout"));

    QLabel *lblType = new QLabel(tr("TR_CType"), right);
    lblType->setObjectName(QStringLiteral("label"));
    lblType->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    lblType->setMinimumWidth(100); lblType->setMaximumWidth(200);
    vR->addWidget(lblType);

    // 5 icon-only radio (иконки — ресурсы; у нас глиф-замена). Авто-эксклюзив.
    QVBoxLayout *vRadio = new QVBoxLayout();
    vRadio->setObjectName(QStringLiteral("verticalLayout_ratioButton"));
    vRadio->setContentsMargins(0, 6, 0, 6);
    auto mkRadio = [&](const char *name, const QString &glyph) {
        QRadioButton *r = new QRadioButton(glyph, right);
        r->setObjectName(QString::fromLatin1(name));
        r->setMinimumWidth(80); r->setMaximumWidth(160);
        vRadio->addWidget(r, 0, Qt::AlignHCenter);
        return r;
    };
    QRadioButton *rLock = mkRadio("radioButton_Lock", tr("TR_CType"));   // Lock (глиф-замена: подпись)
    rLock->setText(QStringLiteral("Lock"));
    QRadioButton *rRD = mkRadio("radioButton_RightDown", QString(QChar(0x2198)));   // ↘
    QRadioButton *rRU = mkRadio("radioButton_RightUp", QString(QChar(0x2197)));     // ↗
    QRadioButton *rLU = mkRadio("radioButton_LeftUp", QString(QChar(0x2196)));      // ↖
    QRadioButton *rLD = mkRadio("radioButton_LeftDown", QString(QChar(0x2199)));    // ↙
    rLock->setChecked(true);   // реф. InitRatioButton дефолт = Lock
    // Реф. слоты SetArrow*: радио → view->SetCursorType (Lock=0/RD=1/RU=2/LU=3/LD=4).
    auto wire = [&](QRadioButton *r, int type) {
        connect(r, &QRadioButton::toggled, view, [view, type](bool on) {
            if (on) view->SetCursorType(type);
        });
    };
    wire(rLock, KImageEditorGraphicsView::Lock);
    wire(rRD, KImageEditorGraphicsView::ArrowRightDown);
    wire(rRU, KImageEditorGraphicsView::ArrowRightUp);
    wire(rLU, KImageEditorGraphicsView::ArrowLeftUp);
    wire(rLD, KImageEditorGraphicsView::ArrowLeftDown);
    vRadio->addStretch(1);     // реф. вертикальный спейсер
    vR->addLayout(vRadio);

    // Кнопки навигации/действий (тексты + хоткеи из ctor).
    QVBoxLayout *vBtn = new QVBoxLayout();
    vBtn->setObjectName(QStringLiteral("verticalLayout_2"));
    vBtn->setContentsMargins(6, 0, 6, 0);
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, right);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumWidth(140); b->setMaximumWidth(280);
        b->setFocusPolicy(Qt::NoFocus);
        vBtn->addWidget(b);
        return b;
    };
    mkBtn("pushButton_pre", tr("TR_LPage") + QStringLiteral(" (F4)"));    // реф. OnBtnPre — UI-нав
    mkBtn("pushButton_next", tr("TR_NPage") + QStringLiteral(" (F5)"));   // реф. OnBtnNext — UI-нав
    mkBtn("pushButton_save", tr("TR_Sve") + QStringLiteral(" (F1)"));     // реф. OnBtnSave — device
    mkBtn("pushButton_delete", tr("TR_Del") + QStringLiteral(" (Del)"));  // реф. OnBtnDelete — device
    QPushButton *btnExit = mkBtn("pushButton_exit", tr("TR_Ext") + QStringLiteral(" (Esc)"));
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. CloseWidget→close
    vR->addLayout(vBtn);

    g->addWidget(right, 0, 1);
}
