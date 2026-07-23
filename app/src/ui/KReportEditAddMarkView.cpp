#include "KReportEditAddMarkView.h"

#include <QBrush>
#include <QButtonGroup>
#include <QColor>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include "KPosNameLineEditDelegate.h"
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace {
QGraphicsView *mkCanvas(QWidget *p, const char *name)
{
    QGraphicsView *v = new QGraphicsView(p);   // реф. KImageEditorGraphicsView → QGraphicsView
    v->setObjectName(QString::fromLatin1(name));
    v->setBackgroundBrush(QBrush(QColor(1, 1, 1), Qt::SolidPattern));
    return v;
}
} // namespace

KReportEditAddMarkView::KReportEditAddMarkView(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x4be7b8: QWidget(parent,0) → setupUi → resize(1614,760) → InitAddMarkView
    // (кисти фона, 2 эксклюзивные QButtonGroup, иконки, сцены, InitPosName, InitConnect).
    setupUi();
    resize(1614, 760);   // реф. resize
}

void KReportEditAddMarkView::setupUi()
{
    setObjectName(QStringLiteral("KReportEditAddMarkView"));

    QGridLayout *g = new QGridLayout(this);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setContentsMargins(6, 6, 6, 6);

    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));

    auto mkRadioRow = [&](QWidget *parent, QButtonGroup *grp,
                          std::initializer_list<std::pair<const char *, QString>> items) {
        QHBoxLayout *row = new QHBoxLayout();
        for (auto &it : items) {
            QRadioButton *r = new QRadioButton(it.second, parent);
            r->setObjectName(QString::fromLatin1(it.first));
            grp->addButton(r);
            row->addWidget(r);
        }
        row->addStretch(1);
        return row;
    };
    const QString aLD = QString(QChar(0x2199)), aLU = QString(QChar(0x2196)),
                  aRU = QString(QChar(0x2197)), aRD = QString(QChar(0x2198)),
                  pt = QString(QChar(0x2022));

    // ===================== Колонка A: метка изображения =====================
    QVBoxLayout *vA = new QVBoxLayout();
    vA->setObjectName(QStringLiteral("vLayout_img_add_mark"));
    vA->setContentsMargins(6, 6, 6, 6);
    vA->addWidget(new QLabel(tr("TR_IProcessing"), this));
    QButtonGroup *grpImg = new QButtonGroup(this);   // реф. эксклюзивная группа imgMark
    grpImg->setExclusive(true);
    QRadioButton *imgLock = nullptr;
    {
        QHBoxLayout *row = new QHBoxLayout();
        auto add = [&](const char *n, const QString &t) {
            QRadioButton *r = new QRadioButton(t, this);
            r->setObjectName(QString::fromLatin1(n));
            grpImg->addButton(r); row->addWidget(r); return r;
        };
        imgLock = add("radioButton_imgMark_lock", QStringLiteral("Lock"));
        add("radioButton_imgMark_ld", aLD); add("radioButton_imgMark_lu", aLU);
        add("radioButton_imgMark_ru", aRU); add("radioButton_imgMark_rd", aRD);
        row->addStretch(1);
        vA->addLayout(row);
    }
    imgLock->setChecked(true);
    QWidget *gvLeft = new QWidget(this);
    gvLeft->setObjectName(QStringLiteral("widget_graphics_view_left"));
    QVBoxLayout *vGv = new QVBoxLayout(gvLeft);
    vGv->addWidget(mkCanvas(gvLeft, "graphicsView_imgMark"), 1);
    QLabel *lblFile = new QLabel(gvLeft); lblFile->setObjectName(QStringLiteral("label_file_name"));
    lblFile->setAlignment(Qt::AlignCenter);
    vGv->addWidget(lblFile);
    vA->addWidget(gvLeft, 1);
    QHBoxLayout *hA = new QHBoxLayout();
    hA->addStretch(1);
    auto mkAbtn = [&](const char *n, const QString &t) {
        QPushButton *b = new QPushButton(t, this); b->setObjectName(QString::fromLatin1(n));
        hA->addWidget(b); return b;
    };
    mkAbtn("pushButton_imgMark_preImg", tr("TR_LPage"));
    mkAbtn("pushButton_imgMark_nextImg", tr("TR_NPage"));
    mkAbtn("pushButton_imgMark_clear", tr("TR_Clear"));
    hA->addStretch(1);
    vA->addLayout(hA);
    h->addLayout(vA, 0);

    // ===================== Колонка B: имя позиции =====================
    QVBoxLayout *vB = new QVBoxLayout();
    vB->setObjectName(QStringLiteral("vLayout_position_name"));
    vB->setContentsMargins(6, 6, 6, 6);
    vB->addWidget(new QLabel(tr("TR_OName"), this));
    QLabel *lblSel = new QLabel(this); lblSel->setObjectName(QStringLiteral("label_selected_posName"));
    vB->addWidget(lblSel);
    QListWidget *listOrgan = new QListWidget(this);   // реф. device-список, editable items
    listOrgan->setObjectName(QStringLiteral("listWidget_organ"));
    // Реф. InitPosName @0x4bfa38: делегат ставится на ВЕСЬ вид (не на колонку) — редактор
    // имени позиции это QLineEdit с maxLength 50 и своим стилем.
    listOrgan->setItemDelegate(new KPosNameLineEditDelegate(listOrgan));
    vB->addWidget(listOrgan, 1);
    QHBoxLayout *hB = new QHBoxLayout();
    QPushButton *btnAdd = new QPushButton(tr("TR_Add"), this);
    btnAdd->setObjectName(QStringLiteral("pushButton_posName_add"));
    QPushButton *btnDel = new QPushButton(tr("TR_Del"), this);
    btnDel->setObjectName(QStringLiteral("pushButton_posName_delete"));
    QPushButton *btnClr = new QPushButton(tr("TR_Clear"), this);
    btnClr->setObjectName(QStringLiteral("pushButton_posName_clear"));
    btnClr->setEnabled(false);   // реф. initially disabled
    hB->addWidget(btnAdd); hB->addWidget(btnDel); hB->addWidget(btnClr);
    vB->addLayout(hB);
    // Add/Del позиции — операции со списком (реф. UI/store); реализуем как UI.
    connect(btnAdd, &QPushButton::clicked, listOrgan, [listOrgan]() {
        QListWidgetItem *it = new QListWidgetItem(QString(), listOrgan);
        it->setFlags(it->flags() | Qt::ItemIsEditable);
        listOrgan->editItem(it);
    });
    connect(btnDel, &QPushButton::clicked, listOrgan, [listOrgan]() {
        delete listOrgan->takeItem(listOrgan->currentRow());
    });
    h->addLayout(vB, 1);

    // ===================== Колонка C: боди-метка =====================
    QVBoxLayout *vC = new QVBoxLayout();
    vC->setObjectName(QStringLiteral("vLayout_position_add_mark"));
    vC->setContentsMargins(6, 6, 6, 6);
    vC->addWidget(new QLabel(tr("TR_Bdmk"), this));
    QButtonGroup *grpPos = new QButtonGroup(this);   // реф. эксклюзивная группа posMark
    grpPos->setExclusive(true);
    vC->addLayout(mkRadioRow(this, grpPos, {
        {"radioButton_posMark_ld", aLD}, {"radioButton_posMark_lu", aLU},
        {"radioButton_posMark_ru", aRU}, {"radioButton_posMark_rd", aRD},
        {"radioButton_posMark_point", pt}}));
    vC->addWidget(mkCanvas(this, "graphicsView_positionMark"), 1);
    QHBoxLayout *hC = new QHBoxLayout();
    hC->addStretch(1);
    QPushButton *posClr = new QPushButton(tr("TR_Clear"), this);
    posClr->setObjectName(QStringLiteral("pushButton_posMark_clear"));
    hC->addWidget(posClr);
    hC->addStretch(1);
    vC->addLayout(hC);
    h->addLayout(vC, 2);

    g->addLayout(h, 0, 1);   // реф. horizontalLayout в grid(0,1); col0 пуст
}
