#include "KExamListCancelDlg.h"
#include "KMessageBox.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

KExamListCancelDlg::KExamListCancelDlg(const QString &examKey, QWidget *parent)
    : KDialog(parent, false)
    , m_examKey(examKey)
{
    // Реф. ctor @0x7ebea0: setupUi(абс. геометрия) → SetKStyle(7) → setWindowTitle(TR_CReason)
    // → InitTable → InitConnect.
    SetKStyle(KDLG_W1024);
    SetTitle(tr("TR_CReason"));
    buildUi();
    InitTable();
}

void KExamListCancelDlg::buildUi()
{
    setObjectName(QStringLiteral("KExamListCancelDlg"));
    QWidget *content = ContentArea();
    QVBoxLayout *outer = new QVBoxLayout(content);

    m_table = new QTableWidget(content);
    m_table->setObjectName(QStringLiteral("table_reson"));
    m_table->setColumnCount(1);
    m_table->setHorizontalHeaderLabels({tr("TR_CReason")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    outer->addWidget(m_table);

    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch();
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), content);
    btnOk->setObjectName(QStringLiteral("btn_ok"));
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), content);
    btnCancel->setObjectName(QStringLiteral("btn_cancel"));
    btns->addWidget(btnOk); btns->addWidget(btnCancel); btns->addStretch();
    outer->addLayout(btns);

    connect(btnOk, &QPushButton::clicked, this, &KExamListCancelDlg::SlotClickBtnOk);
    connect(btnCancel, &QPushButton::clicked, this, &KExamListCancelDlg::SlotClickBtnCancel);
    connect(m_table, &QTableWidget::doubleClicked, this, &KExamListCancelDlg::ClickTableReasons);
}

void KExamListCancelDlg::InitTable()
{
    // Реф. @0x7ebce8: GetReasonList (17 причин DICOM MPPS CID 9300, коды 110500+row).
    static const char *reasons[17] = {
        "TR_DCProcedure", "TR_EFailure", "TR_IPOrdered", "TR_PATMContrast", "TR_PDied",
        "TR_PRTCProcedure", "TR_PTFTOSurgery", "TR_PDNArrive", "TR_PPregnant", "TR_COPFCCharging",
        "TR_DOder", "TR_NUCancel", "TR_ISOrdered", "TR_DFUReason", "TR_IWESelected",
        "TR_PCPContinuing", "TR_EChange"};
    m_table->setRowCount(17);
    for (int i = 0; i < 17; ++i)
        m_table->setItem(i, 0, new QTableWidgetItem(tr(reasons[i])));
}

int KExamListCancelDlg::HaveChosenTableItem() const
{
    // Реф. @0x7eb2e8: нет выбора → -1, иначе currentRow.
    if (m_table->selectedItems().isEmpty())
        return -1;
    return m_table->currentRow();
}

void KExamListCancelDlg::doCancel()
{
    // Реф. общий путь: нет выбора → предупреждение; иначе CancelSeries(examKey, 110500+row) +
    // DB-update + PublishMsg(12006) [DEVICE-STUB] → close.
    const int row = HaveChosenTableItem();
    if (row < 0) {
        KMessageBox::warning(this, tr("TR_Wng"), tr("TR_PSTCReason"), QMessageBox::Ok);
        return;   // диалог остаётся открытым
    }
    // DEVICE-STUB: KDicomInterface::CancelSeries(m_examKey, row + 110500) + exam-DB status +
    // KObject::PublishMsg(0x2ee6). В порте — только закрыть.
    close();
}

void KExamListCancelDlg::SlotClickBtnOk() { doCancel(); }
void KExamListCancelDlg::ClickTableReasons(const QModelIndex &) { doCancel(); }   // двойной клик = OK

void KExamListCancelDlg::SlotClickBtnCancel()
{
    close();   // реф.: без побочных эффектов
}
