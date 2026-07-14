#include "dicom/KDicomInterface.h"
#include "dicom/KEntityDicom.h"

#include <QDebug>

// Реализация сети DICOM требует DCMTK (dcmnet/dcmdata). На устройстве собирается
// с HAVE_DCMTK; на десктопе (нет DCMTK) — заглушки, повторяющие сигнатуры реверса,
// чтобы фасад/БД-слой были собираемы и тестируемы отдельно.

KDicomInterface &KDicomInterface::GetInstance()
{
    static KDicomInterface inst;   // реф. синглтон
    return inst;
}

void KDicomInterface::InitDicomSetting(const DicomSetting &s)
{
    setting_ = s;   // реф.: инициализация SCU/SCP параметрами узла
}

void KDicomInterface::ResetDicomSetting()
{
    setting_ = DicomSetting();
}

#if defined(HAVE_DCMTK)
// --- Устройство: реальные DCMTK-операции ---
// (Ассоциации через T_ASC_*; C-ECHO/C-STORE/C-FIND worklist; MPPS N-CREATE/N-SET;
//  Storage Commitment N-ACTION. Датасеты — по Worklist/Mpps*Format.xml; сохранение
//  результатов worklist в tb_DcmWorklist по FieldMap.) Реализуется в sysroot.
DicomResult KDicomInterface::DicomEcho(const DicomSetting &s) { /* KEchoScp/scu */ return DicomResult::Ok; }
DicomResult KDicomInterface::DicomStore(const DicomSetting &s, const QString &f) { /* KStoreScu::SendFile */ return DicomResult::Ok; }
DicomResult KDicomInterface::DicomStoreOneExam(const DicomSetting &s, const QString &examId) { return DicomResult::Ok; }
DicomResult KDicomInterface::DownloadWorklist(const DicomSetting &s, const QString &dir) { /* KWorklistScu::FindData */ return DicomResult::Ok; }
DicomResult KDicomInterface::DicomMPPS(const DicomSetting &s, const QString &examId) { return DicomResult::Ok; }
DicomResult KDicomInterface::DicomCommit(const DicomSetting &s, const QString &examId) { return DicomResult::Ok; }
#else
// --- Десктоп: DCMTK недоступен ---
namespace {
DicomResult noDcmtk(const char *op) {
    qWarning() << "KDicomInterface::" << op << ": DCMTK недоступен (сборка без HAVE_DCMTK)";
    return DicomResult::NetworkError;
}
}
DicomResult KDicomInterface::DicomEcho(const DicomSetting &) { return noDcmtk("DicomEcho"); }
DicomResult KDicomInterface::DicomStore(const DicomSetting &, const QString &) { return noDcmtk("DicomStore"); }
DicomResult KDicomInterface::DicomStoreOneExam(const DicomSetting &, const QString &) { return noDcmtk("DicomStoreOneExam"); }
DicomResult KDicomInterface::DownloadWorklist(const DicomSetting &, const QString &) { return noDcmtk("DownloadWorklist"); }
DicomResult KDicomInterface::DicomMPPS(const DicomSetting &, const QString &) { return noDcmtk("DicomMPPS"); }
DicomResult KDicomInterface::DicomCommit(const DicomSetting &, const QString &) { return noDcmtk("DicomCommit"); }
#endif
