#include "KReportEditUi.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFrame>
#include <QGridLayout>

#include "KGridWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// Реф. KQuickInputComboBox → editable QComboBox (быстрый ввод/история).
QComboBox *quickCombo(QWidget *p, const char *name)
{
    QComboBox *c = new QComboBox(p);
    c->setObjectName(QString::fromLatin1(name));
    c->setEditable(true);
    return c;
}
// Ячейка «подпись сверху / редактор снизу» для сетки базовой инфо.
QWidget *labeled(QWidget *p, const QString &cap, const char *capName, QWidget *editor)
{
    QWidget *w = new QWidget(p);
    QVBoxLayout *v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    QLabel *l = new QLabel(cap, w);
    l->setObjectName(QString::fromLatin1(capName));
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    v->addWidget(l);
    v->addWidget(editor);
    return w;
}
// Реф. KCounterTextEdit → QTextEdit + счётчик «n/max» + обрезка по maxLen (чистый UI).
QWidget *counterEditor(QWidget *p, const QString &title, const char *editName,
                       const char *counterName, int maxLen)
{
    QWidget *w = new QWidget(p);
    QVBoxLayout *v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *head = new QHBoxLayout();
    head->addWidget(new QLabel(title, w));
    head->addStretch(1);
    QLabel *cnt = new QLabel(QStringLiteral("0/%1").arg(maxLen), w);
    cnt->setObjectName(QString::fromLatin1(counterName));
    head->addWidget(cnt);
    v->addLayout(head);
    QTextEdit *e = new QTextEdit(w);
    e->setObjectName(QString::fromLatin1(editName));
    QObject::connect(e, &QTextEdit::textChanged, e, [e, cnt, maxLen]() {
        QString txt = e->toPlainText();
        if (txt.length() > maxLen) {   // реф. ограничение длины
            const QSignalBlocker b(e);
            txt.truncate(maxLen);
            e->setPlainText(txt);
            QTextCursor c = e->textCursor();
            c.movePosition(QTextCursor::End);
            e->setTextCursor(c);
        }
        cnt->setText(QStringLiteral("%1/%2").arg(e->toPlainText().length()).arg(maxLen));
    });
    v->addWidget(e, 1);
    return w;
}
} // namespace

KReportEditUi::KReportEditUi(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x4d4108: KFullScreenDialog(parent,2007) → setupUi (SetKStyle/SetTitle нет).
    // Портируем над KDialog(FULLSCREEN); титул-бар не задаём (реф. страница без титула).
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);
}

void KReportEditUi::setupUi()
{
    setObjectName(QStringLiteral("KReportEditUi"));

    QWidget *host = ContentArea();
    // Реф. фрагмент стиля (тёмные подложки контейнеров по objectName).
    host->setStyleSheet(QStringLiteral(
        "QWidget#layoutWidget_left_options{background:rgb(29,31,35);}"
        "QWidget#gridWidget_base_info,QWidget#widget_main_editor,QWidget#word_bank,"
        "QWidget#container_exam_img{background:rgb(20,21,25);border-radius:8px;}"
        "QFrame#line_splitor{color:black;}"));

    QHBoxLayout *root = new QHBoxLayout(host);
    root->setObjectName(QStringLiteral("horizontalLayout_9"));
    root->setContentsMargins(0, 46, 0, 0);

    // ===================== A. Левая колонка кнопок =====================
    QWidget *leftCol = new QWidget(host);
    leftCol->setObjectName(QStringLiteral("layoutWidget_left_options"));
    QVBoxLayout *vL = new QVBoxLayout(leftCol);
    vL->setObjectName(QStringLiteral("verticalLayout_17"));
    vL->setContentsMargins(12, 20, 12, 20);
    auto addBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, leftCol);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedWidth(211);   // реф. min=max=211
        vL->addWidget(b);
        return b;
    };
    addBtn("btn_select_img", tr("TR_SImage"));
    addBtn("btn_add_mark", tr("TR_AMark"));
    addBtn("btn_write_conclusion", tr("TR_WConclusion"));
    vL->addStretch(1);
    QFrame *line = new QFrame(leftCol);
    line->setObjectName(QStringLiteral("line_splitor"));
    line->setFrameShape(QFrame::HLine); line->setFrameShadow(QFrame::Sunken);
    vL->addWidget(line);
    addBtn("btn_upload_dicom", tr("TR_UDICOM"));
    addBtn("btn_print_preview", tr("TR_PPreview"));
    addBtn("btn_print", tr("TR_Prnt"));
    addBtn("btn_template_settings", tr("TR_SPrinter"));
    addBtn("btn_word_bank", tr("TR_Glry"));
    addBtn("btn_save_as_word_bank", tr("TR_SAGlossary"));
    addBtn("btn_settings", tr("TR_Sttgs"));
    vL->addStretch(1);
    addBtn("btn_save", tr("TR_Sve"));                          // реф. = «OK» (ClickBtnSave device)
    QPushButton *btnExit = addBtn("btn_exit", tr("TR_Ext"));   // реф. = «Cancel»
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. ClickBtnExit→close
    root->addWidget(leftCol);

    // ===================== B. Правая основная область =====================
    QWidget *rightMain = new QWidget(host);
    QVBoxLayout *vR = new QVBoxLayout(rightMain);
    vR->setObjectName(QStringLiteral("verticalLayout_main"));
    vR->setContentsMargins(18, 16, 18, 16);

    // B1. Сетка базовой инфо.
    // Реф. Ui_KReportEditUi::setupUi @0x4d83ec: это KGridWidget (переукладка сетки
    // базовой информации), а НЕ голый QWidget — objectName "gridWidget_base_info".
    KGridWidget *baseInfo = new KGridWidget(rightMain);
    baseInfo->setObjectName(QStringLiteral("gridWidget_base_info"));
    baseInfo->setFocusPolicy(Qt::NoFocus);   // реф. @0x4d8424
    QGridLayout *gB = new QGridLayout(baseInfo);
    auto le = [&](const char *name) {
        QLineEdit *e = new QLineEdit(baseInfo);
        e->setObjectName(QString::fromLatin1(name));
        return e;
    };
    gB->addWidget(labeled(baseInfo, tr("TR_ENo"), "label_exam_id", le("lineEdit_exam_id")), 0, 0);
    gB->addWidget(labeled(baseInfo, tr("TR_EmDate"), "label_exam_date", le("lineEdit_exam_date")), 0, 1);
    gB->addWidget(labeled(baseInfo, tr("TR_EModel"), "label_endoscope", le("lineEdit_endoscope")), 0, 2);
    gB->addWidget(labeled(baseInfo, tr("TR_Stts"), "label_status", le("lineEdit_status")), 0, 3);
    gB->addWidget(labeled(baseInfo, tr("TR_Nme"), "label_name", quickCombo(baseInfo, "comboBox_patient_name")), 1, 0);
    // Пол + возраст — составная ячейка.
    QWidget *wGA = new QWidget(baseInfo);
    QHBoxLayout *hGA = new QHBoxLayout(wGA); hGA->setContentsMargins(0, 0, 0, 0);
    QComboBox *cmbGender = new QComboBox(wGA); cmbGender->setObjectName(QStringLiteral("comboBox_gender"));
    hGA->addWidget(labeled(wGA, tr("TR_Gdr"), "label_gender", cmbGender));
    hGA->addWidget(labeled(wGA, tr("TR_Age"), "label_age", le("lineEdit_age")));
    gB->addWidget(wGA, 1, 1);
    gB->addWidget(labeled(baseInfo, tr("TR_PID"), "label_patient_id", quickCombo(baseInfo, "comboBox_patient_id")), 1, 2);
    gB->addWidget(labeled(baseInfo, tr("TR_Aplct"), "label_applicant", quickCombo(baseInfo, "comboBox_applicant")), 1, 3);
    QDateEdit *dob = new QDateEdit(baseInfo); dob->setObjectName(QStringLiteral("dateEdit_birthday"));
    dob->setCalendarPopup(true); dob->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); dob->setDate(QDate(2000, 1, 1));
    gB->addWidget(labeled(baseInfo, tr("TR_DoB"), "label_birthday_title", dob), 2, 0);
    gB->addWidget(labeled(baseInfo, tr("TR_Tel"), "label_telephone_num", le("lineEdit_telephone_num")), 2, 1);
    gB->addWidget(labeled(baseInfo, tr("TR_BNo"), "label_bed_num", le("lineEdit_bed_num")), 2, 2);
    gB->addWidget(labeled(baseInfo, tr("TR_CField1"), "label_base_info_custom_field1", le("lineEdit_base_info_custom_field1")), 2, 3);
    gB->addWidget(labeled(baseInfo, tr("TR_CField2"), "label_base_info_custom_field2", le("lineEdit_base_info_custom_field2")), 3, 0);
    vR->addWidget(baseInfo, 1);

    // B2. Контейнер изображений (реф. exam_img — runtime; плейсхолдер).
    QWidget *imgInfo = new QWidget(rightMain);
    imgInfo->setObjectName(QStringLiteral("hWidget_exam_img_info"));
    QHBoxLayout *hImg = new QHBoxLayout(imgInfo);
    hImg->addWidget(new QLabel(tr("TR_Nme"), imgInfo));
    hImg->addWidget(new QLabel(tr("TR_EmDate"), imgInfo));
    hImg->addWidget(new QLabel(QString(), imgInfo));   // selected_count_label ""
    hImg->addStretch(1);
    vR->addWidget(imgInfo);

    // B3. Область текстовых полей + панель глоссария (реф. editor:word_bank = 4:1).
    QHBoxLayout *hContent = new QHBoxLayout();
    QWidget *editor = new QWidget(rightMain);
    editor->setObjectName(QStringLiteral("widget_main_editor"));
    QVBoxLayout *vEd = new QVBoxLayout(editor);
    vEd->setObjectName(QStringLiteral("main_editor"));
    vEd->addWidget(counterEditor(editor, tr("TR_VOExam"), "textEdit_exam_finding", "label_exam_finding_counter", 3000));
    vEd->addWidget(counterEditor(editor, tr("TR_EConclusion"), "textEdit_diagnose", "label_diagnose_counter", 3000));
    vEd->addWidget(counterEditor(editor, tr("TR_DName"), "textEdit_disease_name", "label_disease_name_counter", 800));
    vEd->addWidget(counterEditor(editor, tr("TR_OMode3"), "textEdit_surgical_method", "label_surgical_method_counter", 800));
    vEd->addWidget(counterEditor(editor, tr("TR_IFindings"), "textEdit_surgery_finding", "label_surgery_finding_counter", 800));
    vEd->addWidget(counterEditor(editor, tr("TR_Sgstn"), "textEdit_suggestion", "label_suggest_counter", 800));
    vEd->addWidget(counterEditor(editor, tr("TR_CustomField1"), "textEdit_conclusion_custom_field1", "label_conclusion_custom_field1_counter", 800));
    vEd->addWidget(counterEditor(editor, tr("TR_CustomField2"), "textEdit_conclusion_custom_field2", "label_conclusion_custom_field2_counter", 800));

    // Нижний ряд врачей.
    QWidget *bottom = new QWidget(editor);
    bottom->setObjectName(QStringLiteral("widget_main_editor_bottom"));
    QHBoxLayout *hBot = new QHBoxLayout(bottom);
    QLineEdit *biopsy = new QLineEdit(bottom); biopsy->setObjectName(QStringLiteral("lineEdit_biopsy"));
    hBot->addWidget(labeled(bottom, tr("TR_BSite"), "label_biopsy", biopsy));
    QComboBox *cmbHP = new QComboBox(bottom); cmbHP->setObjectName(QStringLiteral("comboBox_HP"));
    cmbHP->setEditable(true); cmbHP->setEnabled(false);   // реф. setEnabled(false)
    hBot->addWidget(labeled(bottom, tr("TR_HP"), "label_HP", cmbHP));
    hBot->addWidget(labeled(bottom, tr("TR_ADoctor"), "label_assist_doctor", quickCombo(bottom, "comboBox_assist_doctor")));
    hBot->addWidget(labeled(bottom, tr("TR_Dctr"), "label_doctor", quickCombo(bottom, "comboBox_doctor")));
    vEd->addWidget(bottom);

    hContent->addWidget(editor, 4);

    // Панель глоссария (реф. word_bank — дерево строится в InitQuickInput/runtime; плейсхолдер).
    QWidget *wordBank = new QWidget(rightMain);
    wordBank->setObjectName(QStringLiteral("word_bank"));
    wordBank->setAutoFillBackground(true);
    new QVBoxLayout(wordBank);
    hContent->addWidget(wordBank, 1);

    vR->addLayout(hContent, 4);
    root->addWidget(rightMain, 1);   // реф. left:right = 0:1
}
