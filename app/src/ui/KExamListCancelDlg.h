#pragma once

#include "KDialog.h"

class QTableWidget;
class QModelIndex;

// Диалог выбора причины отмены обследования (реф. KExamListCancelDlg @ctor 0x7ebea0, base
// KDialog, SetKStyle(7)=W1024, title TR_CReason). UI-порт. Вопреки имени — НЕ список
// обследований, а пикер ОДНОЙ причины отмены DICOM MPPS (CID 9300, коды 110500+row).
// std::string ctor-арг = ключ/GUID отменяемого обследования. Одна колонка table_reson с 17
// причинами (NoEditTriggers, stretchLastSection) + btn_ok(TR_OK)/btn_cancel(TR_Ccl).
// Двойной клик по строке = OK. Пустой выбор → предупреждение TR_PSTCReason.
// DEVICE-STUB: KDicomInterface::CancelSeries + KExamListDBTableHandler get/update + IPC
// PublishMsg(12006) — заглушки.
class KExamListCancelDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KExamListCancelDlg(const QString &examKey = QString(), QWidget *parent = nullptr);

private slots:
    void SlotClickBtnOk();      // реф. @0x7ec6a8
    void SlotClickBtnCancel();  // реф. @0x7eb058
    void ClickTableReasons(const QModelIndex &idx);   // реф. @0x7ed0e8 = OK

private:
    void buildUi();
    void InitTable();           // реф. @0x7ebce8: 17 причин
    int HaveChosenTableItem() const;   // реф. @0x7eb2e8: currentRow или -1
    void doCancel();            // общий путь OK/double-click (device stub)

    QTableWidget *m_table = nullptr;
    QString m_examKey;
};
