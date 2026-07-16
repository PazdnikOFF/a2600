// Qt-only превью экранов UI (без GStreamer) — для рендера/сверки с оригиналом.
//
//   QT_QPA_PLATFORM=offscreen ./ui_preview desktop out.png     # рендер в файл
//   ./ui_preview desktop                                       # показать окно
//
// ENDO_RES должен указывать на .../app/resources/ui (или запускать из app/).

#include "ui/Theme.h"
#include "ui/KUIDesktop.h"
#include "ui/KDisplayOption.h"
#include "ui/KImgList.h"
#include "endo/KSoftEndoParam.h"
#include "db/KEntityManage.h"
#include "db/KEntityQuickInput.h"
#include "db/KExamListConfigHandler.h"
#include "db/KEntityExam.h"
#include "db/KFileBackup.h"
#include "db/KSaveFile.h"
#include "db/KEntityService.h"
#include "alg/AlgParaManager.h"
#include "ctrl/KPlControl.h"
#include "ctrl/KDccuParam.h"
#include "video/KVideoParam.h"
#include "video/KVideoSet.h"
#include "video/KVideoProxy.h"
#include "video/KVideoCal.h"
#include "sys/KSystem.h"
#include "dicom/KDicomFieldMap.h"
#include "dicom/KEntityDicom.h"
#include "dicom/KDicomDatasetFormat.h"
#include "dicom/KSysDICOMData.h"
#include "kernel/KConfig.h"
#include "report/KMeaXMLBase.h"
#include "report/KMeaStringUtil.h"
#include "db/KExamNoGenerate.h"
#include "sys/KManuPwdMng.h"
#include "db/KDbFileOperation.h"
#include "kernel/KControlINI.h"
#include "dicom/KPatientStringOperation.h"
#include "dicom/KDbStringOperation.h"
#include "ui/KStopWatch.h"
#include "db/KEntityPatient.h"
#include "db/KEntityDoctor.h"
#include "db/KDbStrHandler.h"
#include "sys/KSessionInfo.h"
#include "sys/KEncSettings.h"
#include "report/KTextBlock.h"
#include "report/KImageBlock.h"
#include "report/KTableBlock.h"
#include "report/KTitleTableBlock.h"
#include "report/KReportTemplateCommonDef.h"
#include "db/KPatientExamData.h"
#include "db/KDbSqlite.h"
#include "report/XmlParser.h"
#include <QTemporaryDir>
#include "kernel/KObject.h"
#include "kernel/KMessage.h"
#include "kernel/KPublishManager.h"

// Тестовый подкласс KObject для self-test kobject: фиксирует доставку обработчиков.
namespace {
class KObjTestSink : public KObject
{
public:
    KObjTestSink(int id, KObject *parent) : KObject(id, parent) {}
    int subCount = 0, lastSub = 0;
    int msgCount = 0, lastMsg = 0;
    int reqCount = 0, lastReq = 0;
    void HandleSubscribeMsg(KMessage &m) override { ++subCount; lastSub = m.m_msgId; }
    void HandleMsg(KMessage &m) override { ++msgCount; lastMsg = m.m_msgId; m.m_bHandled = true; }
    void HandleChildRequest(KMessage &m) override { ++reqCount; lastReq = m.m_msgId; }
};
} // namespace
#include <QNetworkAccessManager>
#include "sys/KSystemSet.h"
#include "report/KTemplateCfg.h"
#include "report/KTemplateLibCfg.h"
#include "report/KTemplateParamCfg.h"
#include "report/KReportTemplateCommonDef.h"
#include "sys/KEnvConfig.h"
#include "db/KPatientListConfigSetupHandler.h"
#include "db/KWorklistConfigSetupHandler.h"
#include <QJsonArray>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include "report/KReportTemplate.h"
#include "report/KSysReportTempletCfg.h"
#include "report/KReportDataSource.h"
#include "report/KReportDisplayParam.h"
#include "report/KDocumentGenerator.h"
#include "report/KEntityReport.h"
#include "report/KThesaurusOpt.h"
#include "report/KRTDataSourceReal.h"
#include "report/KRTDataSourceDemo.h"
#include "sys/KAccount.h"
#include "sys/KSystemSet.h"
#include "sys/KUserSet.h"
#include "sys/KUserOsdSet.h"
#include "ctrl/KColdLightConfig.h"
#include "sys/KUpdateConf.h"
#include "sys/KUpdateManifest.h"
#include "sys/KEndoInfoServerConfig.h"
#include "sys/KRemoteSwitchConfig.h"
#include "sys/KPatientTimeOperation.h"
#include "sys/KVersionConfig.h"
#include "sys/KProjectSet.h"
#include "sys/KStyleConfig.h"
#include "sys/KStatisticConfig.h"
#include "sys/languageConfig.h"
#include "sys/KLoadUnicodeText.h"
#include "sys/KEncStyle.h"
#include "db/KExamListRecordFileUpdate.h"
#include "sys/KSystemStatus.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMap>

#include <QApplication>
#include <QWidget>
#include <QPixmap>
#include <QProcessEnvironment>
#include <QDebug>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    const QString sheet = theme::loadStyleSheet();
    if (sheet.isEmpty())
        qWarning() << "style.qss не найден — проверь ENDO_ROOT:" << theme::root();
    app.setStyleSheet(sheet);

    const QString screen = argc > 1 ? QString::fromUtf8(argv[1]) : "desktop";
    const QString outFile = argc > 2 ? QString::fromUtf8(argv[2]) : QString();
    const QString scope   = argc > 3 ? QString::fromUtf8(argv[3]) : QString();

    // Демонстрация подключения эндоскопа: имя модели → video.ini → videoSize →
    // выбор IMG-раскладки (как EndoStatusChangeAct в оригинале).
    if (!scope.isEmpty()) {
        KSoftEndoParam ep;
        const QSize sz = ep.GetVideoSize(scope);
        qInfo() << "scope" << scope << "sensor" << ep.GetSensorType(scope)
                << "videoSize" << sz;
        if (sz.isValid())
            KDisplayOption::Instance().SetCurrentImageSize(sz);
    }

    // Self-test БД (реф. KEntityManage): создать БД, добавить пациента+осмотр, прочитать.
    if (screen == "db") {
        const QString dbPath = outFile.isEmpty() ? "/tmp/endo_test.db" : outFile;
        KEntityManage &em = KEntityManage::Instance();
        if (!em.OpenDb(dbPath)) { qWarning() << "OpenDb failed"; return 3; }
        em.AddPatientEntity({"P001", "Ivanov Ivan", "M", "45", "1980-05-01"});
        em.AddExamEntity({"A0001", "P001", "2026-07-14", "12:00:00", "/data/exam/A0001"});
        PatientEntity p;
        const bool got = em.GetPatientEntity("P001", p);
        qInfo() << "DB roundtrip: got=" << got << "name=" << p.patientName
                << "sex=" << p.patientSex << "age=" << p.patientAge;
        qInfo() << "exams for P001:" << em.GetExamList("P001").size();
        em.CloseDb();
        return got ? 0 : 4;
    }

    // Self-test обработки (реф. AlgParaManager): гамма-LUT из videoconf/Gamma.
    if (screen == "alg") {
        AlgParaManager &alg = AlgParaManager::GetInstance();
        const auto p = alg.LoadGammaPara("IMX274", "ARTHROSCOPE");
        const QVector<int> lut = AlgParaManager::CalGammaLut(p);
        bool mono = true;
        for (int i = 1; i < lut.size(); ++i) if (lut[i] < lut[i-1]) { mono = false; break; }
        qInfo() << "Gamma IMX274/ARTHROSCOPE: bp=" << p.bp << "gamma=" << p.gamma
                << "inputmax=" << p.inputmax;
        qInfo() << "LUT size=" << lut.size() << "first=" << lut.first()
                << "mid=" << lut[lut.size()/2] << "last=" << lut.last()
                << "monotonic=" << mono;
        // CCM 3×3 из реального конфига
        const auto ccm = alg.LoadCcmMatrix("IMX274", "1920X1080", "ARTHROSCOPE");
        if (ccm.valid) {
            const int r0 = ccm.m[0] + ccm.m[1] + ccm.m[2];
            const int r1 = ccm.m[3] + ccm.m[4] + ccm.m[5];
            const int r2 = ccm.m[6] + ccm.m[7] + ccm.m[8];
            qInfo() << "CCM row0:" << ccm.m[0] << ccm.m[1] << ccm.m[2] << "sum=" << r0;
            qInfo() << "CCM row1:" << ccm.m[3] << ccm.m[4] << ccm.m[5] << "sum=" << r1;
            qInfo() << "CCM row2:" << ccm.m[6] << ccm.m[7] << ccm.m[8] << "sum=" << r2;
            qInfo() << "CCM (Q9, 1.0=512) row-sums ~512:" << r0 << r1 << r2;
        } else {
            qInfo() << "CCM: не загружена";
        }
        const bool ok = mono && lut.first() == 0 && lut.last() == p.inputmax - 1;
        return ok ? 0 : 5;
    }

    // Self-test PL-регистров параметров изображения (реф. KPlControl setters,
    // источники значений — AlgParaManager из videoconf). Без /dev/mem: трассировка.
    if (screen == "plreg") {
        const QString sensor = scope.isEmpty() ? "IMX274" : scope;
        AlgParaManager &alg = AlgParaManager::GetInstance();
        alg.LoadColEnhLevels(sensor);
        alg.LoadImgEnhLevels("WL", "OV2740_EG_1280X960", 'a');
        alg.LoadBrightEqPara(sensor);

        KPlControl pl;
        pl.EnableTrace(true);

        // 1) ColorEnhance: level 0 → выкл. (enable=0), затем level 5 → вкл.+значение.
        pl.SetColorEnhParam(false, 0);
        pl.SetColorEnhParam(true, 5);
        // 2) ImageEnhance: level 4.
        pl.SetImageEnhValue(4);
        // 3) RBC-тон: R/B/C гейны (реф. SetToneValue).
        pl.SetToneValue(0x120, 0x40, 0x100);
        // 4) BrightEQ enable.
        pl.SetBrightEQEnable(true);
        // 5) AEC+AGC combined.
        pl.SetAECAndAGCValue(0x1234, 0x0056);
        // 6) AWB гейны + строб.
        pl.SetAWBValue(0x0180, 0x0140);
        // 7) AWB cut пороги.
        pl.SetAwbCut(0x10, 0x2f);

        const int colEnh5 = alg.ColEnhLevelValue(5);   // из colenh_level.txt
        const int imgEnh4 = alg.ImgEnhLevelValue(4);   // из level_a.txt
        qInfo() << "ColEnh level5 value =" << QString::number(colEnh5, 16)
                << " ImgEnh level4 value =" << QString::number(imgEnh4, 16);

        // Ожидаемые записи (адрес,значение) по картам регистров из дизассемблера.
        struct Exp { unsigned long a; unsigned int v; const char *what; };
        const QVector<Exp> exp = {
            {0xa18f0008, 0,       "ColorEnh disable"},
            {0xa18f0024, 0,       "ColorEnh param(level0)"},
            {0xa18f0008, 1,       "ColorEnh enable"},
            {0xa18f0024, (unsigned)colEnh5, "ColorEnh param(level5)"},
            {0xa1850058, (unsigned)imgEnh4, "ImageEnh param(level4)"},
            {0xa1870004, 0x120,   "Tone R"},
            {0xa1870008, 0x40,    "Tone B"},
            {0xa1870000, 0x100,   "Tone C"},
            {0xa1950000, 1,       "BrightEQ enable"},
            {0xa0048020, (0x1234u) | (0x0056u << 16), "AEC|AGC"},
            {0xa184000c, (0x0140u & 0x1ffff) | (0x0180u << 16), "AWB gains"},
            {0xa100019c, 1,       "AWB strobe hi"},
            {0xa100019c, 0,       "AWB strobe lo"},
            {0xa1840018, 0x10u | (0x2fu << 16), "AWB cut"},
        };
        const auto &tr = pl.Trace();
        bool ok = (tr.size() == exp.size());
        qInfo() << "PL writes:" << tr.size() << "expected:" << exp.size();
        for (int i = 0; i < exp.size() && i < tr.size(); ++i) {
            const bool m = (tr[i].first == exp[i].a && tr[i].second == exp[i].v);
            if (!m) ok = false;
            qInfo("  [%2d] %-24s W 0x%08lx=0x%08x  exp 0x%08lx=0x%08x  %s",
                  i, exp[i].what, tr[i].first, tr[i].second,
                  exp[i].a, exp[i].v, m ? "OK" : "MISMATCH");
        }
        // Осмысленность значений из конфигов: level5 > level0, оба валидны.
        if (colEnh5 == 0 || imgEnh4 == 0) {
            qWarning() << "конфиг уровней не загружен (проверь ENDO_ROOT/videoconf)";
            ok = false;
        }

        // Bright EQ LUT: 18 записей гаусса (0xa1950004..) + 512 lumaGainLut (0xa1958000..).
        pl.ClearTrace();
        pl.SetBrightEQLut(2);   // level 2 → индекс low (clamp(2,1,3)-1=1)
        const auto &tl = pl.Trace();
        const int nGauss = 18, nLuma = 512;
        bool eqOk = (tl.size() == nGauss + nLuma);
        if (eqOk) {
            eqOk = tl.first().first == 0xa1950004 &&
                   tl[nGauss - 1].first == 0xa1950004 + (nGauss - 1) * 4 &&
                   tl[nGauss].first == 0xa1958000 &&
                   tl.last().first == 0xa1958000 + (nLuma - 1) * 4;
        }
        // Проверка упаковки первой записи гаусса: gaussian[0]|(gaussian[1]<<16).
        const QVector<int> g = alg.BrightEqGaussian();
        const QVector<int> lo = alg.BrightEqLumaLut(1);
        if (g.size() >= 2) {
            const unsigned exp0 = (unsigned(g[0]) & 0x7fff) | ((unsigned(g[1]) & 0x7fff) << 16);
            if (tl.first().second != exp0) eqOk = false;
        }
        qInfo() << "BrightEQ LUT writes:" << tl.size() << "(18 gauss +512 luma), gauss size="
                << g.size() << "luma(low) size=" << lo.size()
                << " first=0x" + QString::number(tl.isEmpty()?0:tl.first().second, 16);
        if (!eqOk) { qWarning() << "BrightEQ LUT: карта записей не сошлась"; ok = false; }

        // Новые register-методы (батч): SetGammaEnable/SetZoomValue/SetCCM1/CHb.
        // CHb: config-driven (4-е значение CHb-файла); загружаем перед SetChbStatus.
        alg.LoadChbPara("OV2740", "EG_1504X1080");   // 4-е значение = 0x34803c0
        const int chb = alg.ChbValue();
        pl.ClearTrace();
        pl.SetGammaEnable(true);
        pl.SetZoomValue(0x123);
        pl.SetCCM1(1);
        const unsigned ccm1[9] = {0x0100,0,0, 0,0x0100,0, 0,0,0x0100};
        pl.SetCCM1Matrix(ccm1, 9);
        pl.SetChbStatus(1);   // status!=0 → вкл + запись ChbValue
        pl.SetChbStatus(0);   // status==0 → выкл
        const auto &tb = pl.Trace();
        // gamma(1)+zoom(1)+ccm1en(1)+ccm1matrix(4 пары+1 хвост=5)+chb-on(2)+chb-off(1) = 11
        bool batchOk = tb.size() == 1 + 1 + 1 + 5 + 2 + 1 && chb == 0x34803c0 &&
                       tb[0].first == 0xa1830000 && tb[1].first == 0xa18d0004 &&
                       tb[1].second == 0x123 && tb[2].first == 0xa1880000 &&
                       tb[3].first == 0xa1880004 && tb[3].second == 0x100 &&   // 1-я пара
                       tb[7].first == 0xa1880014 && tb[7].second == 0x100 &&   // хвост (m[8])
                       tb[8].first == 0xa1900008 && tb[8].second == 1 &&
                       tb[9].first == 0xa1900018 && tb[9].second == (unsigned)chb &&
                       tb[10].first == 0xa1900008 && tb[10].second == 0;
        qInfo() << "batch writes:" << tb.size() << "(exp 11) chb=0x" + QString::number(chb,16)
                << (batchOk ? "OK" : "MISMATCH");
        if (!batchOk) ok = false;

        // Freeze/scaler/geometry (простые регистры).
        pl.ClearTrace();
        pl.SetFreezeStatus(1);
        pl.SetVideoDisplay(2);
        pl.SetFreezeScalerIn(0x10, 0x20);
        pl.SetFreezeScalerOut(0x30, 0x40);
        pl.SetFreezeScalerRatio(0x50, 0x60);
        pl.SetCutPara(0x7, 0x8);
        const auto &tg = pl.Trace();
        bool geoOk = tg.size() == 6 &&
                     tg[0].first == 0xa180002c && tg[0].second == 1 &&
                     tg[1].first == 0xa0080028 && tg[1].second == 2 &&
                     tg[2].first == 0xa191000c && tg[2].second == (0x10u | (0x20u<<16)) &&
                     tg[3].first == 0xa1910010 && tg[4].first == 0xa1910008 &&
                     tg[5].first == 0xa1860018 && tg[5].second == (0x8u | (0x7u<<16));
        qInfo() << "geo/freeze writes:" << tg.size() << "(exp 6)" << (geoOk ? "OK" : "MISMATCH");
        if (!geoOk) ok = false;

        // Новые декодированные методы (логика 1:1 из дизасма X2000):
        //   SetFreezeVideoLoc → 0xa1800024=a|(b<<16), 0xa1800028=c|(d<<16);
        //   SetLensSize → 0xa189000c=a|(b<<16); SetEnhanceSize/SetContrastLevel — пустые.
        pl.ClearTrace();
        pl.SetFreezeVideoLoc(0x11, 0x22, 0x33, 0x44);
        pl.SetLensSize(0x5, 0x6);
        pl.SetEnhanceSize(0x7, 0x8);   // no-op (пустая в прошивке)
        pl.SetContrastLevel(3);        // no-op (пустая в прошивке)
        pl.SetDemoireEN(0x9);          // → 0xa18501cc = 9 (passthrough)
        pl.SetApmAreaDisplay(true);    // → 0xa18a0008 = 1
        const auto &tv = pl.Trace();
        const bool vlocOk = tv.size() == 5 &&   // 2 + 1 + 0 + 0 + 1 (demoire) + 1 (apm)
                            tv[0].first == 0xa1800024 && tv[0].second == (0x11u | (0x22u<<16)) &&
                            tv[1].first == 0xa1800028 && tv[1].second == (0x33u | (0x44u<<16)) &&
                            tv[2].first == 0xa189000c && tv[2].second == (0x5u | (0x6u<<16)) &&
                            tv[3].first == 0xa18501cc && tv[3].second == 0x9 &&
                            tv[4].first == 0xa18a0008 && tv[4].second == 1;
        qInfo() << "freezeloc/lens/noop/demoire/apm writes:" << tv.size() << "(exp 5)"
                << (vlocOk ? "OK" : "MISMATCH");
        if (!vlocOk) ok = false;

        // SetCameraIrisType — точная кодировка из дизасма (0xa18a0000):
        //   type0→0x530, type1→0x431, type2(sub5)→0x132, type2(др.)→0x232, type3→0x433.
        pl.ClearTrace();
        pl.SetCameraIrisType(0, 0);
        pl.SetCameraIrisType(1, 0);
        pl.SetCameraIrisType(2, 5);
        pl.SetCameraIrisType(2, 0);
        pl.SetCameraIrisType(3, 0);
        const auto &tci = pl.Trace();
        const bool camIrisOk = tci.size() == 5 &&
                            tci[0].first == 0xa18a0000 && tci[0].second == 0x530 &&
                            tci[1].second == 0x431 && tci[2].second == 0x132 &&
                            tci[3].second == 0x232 && tci[4].second == 0x433;
        qInfo() << "cameraIris writes:" << tci.size() << "vals:"
                << QString::number(tci.size()>0?tci[0].second:0,16)
                << QString::number(tci.size()>2?tci[2].second:0,16)
                << (camIrisOk ? "OK" : "MISMATCH");
        if (!camIrisOk) ok = false;

        // Sensor LUT (config-driven, OV2740_EC_1504X1080: по 1024 → 512 пар/канал).
        const auto sl = alg.LoadSensorLut("OV2740", "EC_1504X1080");
        pl.ClearTrace();
        if (sl.valid) {
            pl.SetSensorRLut(sl.r.constData(), sl.r.size());
            pl.SetSensorGLut(sl.g.constData(), sl.g.size());
            pl.SetSensorBLut(sl.b.constData(), sl.b.size());
        }
        const auto &ts = pl.Trace();
        bool lutOk = sl.valid && sl.r.size() == 1024 && ts.size() == 512*3 &&
                     ts.first().first == 0xa1820800 &&
                     ts[512].first == 0xa1821000 && ts[1024].first == 0xa1821800;
        qInfo() << "sensorLUT valid=" << sl.valid << "r=" << sl.r.size()
                << "writes=" << ts.size() << "(exp 1536)" << (lutOk ? "OK" : "MISMATCH");
        if (!lutOk) ok = false;

        // RBC LUT (config-driven, 3 канала → банки 0xa1878200/0100/0000).
        const auto rb = alg.LoadRbcLut("OV2740", "EC_1504X1080");
        pl.ClearTrace();
        if (rb.valid) {
            alg.SetCurRbcLut(rb);   // реф.: данные в массив AlgParaManager → void SetRbcLut
            pl.SetRbcLut();
        }
        const auto &tr2 = pl.Trace();
        const int rn = rb.valid ? qMin(rb.hb.size(), qMin(rb.hr.size(), rb.s.size())) : 0;
        bool rbcOk = rb.valid && rn > 0 && tr2.size() == rn*3 &&
                     tr2[0].first == 0xa1878200 && tr2[1].first == 0xa1878100 &&
                     tr2[2].first == 0xa1878000 &&
                     tr2[0].second == rb.hb[0] && tr2[2].second == rb.s[0];
        qInfo() << "RBC LUT valid=" << rb.valid << "n=" << rn
                << "writes=" << tr2.size() << (rbcOk ? "OK" : "MISMATCH");
        if (!rbcOk) ok = false;

        // Прочие простые регистры: RealtimeVideoState/ApmAreaDisplay/VideoTest.
        pl.ClearTrace();
        pl.SetRealtimeVideoState(1);
        pl.SetApmAreaDisplay(true);
        pl.VideoTest(3);
        const auto &tm = pl.Trace();
        bool miscOk = tm.size() == 3 &&
                      tm[0].first == 0xa0080024 && tm[0].second == 1 &&
                      tm[1].first == 0xa18a0008 && tm[1].second == 1 &&
                      tm[2].first == 0xa004a040 && tm[2].second == (3u << 2);
        pl.StartAWB();   // + StartAWB: 0xa1840000=1, 0xa18e0000=0
        const bool awbOk = tm.size() == 5 &&
                           tm[3].first == 0xa1840000 && tm[3].second == 1 &&
                           tm[4].first == 0xa18e0000 && tm[4].second == 0;
        qInfo() << "misc+AWB writes:" << tm.size() << "(exp 5)"
                << (miscOk && awbOk ? "OK" : "MISMATCH");
        if (!miscOk || !awbOk) ok = false;

        // Knee LUT (config-driven, 10-бит пары → 3 банка + финализация).
        const auto knee = alg.LoadKneeLut("IMX274", "THORACOSCOPE");
        pl.ClearTrace();
        alg.SetCurKneeLut(knee);   // реф.: данные в массив AlgParaManager → void SetKneeLut
        pl.SetKneeLut();
        const auto &tk = pl.Trace();
        const int kpairs = knee.size() / 2;
        // 3 банка × pairs + 1 финализация.
        bool kneeOk = knee.size() == 1024 && tk.size() == kpairs*3 + 1 &&
                      tk[0].first == 0xa1930800 && tk[1].first == 0xa1931000 &&
                      tk[2].first == 0xa1931800 && tk.last().first == 0xa1930000 &&
                      (tk.last().second & 0x2) &&
                      tk[0].second == ((unsigned(knee[1])&0x3ff)<<16 | (unsigned(knee[0])&0x3ff));
        qInfo() << "Knee LUT size=" << knee.size() << "writes=" << tk.size()
                << "(exp" << (kpairs*3+1) << ")" << (kneeOk ? "OK" : "MISMATCH");
        if (!kneeOk) ok = false;

        // Gamma LUT (реф.: 10-бит пары → 3 банка 0xa1830800/1000/1800 + защёлка).
        {
            const auto gp = alg.LoadGammaPara("IMX274", "ARTHROSCOPE");
            const QVector<int> glut = AlgParaManager::CalGammaLut(gp);
            pl.ClearTrace();
            alg.SetCurGammaLut(glut);   // реф.: данные в массив AlgParaManager → void SetGammaLut
            pl.SetGammaLut();
            const auto &tg = pl.Trace();
            const int gpairs = glut.size() / 2;
            const unsigned gv0 = (unsigned(glut.value(1)) & 0x3ff) << 16 |
                                 (unsigned(glut.value(0)) & 0x3ff);
            bool gammaOk = gpairs > 0 && tg.size() == gpairs*3 + 1 &&
                           tg[0].first == 0xa1830800 && tg[0].second == gv0 &&
                           tg[1].first == 0xa1831000 && tg[2].first == 0xa1831800 &&
                           tg[3].first == 0xa1830804 &&
                           tg.last().first == 0xa1830000 && (tg.last().second & 0x2);
            qInfo() << "Gamma LUT size=" << glut.size() << "writes=" << tg.size()
                    << "(exp" << (gpairs*3+1) << ")" << (gammaOk ? "OK" : "MISMATCH");
            if (!gammaOk) ok = false;
        }

        // Iris table (config-driven, 8040 значений → 1005 записей, упаковка 8/регистр).
        const auto iris = alg.LoadIrisTable("IMX274", "1920X1080");
        pl.ClearTrace();
        alg.SetCurIrisTable(iris);   // реф.: данные в массив AlgParaManager → SetIrisTable(shift)
        pl.SetIrisTable(0);          // shift 0
        const auto &ti = pl.Trace();
        unsigned expFirst = 0;
        if (iris.size() >= 8)
            for (int k = 0; k < 8; ++k) expFirst |= (unsigned(iris[k]) >> 0) << (k*4);
        bool irisOk = iris.size() == 8040 && ti.size() == 8040/8 &&
                      ti.first().first == 0xa18a8000 &&
                      ti[1].first == 0xa18a8004 && ti.first().second == expFirst;
        qInfo() << "Iris size=" << iris.size() << "writes=" << ti.size()
                << "(exp 1005) first=0x" + QString::number(ti.isEmpty()?0:ti.first().second,16)
                << (irisOk ? "OK" : "MISMATCH");
        if (!irisOk) ok = false;

        // Aurora offset + VideoCaptureArea (знак-величина) + histogram-триггер.
        pl.ClearTrace();
        pl.SetAuroraOffset(0x12, 0x34);
        pl.SetVideoCaptureArea(10, -20);      // enc(10)=20, enc(-20)=(40|0x100)&0x1ff=0x128
        unsigned short hist[256] = {0};
        pl.ReadBrightnessHistogramValue(hist, 256);   // десктоп: триггер 1 + сброс 0 (реф.)
        pl.ReadIrisValue();                            // read → 0 (нет /dev/mem)
        const auto &tx = pl.Trace();
        bool auxOk = tx.size() == 4 &&
                     tx[0].first == 0xa004a02c && tx[0].second == (0x12u | (0x34u<<8)) &&
                     tx[1].first == 0xa18d0008 && tx[1].second == (20u | (0x128u<<16)) &&
                     tx[2].first == 0xa18a0010 && tx[2].second == 1 &&   // hist триггер=1
                     tx[3].first == 0xa18a0010 && tx[3].second == 0;     // hist сброс=0 (реф.)
        qInfo() << "aux writes:" << tx.size() << "(exp 4: aurora/capture/hist-trig1/hist-trig0)"
                << (auxOk ? "OK" : "MISMATCH");
        if (!auxOk) ok = false;

        // Тип диафрагмы камеры (кодировка type/subtype).
        pl.ClearTrace();
        pl.SetCameraIrisType(0, 0);   // → 0x530
        pl.SetCameraIrisType(1, 0);   // → 0x431
        pl.SetCameraIrisType(2, 5);   // → 0x132
        pl.SetCameraIrisType(2, 0);   // → 0x232
        const auto &tc2 = pl.Trace();
        bool irisTypeOk = tc2.size() == 4 &&
                          tc2[0].second == 0x530 && tc2[1].second == 0x431 &&
                          tc2[2].second == 0x132 && tc2[3].second == 0x232 &&
                          tc2[0].first == 0xa18a0000;
        qInfo() << "irisType writes:" << tc2.size() << "(exp 4)" << (irisTypeOk ? "OK" : "MISMATCH");
        if (!irisTypeOk) ok = false;

        // SetVideoArea → AlgParaManager::resize (без прямой записи в PL).
        pl.SetVideoArea(1280, 960);
        const bool areaOk = alg.VideoWidth() == 1280 && alg.VideoHeight() == 960;
        qInfo() << "VideoArea resize:" << alg.VideoWidth() << "x" << alg.VideoHeight()
                << (areaOk ? "OK" : "MISMATCH");
        if (!areaOk) ok = false;

        // Линза/стекло/Aurora-сброс (реф. SetLens/SetGlassType/AuroraTxReset).
        alg.SetCurLensParam(0x2ab);   // реф. AlgPara 0x7a40
        pl.ClearTrace();
        pl.SetLens(1);                // enable=1: 0xa1890000=1, 0xa1890004=param
        pl.SetLens(0);                // enable=0: 0xa1890000=0
        pl.SetGlassType(7);           // игнор тип → SetLens(0): 0xa1890000=0
        pl.AuroraTxReset();           // 0xa1000014 1→0
        const auto &tln = pl.Trace();
        const bool lensOk = tln.size() == 6 &&
                            tln[0].first == 0xa1890000 && tln[0].second == 1 &&
                            tln[1].first == 0xa1890004 && tln[1].second == 0x2ab &&
                            tln[2].first == 0xa1890000 && tln[2].second == 0 &&
                            tln[3].first == 0xa1890000 && tln[3].second == 0 &&
                            tln[4].first == 0xa1000014 && tln[4].second == 1 &&
                            tln[5].first == 0xa1000014 && tln[5].second == 0;
        qInfo() << "lens/glass/aurora writes:" << tln.size() << "(exp 6)"
                << (lensOk ? "OK" : "MISMATCH");
        if (!lensOk) ok = false;

        qInfo() << (ok ? "plreg: PASS" : "plreg: FAIL");
        return ok ? 0 : 6;
    }

    // Self-test VIST/SFI-матрицы и Denoise LUT (реф. SetVistMatrix/SetDenoiseLut).
    if (screen == "filt") {
        const QString sensor = "IMX274", res = "1920X1080";
        AlgParaManager &alg = AlgParaManager::GetInstance();
        KPlControl pl;
        pl.EnableTrace(true);

        // --- VIST ---
        const QVector<unsigned> vm = alg.LoadVistMatrix(AlgParaManager::VIST, sensor, res);
        qInfo() << "VIST matrix values:" << vm.size();
        pl.ClearTrace();
        pl.SetVistSwitch(true);
        pl.SetVistMatrix(vm.constData(), vm.size());
        const auto &tv = pl.Trace();
        // Ожидаем: switch(0xa18e0000)+inv(0xa1840000) + пары(0xa18e0004..) + хвост(0xa18e0014).
        bool vistOk = vm.size() == 9 && tv.size() == 2 + 4 + 1;
        if (vistOk)
            vistOk = tv[0].first == 0xa18e0000 && tv[1].first == 0xa1840000 &&
                     tv[2].first == 0xa18e0004 && tv[5].first == 0xa18e0010 &&
                     tv.last().first == 0xa18e0014 &&
                     tv[2].second == (vm[0] | (vm[1] << 16)) &&
                     tv.last().second == (vm[8] & 0xffff);
        qInfo() << "VIST writes:" << tv.size() << "(exp 7)" << (vistOk ? "OK" : "MISMATCH");

        // --- Denoise ---
        const auto dp = alg.LoadDenoisePara(sensor, AlgParaManager::VIST, 2);
        qInfo() << "Denoise L2: valid=" << dp.valid << " kernelG=" << dp.kernelG.size()
                << "kernelRB=" << dp.kernelRB.size() << "lut=" << dp.lut.size()
                << "dpc=" << dp.dpcT1 << dp.dpcT2;
        AlgParaManager::DenoisePlData d;   // реф.: данные в массив AlgParaManager
        d.dpc[0] = d.dpc[1] = dp.dpcT1; d.dpc[2] = d.dpc[3] = dp.dpcT2;
        d.kernelG = dp.kernelG; d.kernelRB = dp.kernelRB; d.lut = dp.lut;
        alg.SetCurDenoise(d);
        pl.ClearTrace();
        pl.SetDenoiseLut();   // void — читает CurDenoise
        pl.SetDenoiseLevel(2);
        const auto &td = pl.Trace();
        const int expDen = 4 + 41*4 + 25*4 + 256*4 + 1;   // header+kernelG+kernelRB+lut+level
        bool denOk = dp.valid && td.size() == expDen &&
                     td[0].first == 0xa1941010 && td[4].first == 0xa1941600 &&
                     td.last().first == 0xa1940008 && unsigned(td.last().second) == 2u;
        qInfo() << "Denoise writes:" << td.size() << "(exp" << expDen << ")"
                << (denOk ? "OK" : "MISMATCH");

        const bool ok = vistOk && denOk;
        qInfo() << (ok ? "filt: PASS" : "filt: FAIL");
        return ok ? 0 : 8;
    }

    // Self-test аккаунтов/настроек: вход, MD5, блокировка, KSystemSet-роундтрип.
    if (screen == "account") {
        const QString ini = outFile.isEmpty() ? "/tmp/endo_system.ini" : outFile;
        QFile::remove(ini);
        KAccount &acc = KAccount::GetInstance();
        acc.SetConfigFile(ini);

        // Дефолтный пароль = MD5("admin").
        const QString md5admin = KAccount::ConvertPasswordToMD5("admin");
        const bool md5ok = md5admin == "21232f297a57a5a743894a0e4a801fc3" &&
                           acc.GetAdminDefaultPassWord() == md5admin;
        qInfo() << "MD5(admin)=" << md5admin << "ok=" << md5ok;

        // Валидатор пароля: обычный ок, с запрещённой CJK-пунктуацией — нет.
        const bool valOk = KAccount::ValidateIfPWValid("Pass123") &&
                           !KAccount::ValidateIfPWValid(QString::fromUtf8("пароль，"));

        // Вход дефолтным паролем.
        const auto r1 = acc.Login("admin", "admin");
        // Неверный пароль kLockThreshold раз → блокировка.
        for (int i = 0; i < KAccount::LockFailThreshold(); ++i)
            acc.Login("admin", "wrong");
        const bool locked = acc.GetAccountLockStatus("admin");
        const auto rLocked = acc.Login("admin", "admin");   // заблокирован → RoleNone
        acc.ResetAccountLockInfo("admin");
        const auto r2 = acc.Login("admin", "admin");         // после сброса — успех

        // Смена пароля.
        acc.SaveAdminPassWord("NewPass1");
        const bool changed = acc.IsAdminPasswordChange();
        acc.ResetAccountLockInfo("admin");
        const auto r3 = acc.Login("admin", "NewPass1");

        qInfo() << "Login default role=" << r1 << " locked=" << locked
                << " lockedLogin=" << rLocked << " afterReset=" << r2
                << " changed=" << changed << " newPwLogin=" << r3;

        // KSystemSet роундтрип.
        KSystemSet &ss = KSystemSet::GetInstance();
        ss.SetConfigFile(ini);
        ss.SetLanguage("Russian"); ss.SetDateFormat("dd.MM.yyyy");
        ss.SetBrightness(70); ss.SetAutoLogin(true); ss.SetForceLogoutTime(15);
        const bool ssOk = ss.Language() == "Russian" && ss.DateFormat() == "dd.MM.yyyy" &&
                          ss.Brightness() == 70 && ss.AutoLogin() &&
                          ss.ForceLogoutTime() == 15;
        qInfo() << "SystemSet: lang=" << ss.Language() << "date=" << ss.DateFormat()
                << "bright=" << ss.Brightness() << "autologin=" << ss.AutoLogin();

        const bool ok = md5ok && valOk && r1 == KAccount::RoleAdmin && locked &&
                        rLocked == KAccount::RoleNone && r2 == KAccount::RoleAdmin &&
                        changed && r3 == KAccount::RoleAdmin && ssOk;
        qInfo() << (ok ? "account: PASS" : "account: FAIL");
        return ok ? 0 : 12;
    }

    // Self-test оркестрации KVideoSet: Set*Level → KVideoParam + регистры (по trace).
    if (screen == "videoset") {
        AlgParaManager::GetInstance().LoadColEnhLevels("IMX274");   // для резолва ColorEnh
        KPlControl pl;
        pl.EnableTrace(true);
        KVideoSet &vs = KVideoSet::Instance();
        vs.AttachPl(&pl);

        vs.SetColEnhLevel(5);      // → KVideoParam + ColorEnh регистры (enable+значение)
        vs.SetDenoiseLevel(2);     // → 0xa1940008
        vs.SetBrightEQLevel(1);    // → 0xa1950000 (enable)
        vs.SetColorRValue(0, 0x120);  // → 0xa1870004
        vs.SetColorBValue(0, 0x40);   // → 0xa1870008
        vs.SetColorCValue(0, 0x100);  // → 0xa1870000
        vs.SetZoomLevel(3);        // → 0xa18d0004
        vs.SetOperationMode(3);    // VIST → SetVistSwitch (0xa18e0000+0xa1840000)

        const auto &tr = pl.Trace();
        // Проверка держателя (Get*) + ключевых регистров.
        const bool paramOk = vs.GetColEnhLevel() == 5 && vs.GetDenoiseLevel() == 2 &&
                             vs.GetBrightEQLevel() == 1 && vs.GetZoomLevel() == 3;
        bool regOk = false;
        int denoiseReg = 0, zoomReg = 0, rReg = 0, vistReg = 0;
        for (const auto &w : tr) {
            if (w.first == 0xa1940008 && w.second == 2) denoiseReg++;
            if (w.first == 0xa18d0004 && w.second == 3) zoomReg++;
            if (w.first == 0xa1870004 && w.second == 0x120) rReg++;
            if (w.first == 0xa18e0000 && w.second == 1) vistReg++;
        }
        regOk = denoiseReg && zoomReg && rReg && vistReg;
        qInfo() << "держатель ок:" << paramOk << "| регистры (denoise/zoom/R/vist):"
                << denoiseReg << zoomReg << rReg << vistReg << "| всего записей:" << tr.size();

        // ResetVideoParam → дефолты.
        vs.ResetVideoParam();
        const bool resetOk = vs.GetColEnhLevel() == 1 && vs.GetDenoiseLevel() == 0 &&
                             vs.GetZoomLevel() == 0;
        qInfo() << "после reset — ColEnh/Denoise/Zoom:" << vs.GetColEnhLevel()
                << vs.GetDenoiseLevel() << vs.GetZoomLevel();

        const bool ok = paramOk && regOk && resetOk;
        qInfo() << (ok ? "videoset: PASS" : "videoset: FAIL");
        return ok ? 0 : 27;
    }

    // Self-test калибровки видео: диапазоны смещения центра по типу прошивки (1:1 X2000)
    // + roundtrip записи/чтения области отображения (SaveDisplayArea → display-ini).
    if (screen == "videocal") {
        // 1) Диапазоны центра (чистая логика, 1:1 с бинарником).
        auto H = &KVideoCal::GetCenterOffsetHorizontalRange;
        auto V = &KVideoCal::GetCenterOffsetVerticalRange;
        const bool rangesOk =
            H(KVideoCal::FW_OV2740)          == qMakePair(-4, 4)   &&
            V(KVideoCal::FW_OV2740)          == qMakePair(-4, 4)   &&
            H(KVideoCal::FW_OH01A_928X768)   == qMakePair(-16, 16) &&
            V(KVideoCal::FW_OH01A_928X768)   == qMakePair(-10, 10) &&
            H(KVideoCal::FW_OH01A_768X928)   == qMakePair(-16, 16) &&
            V(KVideoCal::FW_OH01A_768X928)   == qMakePair(-10, 10) &&
            H(KVideoCal::FW_OV2740_1280X960) == qMakePair(-16, 16) &&
            V(KVideoCal::FW_OV2740_1280X960) == qMakePair(-16, 16) &&
            H(KVideoCal::FW_OV2740_1024X1024)== qMakePair(-16, 16) &&
            V(KVideoCal::FW_OV2740_1024X1024)== qMakePair(-16, 16) &&
            H(KVideoCal::FW_IMX274)          == qMakePair(0, 0)    &&
            V(KVideoCal::FW_IMX274)          == qMakePair(0, 0)    &&
            H(KVideoCal::FW_OV6946)          == qMakePair(0, 0)    &&
            H(KVideoCal::FW_OCHFA_720X720)   == qMakePair(0, 0);
        qInfo() << "диапазоны центра ок:" << rangesOk
                << "| OH01A H:" << H(KVideoCal::FW_OH01A_928X768)
                << "V:" << V(KVideoCal::FW_OH01A_928X768);

        // 2) Roundtrip области: сеем реальный display-ini в tmp-root и пишем/читаем.
        const QString src = QDir(KSystem::DisplayConfigPath())
                                .absoluteFilePath("IMG19201080-UI19201080.ini");
        const QString tmpRoot = "/tmp/endo_vcal";
        const QString tmpDisp = tmpRoot + "/system/display";
        QDir().mkpath(tmpDisp);
        const QString dst = tmpDisp + "/IMG19201080-UI19201080.ini";
        QFile::remove(dst);
        const bool seeded = QFile::copy(src, dst);
        qputenv("ENDO_ROOT", tmpRoot.toUtf8());   // переключаем корень на tmp

        KDisplayOption &opt = KDisplayOption::Instance();
        opt.SelectLayout(QSize(1920, 1080), QSize(1920, 1080));
        const bool layoutOk = opt.LayoutFile() == dst;

        // Чтение из посева: freeze-rect и раскладка рабочего стола (hard-endo).
        const QRect freeze = opt.getFreezeVideoRect();
        const auto view = opt.GetDesktopViewConf(/*hardEndo=*/true);
        const bool readOk = freeze == QRect(16, 16, 416, 234) &&
                            view.value("osd") == QRect(6, 330, 290, 960) &&
                            view.contains("endobtnguide");
        qInfo() << "layout:" << layoutOk << "| freeze:" << freeze
                << "| osd:" << view.value("osd") << "| view-ключей:" << view.size();

        // Запись области отображения и обратное чтение.
        const QRect wImg(10, 20, 1900, 1040), wUi(0, 60, 1920, 1080);
        const bool saved = KVideoCal::SaveDisplayArea(wImg, wUi);
        // Свежий инстанс QSettings увидит записанное (SetRect делает sync()).
        const bool writeOk = opt.getVideoRectForImgPro() == wImg &&
                             opt.getVideoRectForUI(0) == wUi;
        qInfo() << "saved:" << saved << "| imgPro:" << opt.getVideoRectForImgPro()
                << "| ui:" << opt.getVideoRectForUI(0);

        const bool ok = rangesOk && seeded && layoutOk && readOk && saved && writeOk;
        qInfo() << (ok ? "videocal: PASS" : "videocal: FAIL");
        return ok ? 0 : 29;
    }

    // Self-test апдейт-пайплайна: манифест update.ini + решение «что обновлять»
    // (KUpdateManifest поверх KVersionConfig + KUpdateConf matchedversion).
    if (screen == "update") {
        const QString root = "/tmp/endo_upd";
        QDir().mkpath(root);
        const QString manifest = root + "/update.ini";
        const QString instFile = root + "/version.ini";
        const QString matchFile = root + "/matchedversion.ini";
        for (const QString &f : {manifest, instFile, matchFile}) QFile::remove(f);

        // 1) Версии пакета (манифест update.ini): app обновляется, hmi актуален,
        //    lcd несовместим, pap отсутствует в пакете.
        { QSettings pkg(manifest, QSettings::IniFormat);
          pkg.setValue("app/Version", "2.0.0");
          pkg.setValue("hmi/Version", "1.5.0");
          pkg.setValue("lcd/Version", "9.9.9"); }  // не в matched → несовместим

        // 2) Установленные версии (KVersionConfig): app старее, hmi совпадает.
        KVersionConfig &ver = KVersionConfig::GetInstance();
        ver.SetConfigFile(instFile);
        ver.SetVersion("app", "1.0.0");
        ver.SetVersion("hmi", "1.5.0");
        ver.SetVersion("lcd", "1.0.0");

        // 3) Матрица совместимости (KUpdateConf): для lcd допустим только 1.0.0/1.1.0.
        { QSettings mv(matchFile, QSettings::IniFormat);
          mv.setValue("MatchedVersion/app", QStringList{"1.0.0", "2.0.0"});
          mv.setValue("MatchedVersion/lcd", QStringList{"1.0.0", "1.1.0"}); }
        KUpdateConf::GetInstance().SetConfigFile(matchFile);

        // 4) Прогон решения.
        KUpdateManifest &um = KUpdateManifest::GetInstance();
        um.SetUpdateRoot(root);
        const auto st = um.CheckUpdateItems();

        using S = KUpdateManifest;
        const bool itemsOk = S::UpdateItems().contains("papp00") &&
                             S::UpdateItems().size() == 12;
        const bool decideOk =
            st.value("app") == S::NeedUpdate &&      // 1.0.0 → 2.0.0 (в matched)
            st.value("hmi") == S::UpToDate &&        // 1.5.0 == 1.5.0
            st.value("lcd") == S::Incompatible &&    // 9.9.9 не в matched
            st.value("pap") == S::NoPackage;         // нет в пакете
        // Флаги IsNeedUpdate записаны в манифест (реф. SetItemNeedUpdate).
        const bool flagsOk = um.GetItemNeedUpdate("app") &&
                             !um.GetItemNeedUpdate("hmi") &&
                             !um.GetItemNeedUpdate("lcd");

        qInfo() << "items:" << itemsOk << "| решение app/hmi/lcd/pap:"
                << st.value("app") << st.value("hmi") << st.value("lcd") << st.value("pap")
                << "| флаги (app/hmi):" << um.GetItemNeedUpdate("app") << um.GetItemNeedUpdate("hmi")
                << "| root:" << um.GetUpdateRoot();

        // Проверка целостности файлов пакета (реф. *FileMd5Code).
        const QString fwFile = root + "/app.bin";
        { QFile bf(fwFile); bf.open(QIODevice::WriteOnly); bf.write("firmware-payload-v2"); bf.close(); }
        const QString calcMd5 = KUpdateManifest::CalcFileMd5(fwFile);
        // md5-файл формата md5sum ("<md5>  <имя>").
        const QString md5File = root + "/md5sum.txt";
        { QFile mf(md5File); mf.open(QIODevice::WriteOnly | QIODevice::Text);
          mf.write((calcMd5 + "  app.bin\n").toUtf8());
          mf.write("deadbeef00000000deadbeef00000000  other.bin\n"); mf.close(); }
        const bool md5Ok = calcMd5.size() == 32 &&
                           KUpdateManifest::ReadFileMd5(md5File, "app.bin") == calcMd5 &&
                           KUpdateManifest::CheckFileMd5(fwFile, md5File) &&           // совпал
                           KUpdateManifest::ReadFileMd5(md5File, "nope.bin").isEmpty();
        // Порча файла → проверка не проходит.
        { QFile bf(fwFile); bf.open(QIODevice::WriteOnly); bf.write("tampered"); bf.close(); }
        const bool tamperOk = !KUpdateManifest::CheckFileMd5(fwFile, md5File);
        qInfo() << "md5:" << calcMd5 << "check ok:" << md5Ok << "tamper detected:" << tamperOk;

        const bool ok = itemsOk && decideOk && flagsOk && md5Ok && tamperOk;
        qInfo() << (ok ? "update: PASS" : "update: FAIL");
        return ok ? 0 : 30;
    }

    // Self-test каталога шаблонов отчёта: TempletInfo.xml → шаблоны + департаменты
    // (KSysReportTempletCfg поверх KReportTemplateManager).
    if (screen == "templetcfg") {
        KSysReportTempletCfg &cfg = KSysReportTempletCfg::GetInstance();
        const bool loaded = cfg.Reload();
        const QStringList names = cfg.TempletNames();
        qInfo() << "загружено:" << loaded << "| шаблоны:" << names;

        bool found = false;
        const KTempletBaseInfo np22 = cfg.GetTemplateInfoByName("NP-2x2", &found);
        const QStringList depts = cfg.GetDeptsOfTemplate("NP-2x2");
        const QStringList byDept = cfg.GetTempletNamesByDept("KW_NP-2x2");
        const QString def = cfg.GetDefaultTempletByDept("KW_NP-2x2");
        qInfo() << "NP-2x2 found:" << found << "factory:" << np22.factory
                << "modifydate:" << np22.modifyDate << "| depts:" << depts
                << "| by-dept:" << byDept << "| default:" << def;

        // Библиотека шаблонов (TempletLibInfo.xml — полные имена layout-файлов).
        const QStringList libNames = cfg.TempletLibNames();
        bool libFound = false;
        const KTempletBaseInfo lib = cfg.GetLibTemplateInfoByName("ReportTemplateNP-2x2", &libFound);
        qInfo() << "библиотека:" << libNames << "| lib NP-2x2 found:" << libFound
                << "depts:" << lib.deptNames();
        const bool libOk = libNames.contains("ReportTemplateNP-2x2") &&
                           libNames.contains("ReportTemplateNP-1x4") &&
                           libFound && lib.factory && lib.hasDept("KW_NP-2x2");

        const bool ok = loaded && names.contains("NP-2x2") && names.contains("NP-1x4") &&
                        found && np22.factory && np22.modifyDate == "factory" &&
                        depts == QStringList{"KW_NP-2x2"} &&
                        byDept == QStringList{"NP-2x2"} && def == "NP-2x2" &&
                        cfg.GetTemplateInfoByName("no-such", &found).name.isEmpty() &&
                        libOk;
        qInfo() << (ok ? "templetcfg: PASS" : "templetcfg: FAIL");
        return ok ? 0 : 31;
    }

    // Self-test постраничного БД-доступа к отчётам (реф. KReportDBTableHandler):
    // пагинация + запрос по ключевому слову + список ключей.
    if (screen == "reportdb") {
        const QString db = "/tmp/endo_reportdb.db";
        QFile::remove(db);
        KEntityReport &er = KEntityReport::Instance();
        er.OpenDb(db);
        // 6 отчётов: 2 с "Gastritis" в диагнозе, 1 с "Gastritis" в disease.
        for (int i = 1; i <= 6; ++i) {
            ReportEntity r;
            r.accessionNumber = QString("A%1").arg(i, 4, 10, QChar('0'));
            r.templateName = "NP-2x2";
            r.diagnosis = (i % 2 == 0) ? "Chronic gastritis" : "Normal mucosa";
            r.diseaseName = (i == 3) ? "Gastritis" : "None";
            er.SaveReport(r);
        }
        const int total = er.GetReportNumber();
        const auto page1 = er.GetPageRecord(0, 4);     // первые 4 по ключу
        const auto page2 = er.GetPageRecord(4, 4);     // остаток (2)
        const QStringList keys = er.GetAllRecordMainKey();
        const int gastrNum = er.GetQueryRecordNum("astrit");  // Gastritis/gastritis (LIKE)
        const auto gastrPage = er.QueryPageRecord("astrit", 0, 10);
        er.CloseDb();

        qInfo() << "всего:" << total << "| стр1:" << page1.size() << "стр2:" << page2.size()
                << "| ключи:" << keys << "| gastritis num:" << gastrNum;
        const bool ok = total == 6 &&
                        page1.size() == 4 && page2.size() == 2 &&
                        page1.first().accessionNumber == "A0001" &&   // порядок по ключу
                        page2.last().accessionNumber == "A0006" &&
                        keys.size() == 6 && keys.first() == "A0001" &&
                        gastrNum == 4 && gastrPage.size() == 4;       // A2,A4,A6 (диагноз) + A3 (disease)
        qInfo() << (ok ? "reportdb: PASS" : "reportdb: FAIL");
        return ok ? 0 : 32;
    }

    // Self-test нумерации файлов снимков/видео (реф. KSaveFile::FormatFlowNumber +
    // разбор имён + поиск следующего номера в каталоге).
    if (screen == "savefile") {
        // Форматирование: 3 цифры zero-pad, overflow "999^".
        const bool fmtOk = KSaveFile::FormatFlowNumber(0) == "000" &&
                           KSaveFile::FormatFlowNumber(1) == "001" &&
                           KSaveFile::FormatFlowNumber(42) == "042" &&
                           KSaveFile::FormatFlowNumber(999) == "999" &&
                           KSaveFile::FormatFlowNumber(1000) == "999^" &&
                           KSaveFile::MakeFileName(7, false) == "007.jpg" &&
                           KSaveFile::MakeFileName(12, true) == "012.mp4";
        // Разбор номера из имени.
        const bool parseOk = KSaveFile::FlowNumberFromName("003.jpg") == 3 &&
                             KSaveFile::FlowNumberFromName("012.mp4") == 12 &&
                             KSaveFile::FlowNumberFromName("999^.jpg") == 999 &&
                             KSaveFile::FlowNumberFromName("readme.txt") == -1;

        // Поиск следующего номера в каталоге со снимками.
        const QString dir = "/tmp/endo_savefile";
        QDir().mkpath(dir);
        for (const QString &f : QDir(dir).entryList({"*.jpg", "*.mp4"}, QDir::Files))
            QFile::remove(dir + "/" + f);
        const bool emptyNext = KSaveFile::NextFlowNumber(dir) == 0;  // пустой каталог → 0
        for (const QString &n : {"001.jpg", "002.jpg", "005.mp4", "notes.txt"})
            { QFile ff(dir + "/" + n); ff.open(QIODevice::WriteOnly); ff.close(); }
        const int mx = KSaveFile::FindMaxFileFlowNumber(dir);
        const int next = KSaveFile::NextFlowNumber(dir);
        qInfo() << "fmt:" << fmtOk << "parse:" << parseOk
                << "| пустой→" << emptyNext << "| max:" << mx << "next:" << next;

        const bool ok = fmtOk && parseOk && emptyNext && mx == 5 && next == 6;
        qInfo() << (ok ? "savefile: PASS" : "savefile: FAIL");
        return ok ? 0 : 33;
    }

    // Self-test OSD-конфига: список функций + чтение реального osd.ini (кнопки→функции)
    // + roundtrip записи (реф. KUserOsdSet).
    if (screen == "osdset") {
        KUserOsdSet &osd = KUserOsdSet::GetInstance();
        // 1) Список функций (фикс. по ID).
        const QStringList fns = KUserOsdSet::GetFunctionNameList();
        const bool fnOk = fns.size() == 12 && fns.first() == "TR_Frz" &&
                          KUserOsdSet::GetFunctionName(11) == "TR_Rcd" &&
                          KUserOsdSet::GetFunctionName(6) == "TR_Brtnss+" &&
                          KUserOsdSet::GetFunctionName(99).isEmpty();

        // 2) Чтение реального osd.ini прошивки (ButtomA Short=6, Long=9; ButtomB 7/11).
        using B = KUserOsdSet;
        const bool readOk =
            osd.GetButtonFunctionId(B::BTN_A, B::PRESS_SHORT) == 6 &&
            osd.GetButtonFunctionId(B::BTN_A, B::PRESS_LONG)  == 9 &&
            osd.GetButtonFunctionId(B::BTN_B, B::PRESS_SHORT) == 7 &&
            osd.GetButtonFunctionId(B::BTN_B, B::PRESS_LONG)  == 11 &&
            osd.GetButtonFunctionId(B::BTN_M, B::PRESS_SHORT) == 0 &&
            osd.GetFootSwitchFunctionId(2) == 1 &&
            osd.GetOperationMode() == 0 && osd.GetIrisMode() == 0;
        qInfo() << "функций:" << fns.size() << "| ButtomA short/long:"
                << osd.GetButtonFunctionId(B::BTN_A, B::PRESS_SHORT)
                << osd.GetButtonFunctionId(B::BTN_A, B::PRESS_LONG)
                << "→" << B::GetFunctionName(osd.GetButtonFunctionId(B::BTN_A, B::PRESS_SHORT))
                << B::GetFunctionName(osd.GetButtonFunctionId(B::BTN_A, B::PRESS_LONG));

        // 3) Roundtrip записи в tmp-копию osd.ini.
        const QString tmp = "/tmp/endo_osd.ini";
        QFile::remove(tmp);
        { QSettings s(tmp, QSettings::IniFormat);       // затравка
          s.setValue("ButtomM/ShortPress", 0); s.sync(); }
        osd.SetConfigFile(tmp);
        osd.SaveButtonId(B::BTN_M, B::PRESS_SHORT, B::FUNC_RECORD);   // M-short → Rcd(11)
        osd.SaveOperationMode(3);
        osd.SaveIrisMode(2);
        const bool writeOk =
            osd.GetButtonFunctionId(B::BTN_M, B::PRESS_SHORT) == 11 &&
            osd.GetOperationMode() == 3 && osd.GetIrisMode() == 2;
        osd.SetConfigFile(QString());   // вернуть дефолтный путь

        const bool ok = fnOk && readOk && writeOk;
        qInfo() << (ok ? "osdset: PASS" : "osdset: FAIL");
        return ok ? 0 : 34;
    }

    // Self-test сервиса БД (реф. KEntityService): PRAGMA-окружение + бэкап/восстановление.
    if (screen == "dbservice") {
        const QString dir = "/tmp/endo_dbsvc";
        QDir().mkpath(dir);
        const QString dbPath = dir + "/HD-2000.dat";
        QFile::remove(dbPath);

        // Создать БД с данными, применить окружение (реф. SetEnvironment).
        const char *conn = "endo_dbsvc";
        { QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
          db.setDatabaseName(dbPath); db.open();
          QSqlQuery q(db);
          q.exec("CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT)");
          q.exec("INSERT INTO t (v) VALUES ('alpha')");
        }
        const bool envOk = KEntityService::ApplyEnvironment(conn);
        // Проверить, что PRAGMA применились.
        bool syncOk = false, jrnOk = false;
        { QSqlQuery q(QSqlDatabase::database(conn));
          if (q.exec("PRAGMA synchronous") && q.next()) syncOk = q.value(0).toInt() == 1; // NORMAL=1
          if (q.exec("PRAGMA journal_mode") && q.next())
              jrnOk = q.value(0).toString().compare("delete", Qt::CaseInsensitive) == 0;
        }
        { QSqlDatabase::database(conn).close(); }
        QSqlDatabase::removeDatabase(conn);

        // Бэкап (реф. Recover): <base>_<stamp>.bak; затем изменить БД и восстановить.
        const QString stamp = "20260715_120000";   // метка снаружи (время не off-device)
        const QString bak = KEntityService::BackupDatabase(dbPath, dir + "/bak", stamp);
        const bool bakOk = !bak.isEmpty() && QFile::exists(bak) &&
                           QFileInfo(bak).fileName() == "HD-2000_20260715_120000.bak" &&
                           KEntityService::DatabaseFileName() == "HD-2000.dat";

        // Испортить основную БД, восстановить из бэкапа, проверить данные.
        { QFile f(dbPath); f.open(QIODevice::WriteOnly | QIODevice::Truncate); f.close(); }
        const bool recOk = KEntityService::RecoverDatabase(bak, dbPath);
        bool dataOk = false;
        { QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
          db.setDatabaseName(dbPath); db.open();
          QSqlQuery q(db);
          if (q.exec("SELECT v FROM t WHERE id=1") && q.next())
              dataOk = q.value(0).toString() == "alpha";
        }
        { QSqlDatabase::database(conn).close(); }
        QSqlDatabase::removeDatabase(conn);

        qInfo() << "env:" << envOk << "sync/jrn:" << syncOk << jrnOk
                << "| бэкап:" << QFileInfo(bak).fileName() << bakOk
                << "| восстановление:" << recOk << "данные:" << dataOk;
        const bool ok = envOk && syncOk && jrnOk && bakOk && recOk && dataOk;
        qInfo() << (ok ? "dbservice: PASS" : "dbservice: FAIL");
        return ok ? 0 : 35;
    }

    // Self-test валидности элементов отчёта (реф. KReportDisplayParam): элемент
    // валиден для показа, если у него есть данные в источнике (или валидный потомок).
    if (screen == "dispparam") {
        // Дерево шаблона: контейнер с 3 текст-элементами + 1 картинка.
        QVector<ReportItem> items;
        ReportItem root; root.name = "block"; root.type = "RT_TABLE_BLOCK";
        auto mk = [](const QString &n, const QString &src) {
            ReportItem i; i.name = n; i.type = "RT_TEXT_BLOCK"; i.dataSrc = src; return i; };
        root.children << mk("diag", "RT_DATASOURCE_PATIENT,RT_DIAGNOSIS");
        root.children << mk("sugg", "RT_DATASOURCE_PATIENT,RT_SUGGESTION");
        root.children << mk("empty", "RT_DATASOURCE_PATIENT,RT_BIOPSY");   // без данных
        items << root;
        { ReportItem img; img.name = "img0"; img.type = "RT_IMAGE_BLOCK";
          img.dataSrc = "RT_DATASOURCE_PERIPHERAL,RT_TEST_IMAGE0"; items << img; }

        // Источник: заполнены diag/sugg и картинка; biopsy — пусто.
        KReportDataSource ds;
        ds.SetPatient("RT_DIAGNOSIS", "Chronic gastritis");
        ds.SetPatient("RT_SUGGESTION", "Recheck 6mo");
        ds.SetImage(0, "/data/exam/A1/0.jpeg", "M0");

        KReportDisplayParam dp;
        const int n = dp.UpdateTemplateDisplayParam(items, ds);
        // diag, sugg, img0 валидны; empty — нет; block валиден (есть валидные потомки).
        const bool markOk = dp.IsItemValid("diag") && dp.IsItemValid("sugg") &&
                            dp.IsItemValid("img0") && dp.IsItemValid("block") &&
                            !dp.IsItemValid("empty");
        qInfo() << "валидных:" << n << "| diag/sugg/img0/block/empty:"
                << dp.IsItemValid("diag") << dp.IsItemValid("sugg") << dp.IsItemValid("img0")
                << dp.IsItemValid("block") << dp.IsItemValid("empty");

        // Эталонное множество (реф. SetRefValidItems) ограничивает добавляемые имена.
        KReportDisplayParam dp2;
        dp2.SetRefValidItems({"diag", "sugg"});
        const bool refOk = dp2.AppendValidItem("diag") &&      // в ref → ок
                           !dp2.AppendValidItem("img0") &&     // вне ref → отклонён
                           dp2.IsItemValid("diag") && !dp2.IsItemValid("img0");
        dp2.Reset();
        const bool resetOk = dp2.ValidCount() == 0;

        const bool ok = n == 4 && markOk && refOk && resetOk;
        qInfo() << (ok ? "dispparam: PASS" : "dispparam: FAIL");
        return ok ? 0 : 36;
    }

    // Self-test конфига сервера выгрузки инфо об эндоскопе (реф. endoinfoserver.ini).
    if (screen == "endoinfo") {
        KEndoInfoServerConfig &c = KEndoInfoServerConfig::GetInstance();
        qInfo() << "конфиг:" << c.ConfigFile();
        qInfo() << "dns1:" << c.Dns1() << "proxy:" << c.Proxy()
                << "post:" << c.EndoInfoPostUrl();
        // Сверка с реальным endoinfoserver.ini прошивки.
        const bool ok = c.IsValid() &&
                        c.Dns1() == "8.8.8.8" && c.Dns2() == "10.10.102.8" &&
                        c.Proxy() == "http://mail.sonoscape.net:90" &&
                        c.LoginUrl() == "http://mail.sonoscape.net:90/auth/onlyLogin" &&
                        c.EndoInfoPostUrl() ==
                            "http://mail.sonoscape.net:90/aconfig/opLog/uploadLog" &&
                        c.PublicKeyFile().endsWith("endoinfoserver/public_key.pem");
        qInfo() << (ok ? "endoinfo: PASS" : "endoinfo: FAIL");
        return ok ? 0 : 37;
    }

    // Self-test конфига пульта/ножного переключателя + IHb (реф. user.ini).
    if (screen == "remoteswitch") {
        KRemoteSwitchConfig &rs = KRemoteSwitchConfig::GetInstance();
        // Резолв ID функций в имена через KUserOsdSet (общий список).
        const int r1 = rs.GetRemoteSwitchFunctionId(1);   // 0 → Frz
        const int r2 = rs.GetRemoteSwitchFunctionId(2);   // 9 → WBalance
        const int r3 = rs.GetRemoteSwitchFunctionId(3);   // 4 → IEnh
        const int r4 = rs.GetRemoteSwitchFunctionId(4);   // 2 → IRIS
        qInfo() << "конфиг:" << rs.ConfigFile();
        qInfo() << "RemoteSwitch 1-4:" << r1 << r2 << r3 << r4
                << "→" << KUserOsdSet::GetFunctionName(r1) << KUserOsdSet::GetFunctionName(r2)
                << KUserOsdSet::GetFunctionName(r3) << KUserOsdSet::GetFunctionName(r4);
        qInfo() << "FootSwitch 1-2:" << rs.GetFootSwitchFunctionId(1) << rs.GetFootSwitchFunctionId(2)
                << "| IHbMode 1-3:" << rs.GetIHbMode(1) << rs.GetIHbMode(2) << rs.GetIHbMode(3);

        // Сверка с реальным user.ini прошивки.
        const bool ok =
            r1 == 0 && r2 == 9 && r3 == 4 && r4 == 2 &&
            KUserOsdSet::GetFunctionName(r1) == "TR_Frz" &&
            KUserOsdSet::GetFunctionName(r2) == "TR_WBalance" &&
            KUserOsdSet::GetFunctionName(r3) == "TR_IEnh" &&
            rs.GetFootSwitchFunctionId(1) == 9 && rs.GetFootSwitchFunctionId(2) == 0 &&
            rs.GetIHbMode(1) == 1 && rs.GetIHbMode(2) == 2 && rs.GetIHbMode(3) == 3;
        qInfo() << (ok ? "remoteswitch: PASS" : "remoteswitch: FAIL");
        return ok ? 0 : 38;
    }

    // Self-test структуры DICOM-датасета (реф. MppsCreateDatasetFormat.xml):
    // дерево DcmItem/SequenceItem с вложенностью.
    if (screen == "dcmfmt") {
        KDicomDatasetFormat fmt;
        const QString xml = QDir(KSystem::UserPresetPath())
            .absoluteFilePath("dicom/MppsCreateDatasetFormat.xml");
        const bool loaded = fmt.Load(xml);
        // Последовательность ScheduledStepAttributesSequence с вложенными тегами.
        bool seqFound = false;
        const auto seq = fmt.FindItem("DCM_ScheduledStepAttributesSequence", &seqFound);
        qInfo() << "загружено:" << loaded << "| корневых:" << fmt.RootCount()
                << "всего узлов:" << fmt.TotalCount()
                << "| ScheduledStepAttrSeq: seq=" << seq.isSequence
                << "детей=" << seq.children.size();

        // Тот же парсер обрабатывает и MppsSetDatasetFormat.xml (Set-вариант).
        KDicomDatasetFormat fmtSet;
        const bool fmtSetOk = fmtSet.Load(QDir(KSystem::UserPresetPath())
            .absoluteFilePath("dicom/MppsSetDatasetFormat.xml")) && fmtSet.TotalCount() > 0;

        const bool ok = loaded && fmt.RootCount() > 5 && fmt.TotalCount() > fmt.RootCount() &&
                        seqFound && seq.isSequence && seq.children.size() >= 3 &&
                        // вложенные теги последовательности
                        fmt.Contains("DCM_StudyInstanceUID") &&
                        fmt.Contains("DCM_AccessionNumber") &&
                        // корневые листья
                        fmt.Contains("DCM_PatientID") &&
                        fmt.Contains("DCM_PerformedProcedureStepStatus") &&
                        !fmt.Contains("DCM_NoSuchTag") &&
                        fmtSetOk;
        qInfo() << (ok ? "dcmfmt: PASS" : "dcmfmt: FAIL");
        return ok ? 0 : 39;
    }

    // Self-test конвертеров дат/времени пациента (реф. KPatientTimeOperation).
    if (screen == "pattime") {
        using P = KPatientTimeOperation;
        // Конверсии форматов (1:1 с бинарником).
        const bool convOk =
            P::ConvertYYYYMMDDToDbDate("20260714") == "2026-07-14" &&
            P::ConvertYYYYMMDDToDicomDate("20260714") == "20260714" &&
            P::ConvertYYYYMMDDToDate("20260714", "dd/MM/yyyy") == "14/07/2026" &&
            P::ConvertYYYYMMDDToDbDate("2026137") == "" &&           // некорректно
            P::ConvertHHMMSSToDbTime("120530") == "12:05:30" &&
            P::ConvertHHMMSSToDicomTime("120530") == "120530" &&
            P::ConvertHHMMSSToDbTime("250000") == "";               // 25ч — невалидно

        // Возраст по дате рождения (опорная дата инъектируется).
        const QDate today(2026, 7, 14);
        const bool ageOk =
            P::GetAgeByBirthDay("1980-05-01", today) == 46 &&       // ДР прошёл
            P::GetAgeByBirthDay("1980-12-31", today) == 45 &&       // ДР ещё не наступил
            P::GetAgeByBirthDay("2026-07-14", today) == 0 &&        // сегодня
            P::GetAgeByBirthDay("2030-01-01", today) == -1 &&       // будущее
            P::GetAgeByBirthDay("плохая-дата", today) == -1;

        // Валидация DICOM-диапазона дат.
        const bool rangeOk =
            P::IsYYYYMMDDDicomDateRange("20260714") &&
            P::IsYYYYMMDDDicomDateRange("20260101-20261231") &&
            !P::IsYYYYMMDDDicomDateRange("2026") &&
            !P::IsYYYYMMDDDicomDateRange("20260714-bad");

        qInfo() << "conv:" << convOk << "age:" << ageOk << "range:" << rangeOk
                << "| 1980-05-01→" << P::GetAgeByBirthDay("1980-05-01", today)
                << "| 20260714→db:" << P::ConvertYYYYMMDDToDbDate("20260714");
        const bool ok = convOk && ageOk && rangeOk;
        qInfo() << (ok ? "pattime: PASS" : "pattime: FAIL");
        return ok ? 0 : 40;
    }

    // Self-test обрезки углов (реф. SetCutCornerPara/SetRoundPara/SetOctagonPara +
    // KPlControl::SetCornerCutWay). Геометрия + стрим 1080 слов в 0xa18c8000.
    if (screen == "cornercut") {
        auto &alg = AlgParaManager::GetInstance();
        const int N = AlgParaManager::kCutCornerLen;   // 1080
        const int W = 1920, H = 400;
        alg.SetCutCornerSize(W, H);

        // --- Круг (way 0), радиус 200, отступ 10 ---
        const int rb = 200, rc = 10;
        alg.SetCutCornerPara(0, rb, rc);
        const QVector<int> round = alg.CutCornerLut(0);
        const int top = rc + (N - H) / 2;              // 350
        const int rows = H / 2 - rc;                   // 190
        bool roundOk = round.size() == N;
        // Симметрия + обнулённые поля.
        for (int i = 0; roundOk && i < N; ++i)
            if (round[i] != round[N - 1 - i]) roundOk = false;
        for (int i = 0; roundOk && i < top; ++i)
            if (round[i] != 0) roundOk = false;
        // Пара точек дуги: buf[top+i] = min(W-2c, 2·√(b²−y²)), y=rows−i.
        auto arcVal = [&](int i){ int y = rows - i;
            double a = double(rb)*rb - double(y)*y;
            int s = a < 0 ? 0 : int(std::sqrt(a));
            return std::min(W - 2*rc, 2*s); };
        if (roundOk && (round[top] != arcVal(0) || round[top + rows - 1] != arcVal(rows-1)))
            roundOk = false;

        // --- Восьмиугольник (way 1), p2=100, p3=50, c=10 ---
        const int p2 = 100, p3 = 50, oc = 10;
        alg.SetCutCornerPara(1, (p2 << 16) | p3, oc);
        const QVector<int> oct = alg.CutCornerLut(1);
        const int otop = oc + (N - H) / 2;             // 350
        const int lo = otop + p3, hi = (N - otop) - p3;
        bool octOk = oct.size() == N;
        for (int i = 0; octOk && i < N; ++i)
            if (oct[i] != oct[N - 1 - i]) octOk = false;
        // Плоская середина = W-2c.
        for (int idx = lo; octOk && idx < hi; ++idx)
            if (oct[idx] != W - 2*oc) octOk = false;
        // Диагональ: buf[otop+i] = (W-2c) − 2·(p2/p3)·y, y=p3−i.
        const double slope2 = 2.0 * (double(p2)/double(p3));
        auto rampVal = [&](int i){ int y = p3 - i;
            double d = double(W - 2*oc) - slope2*double(y);
            return d < 0 ? 0 : int(d); };
        if (octOk && (oct[otop] != rampVal(0) || oct[otop + p3 - 1] != rampVal(p3-1)))
            octOk = false;

        // --- Стрим в PL (реф. регион 0xa18c8000, 1080 слов) ---
        KPlControl pl;
        pl.EnableTrace(true);
        pl.ClearTrace();
        pl.SetCornerCutWay(0, rb, rc);
        const auto &tc = pl.Trace();
        bool streamOk = tc.size() == N &&
                        tc.first().first == 0xa18c8000 &&
                        tc.last().first == 0xa18c8000 + (unsigned long)(N-1)*4 &&   // 0xa18c90dc
                        tc.first().second == (unsigned)round[0] &&
                        tc[top].second == (unsigned)round[top];

        qInfo() << "round size=" << round.size() << "top=" << top << "rows=" << rows
                << "arc[top]=" << (round.isEmpty()?-1:round[top]) << (roundOk?"OK":"MISMATCH");
        qInfo() << "octagon flat=" << (oct.isEmpty()?-1:oct[lo]) << "(exp" << W-2*oc << ")"
                << "ramp[top]=" << (oct.isEmpty()?-1:oct[otop]) << (octOk?"OK":"MISMATCH");
        qInfo() << "stream writes=" << tc.size() << "(exp" << N << ") last=0x"
                << QString::number(tc.isEmpty()?0:tc.last().first,16) << (streamOk?"OK":"MISMATCH");
        const bool ok = roundOk && octOk && streamOk;
        qInfo() << (ok ? "cornercut: PASS" : "cornercut: FAIL");
        return ok ? 0 : 40;
    }

    // Self-test файлового слоя (копирование/удаление каталогов, размер, тип устройства).
    if (screen == "filebackup") {
        const QString base = "/tmp/endo_fb";
        KFileBackup fb;
        fb.removeDirWithContent(base);   // очистка от прошлых прогонов
        // Создать дерево: base/exam/{1.jpeg, sub/2.jpeg}.
        QDir().mkpath(base + "/exam/sub");
        { QFile f(base + "/exam/1.jpeg"); f.open(QIODevice::WriteOnly); f.write(QByteArray(100, 'a')); }
        { QFile f(base + "/exam/sub/2.jpeg"); f.open(QIODevice::WriteOnly); f.write(QByteArray(50, 'b')); }

        const qint64 srcSize = fb.getFilesSize(base + "/exam");
        // Копировать рекурсивно.
        const bool copyOk = fb.copyDirectoryFiles(base + "/exam", base + "/backup", true);
        const bool f1 = QFile::exists(base + "/backup/1.jpeg");
        const bool f2 = QFile::exists(base + "/backup/sub/2.jpeg");
        const qint64 dstSize = fb.getFilesSize(base + "/backup");
        const qint64 freeSpace = fb.getDiskFreeSpace(base);
        const auto devInt = fb.GetDeviceTypeFromTargetPath("/home/root/data");
        const auto devUsb = fb.GetDeviceTypeFromTargetPath("/media/usb0/export");
        // Удалить копию.
        const bool delOk = fb.removeDirWithContent(base + "/backup");
        const bool gone = !QDir(base + "/backup").exists();

        qInfo() << "размер src:" << srcSize << "dst:" << dstSize << "свободно:" << freeSpace;
        qInfo() << "copy:" << copyOk << "f1/f2:" << f1 << f2 << "del:" << delOk << "gone:" << gone;
        qInfo() << "устройство internal/usb:" << devInt << devUsb;

        const bool ok = srcSize == 150 && copyOk && f1 && f2 && dstSize == 150 &&
                        freeSpace > 0 && delOk && gone &&
                        devInt == KFileBackup::DevInternal && devUsb == KFileBackup::DevUsb;
        fb.removeDirWithContent(base);
        qInfo() << (ok ? "filebackup: PASS" : "filebackup: FAIL");
        return ok ? 0 : 26;
    }

    // Self-test полного CRUD осмотров (tb_ExamList + пагинация).
    if (screen == "exam") {
        const QString dbPath = "/tmp/endo_exam.db";
        QFile::remove(dbPath);
        KEntityManage &em = KEntityManage::Instance();
        if (!em.OpenDb(dbPath)) { qWarning() << "OpenDb failed"; return 24; }
        KEntityExam ex;   // соединение endo_main
        ex.CreateTable();

        // Три осмотра одного пациента (разные даты).
        for (int i = 1; i <= 3; ++i) {
            ExamListEntity e;
            e.examId = QString("E%1").arg(i, 3, 10, QChar('0'));
            e.accessionNumber = QString("ACC%1").arg(i);
            e.patientId = "P001"; e.examType = "Gastroscopy";
            e.examDate = QString("2026-07-%1").arg(10 + i);
            e.examTime = "12:00:00"; e.examStatus = i;
            e.examDir = QString("/data/exam/E%1").arg(i, 3, 10, QChar('0'));
            ex.CreateEntity(e);
        }
        // Осмотр другого пациента.
        { ExamListEntity e; e.examId = "E999"; e.patientId = "P002";
          e.examDate = "2026-07-01"; ex.CreateEntity(e); }

        const int total = ex.GetEntityNumber();
        const auto forP001 = ex.GetEntityDetailList("P001");
        const QString latest = ex.GetLatestExamId();     // E999? нет: E003 (2026-07-13 позже 07-01)
        const auto page = ex.GetPageRecord(0, 2);         // 2 самых свежих
        ExamListEntity got; const bool gotOk = ex.GetEntityDetail("E002", got);
        // Обновление статуса.
        got.examStatus = 9; ex.UpdateEntity(got);
        ExamListEntity re; ex.GetEntityDetail("E002", re);
        // Удаление.
        ex.DeleteSelf("E999");

        qInfo() << "всего осмотров:" << total << "у P001:" << forP001.size()
                << "последний:" << latest << "страница(0,2):" << page.size();
        qInfo() << "E002 статус после update:" << re.examStatus << "тип:" << got.examType;

        const bool ok = total == 4 && forP001.size() == 3 &&
                        latest == "E003" &&                  // самый свежий по дате
                        page.size() == 2 && page.first().examId == "E003" &&
                        gotOk && got.accessionNumber == "ACC2" && re.examStatus == 9 &&
                        ex.GetEntityNumber() == 3;            // после удаления E999
        em.CloseDb();
        qInfo() << (ok ? "exam: PASS" : "exam: FAIL");
        return ok ? 0 : 25;
    }

    // Self-test конфига колонок списка осмотров (видимость + экспорт, персист).
    if (screen == "examcfg") {
        const QString ini = "/tmp/endo_examcfg.ini";
        QFile::remove(ini);
        KExamListConfigHandler &h = KExamListConfigHandler::GetInstance();
        h.SetConfigFile(ini);
        // Дефолты (из реверса: Age/Applicant/EndoScope/ExamDate видимы; Birthday/SN скрыты).
        const bool defOk = h.IsShowAge() && h.IsShowApplicant() && h.IsShowExamDate() &&
                           !h.IsShowBirthday() && !h.IsShowEndoScopeSN();
        // Изменить + сохранить + перечитать (персист).
        h.SetIsShowBirthday(true);
        h.SetIsShowExamDate(false);
        h.SetIsShowTelephone(true);
        h.SetExportType(2);
        h.SetExportPath("/data/export");
        h.SaveConfig();
        // Проверить персист через свежий QSettings-доступ.
        const bool persist = h.IsShowBirthday() && !h.IsShowExamDate() &&
                             h.IsShowTelephone() && h.GetExportType() == 2 &&
                             h.GetExportPath() == "/data/export" &&
                             h.GetColumnIsShow("Birthday");
        qInfo() << "дефолты ок:" << defOk << "| персист ок:" << persist
                << "| exportType:" << h.GetExportType() << "path:" << h.GetExportPath();
        const bool ok = defOk && persist;
        qInfo() << (ok ? "examcfg: PASS" : "examcfg: FAIL");
        return ok ? 0 : 23;
    }

    // Self-test словарей автозаполнения (tb_QuickInput*): частота + предложения.
    if (screen == "quickinput") {
        const QString dbPath = outFile.isEmpty() ? "/tmp/endo_qi.db" : outFile;
        QFile::remove(dbPath);
        KEntityManage &em = KEntityManage::Instance();
        if (!em.OpenDb(dbPath)) { qWarning() << "OpenDb failed"; return 19; }

        KEntityQuickInput qi(KEntityQuickInput::Doctor);   // соединение endo_main
        qi.CreateTable();
        // Ввод: "Dr. Smith" ×3, "Dr. Jones" ×1 → Smith выше по частоте.
        qi.SaveData("Dr. Smith"); qi.SaveData("Dr. Smith"); qi.SaveData("Dr. Smith");
        qi.SaveData("Dr. Jones");
        qi.SaveData("Dr. Adams");
        const auto all = qi.GetAllEntity();
        qInfo() << "врачей в словаре:" << qi.GetEntityNumber();
        for (const auto &e : all) qInfo() << "  " << e.value << "count=" << e.count;
        // Автодополнение по префиксу "Dr. J".
        const auto byPref = qi.GetEntity("Dr. J");
        qInfo() << "по префиксу 'Dr. J':" << byPref.size();
        // Удаление.
        qi.DeleteSelf("Dr. Adams");

        // Разные словари независимы (пациент).
        KEntityQuickInput qp(KEntityQuickInput::Patient);
        qp.CreateTable();
        qp.SaveData("Ivanov");

        const bool ok =
            qi.GetEntityNumber() == 2 &&                       // после удаления Adams
            !all.isEmpty() && all.first().value == "Dr. Smith" && // ранжирование по частоте
            all.first().count == 3 &&
            byPref.size() == 1 && byPref.first().value == "Dr. Jones" &&
            qp.GetEntityNumber() == 1 &&                       // независимая таблица
            KEntityQuickInput::TableName(KEntityQuickInput::Applicant) == "tb_QuickInputApplicant";
        em.CloseDb();
        qInfo() << (ok ? "quickinput: PASS" : "quickinput: FAIL");
        return ok ? 0 : 20;
    }

    // Self-test центрального статуса (KSystemStatus): состояние + сигналы изменений.
    if (screen == "sysstatus") {
        KSystemStatus &ss = KSystemStatus::GetInstance();
        // Собрать все оповещения (тип,значение).
        QVector<QPair<int,int>> events;
        QObject::connect(&ss, &KSystemStatus::SystemStatusChange,
                         [&](int t, int v){ events.append({t, v}); });

        ss.SetFreezeStatus(1);
        ss.SetVlsMode(3);              // VIST (offset 0x3c)
        ss.SetRecordStatus(1);
        ss.SetVlsMode(3);             // без изменения → сигнал НЕ шлём
        ss.SetIrisValue(42);
        ss.SetImageBrightness(80);

        qInfo() << "оповещений:" << events.size();
        for (auto &e : events) qInfo() << "  type=" << e.first << "value=" << e.second;
        qInfo() << "VlsMode:" << ss.VlsMode() << "Freeze:" << ss.FreezeStatus()
                << "Iris:" << ss.IrisValue() << "Brightness:" << ss.ImageBrightness();

        const bool ok =
            ss.FreezeStatus() == 1 && ss.VlsMode() == 3 && ss.RecordStatus() == 1 &&
            ss.IrisValue() == 42 && ss.ImageBrightness() == 80 &&
            events.size() == 5 &&                        // 6 сеттеров, 1 без изменения
            events[1] == qMakePair((int)KSystemStatus::ST_VlsMode, 3) &&
            events[0] == qMakePair((int)KSystemStatus::ST_Freeze, 1);
        qInfo() << (ok ? "sysstatus: PASS" : "sysstatus: FAIL");
        return ok ? 0 : 18;
    }

    // Self-test спеки событий статистики (statistic.ini).
    if (screen == "statistic") {
        KStatisticConfig sc;
        const bool loaded = sc.Load();
        const auto times = sc.EventsOfKind(KStatisticConfig::Time);
        const auto counts = sc.EventsOfKind(KStatisticConfig::Count);
        const auto infos = sc.EventsOfKind(KStatisticConfig::Info);
        qInfo() << "событий всего:" << sc.Events().size()
                << "time:" << times.size() << "count:" << counts.size() << "info:" << infos.size();
        qInfo() << "time_power_on паттерн:" << sc.PatternFor("time_power_on");
        qInfo() << "time_examine_start:" << sc.PatternFor("time_examine_start");
        // dcnt со значением-с-запятой (Fram Lost,Index) — должно собраться обратно.
        QString frameLost;
        for (const auto &e : counts) if (e.section == "Video") frameLost = e.pattern;
        qInfo() << "frameLost паттерн:" << frameLost;

        const bool ok = loaded && sc.Events().size() >= 12 &&
                        times.size() >= 8 && counts.size() == 1 && infos.size() == 2 &&
                        sc.PatternFor("time_power_on") == "Power on" &&
                        sc.PatternFor("time_examine_start") == "Exam: start examination" &&
                        frameLost == "Fram Lost,Index";
        qInfo() << (ok ? "statistic: PASS" : "statistic: FAIL");
        return ok ? 0 : 17;
    }

    // Self-test конфига мультиязычности (mutilanguageinfo.ini + languageConfig).
    if (screen == "language") {
        languageConfig &lc = languageConfig::getInstance();
        lc.Init();  // system/platform/mutilanguageinfo.ini
        qInfo() << "languageType:" << lc.getLanguageType()
                << "currentLanguage:" << lc.getCurrentLanguage();
        qInfo() << "googlePath:" << lc.getGooglePath();
        qInfo() << "puctPath:" << lc.getPuctPath();

        // Референсный ini: LanguageType=1(Chinese), CurrentLanguage=1, пути в platform.
        const bool initOk =
            lc.getLanguageType() == KLangChinese &&
            lc.getCurrentLanguage() == KLangChinese &&
            lc.getGooglePath().endsWith("platform") &&
            lc.getPuctPath().endsWith("kchinesePunct.tab");

        // setLanguageType пишет в ОБА поля.
        lc.setLanguageType(KLangRussian);
        const bool setTypeOk =
            lc.getLanguageType() == KLangRussian &&
            lc.getCurrentLanguage() == KLangRussian;

        // setCurrentLanguage: t==Chinese(1) всегда проходит; иначе только если ==languageType.
        lc.setCurrentLanguage(KLangChinese);            // 1 → проходит
        const bool sc1 = lc.getCurrentLanguage() == KLangChinese;
        lc.setCurrentLanguage(KLangEnglish);            // ≠1 и ≠languageType(Russian) → no-op
        const bool sc2 = lc.getCurrentLanguage() == KLangChinese;   // не изменилось
        lc.setCurrentLanguage(KLangRussian);            // ==languageType → проходит
        const bool sc3 = lc.getCurrentLanguage() == KLangRussian;
        const bool setCurOk = sc1 && sc2 && sc3;

        // Сигнал CurrentLanChange существует и коннектится (эмиссию ref из сеттеров не делает).
        bool sigConnectable = static_cast<bool>(
            QObject::connect(&lc, &languageConfig::CurrentLanChange, [](){}));

        qInfo() << "init" << initOk << "setType" << setTypeOk
                << "setCur" << setCurOk << "sig" << sigConnectable;
        const bool ok = initOk && setTypeOk && setCurOk && sigConnectable;
        qInfo() << (ok ? "language: PASS" : "language: FAIL");
        return ok ? 0 : 24;
    }

    // Self-test карты экранной клавиатуры (multi_language_unicode_2_text.xml).
    if (screen == "unicodetext") {
        KLoadUnicodeText &ut = KLoadUnicodeText::GetInstance();
        const bool built = ut.BuildUnic2TextLayoutLibFromXml();
        int langCount = ut.Lib().isEmpty() ? 0 : ut.Lib().first().maps.size();
        qInfo() << "раскладок:" << ut.Lib().size() << "языков:" << langCount;

        auto find = [&](const QString &key, const QString &lang) {
            return ut.FindTextFromUnic2TextLayoutLib("PAD", "1.1", key, lang);
        };
        // Русская раскладка: кириллица, обычные символы, name2value (&, <).
        const QString io   = find("SONO_Cyrillic_io", "Russian");     // ё
        const QString excl = find("SONO_exclam", "Russian");          // !
        const QString amp  = find("SONO_ampersand", "Russian");       // & (из value=)
        const QString less = find("SONO_less", "Russian");            // < (из value=)
        const QString miss = find("SONO_NOPE", "Russian");            // нет → пусто
        const QString wrong= find("SONO_exclam", "Klingon");          // нет языка → пусто
        qInfo() << "io=" << io << "excl=" << excl << "amp=" << amp
                << "less=" << less << "miss=[" << miss << "]";

        const bool ok = built &&
            ut.Lib().size() == 1 && langCount == 5 &&
            io == QString::fromUtf8("ё") &&
            excl == "!" &&
            amp == "&" &&          // name2value → value-атрибут
            less == "<" &&         // name2value → value-атрибут
            miss.isEmpty() &&
            wrong.isEmpty();
        qInfo() << (ok ? "unicodetext: PASS" : "unicodetext: FAIL");
        return ok ? 0 : 25;
    }

    // Self-test совместимости эндоскопов/камер (matchedScope.ini + KEncStyle).
    if (screen == "encstyle") {
        KEncStyle es;
        // Путь напрямую (X-2600/SonoScape) — не зависит от текущего бренда.
        const QString sysd = KSystem::SystemPath();
        const QString ini = QDir(sysd).absoluteFilePath(
            "style/X-2600/SonoScape/scope/matchedScope.ini");
        const bool loaded = es.Load(ini);
        es.SetProductModel("X-2600");

        const QStringList scopes = es.getSupportedScopeList();
        const QStringList cams = es.GetSupprotedCameraList();
        qInfo() << "scope-моделей:" << scopes.size() << "камер:" << cams.size();
        qInfo() << "первые scope:" << scopes.mid(0, 3) << "камеры:" << cams;

        // Фолбэк на [Default] для неизвестной модели (списки те же в этом файле).
        es.SetProductModel("X-9999");
        const bool fbOk = es.getSupportedScopeList().size() == scopes.size();
        es.SetProductModel("X-2600");

        const bool ok = loaded &&
            scopes.size() == 30 && cams.size() == 4 &&
            es.IsScopeValid("EG-X20") &&               // есть в списке
            es.IsScopeValid("ED-5GT") &&               // есть
            !es.IsScopeValid("EG-999") &&              // нет
            es.IsCameraValid("10-110-201") &&          // есть
            es.IsCameraValid("10-100-201") &&          // есть
            !es.IsCameraValid("99-999-999") &&         // нет
            fbOk;                                       // фолбэк [Default]
        qInfo() << "scopeValid EG-X20:" << es.IsScopeValid("EG-X20")
                << "camValid 10-110-201:" << es.IsCameraValid("10-110-201")
                << "fallback:" << fbOk;
        qInfo() << (ok ? "encstyle: PASS" : "encstyle: FAIL");
        return ok ? 0 : 26;
    }

    // Self-test пересчёта файлов записи осмотра (KExamListRecordFileUpdate).
    if (screen == "recfiles") {
        // Временный каталог осмотра с известным составом файлов.
        QTemporaryDir tmp;
        const QString dir = tmp.path();
        auto touch = [&](const QString &name) {
            QFile f(QDir(dir).absoluteFilePath(name));
            f.open(QIODevice::WriteOnly); f.write("x"); f.close();
        };
        touch("1.jpg"); touch("2.jpg"); touch("3.jpg");   // 3 снимка jpg
        touch("4.bmp");                                    // 1 снимок bmp
        touch("a.mp4"); touch("b.mp4");                    // 2 видео mp4
        touch("c.avi");                                    // 1 видео avi
        touch("notes.txt");                                // не считается

        const int jpg   = KExamListRecordFileUpdate::GetFiletypeNumFromPath(dir, {"*.jpg"});
        const int imgs  = KExamListRecordFileUpdate::ImageFileNum(dir);   // jpg+bmp+png+jpeg
        const int vids  = KExamListRecordFileUpdate::VideoFileNum(dir);   // mp4+avi
        const int none  = KExamListRecordFileUpdate::GetFiletypeNumFromPath(
            "/no/such/path/xyz", {"*.jpg"});               // несуществующий → 0
        qInfo() << "jpg:" << jpg << "images:" << imgs << "videos:" << vids << "missing:" << none;

        const bool ok = jpg == 3 && imgs == 4 && vids == 3 && none == 0;
        qInfo() << (ok ? "recfiles: PASS" : "recfiles: FAIL");
        return ok ? 0 : 27;
    }

    // Self-test модели блока-картинки отчёта (KImageBlock — сиблинг KTextBlock).
    if (screen == "imageblock") {
        KImageBlock::ClearPicMap();
        KReportTemplateDataNew data;

        KReportTemplateItem item;
        item.m_strID = "/RT_HOSPITAL_LOGO"; item.m_strName = "RT_HOSPITAL_LOGO";
        item.m_strTitle = "TR_HLog"; item.m_strType = "RT_IMAGE_BLOCK";
        item.m_strDataSrc = "RT_DATASOURCE_PERIPHERAL,RT_HOSPITAL_LOGO";   // <source>,<key>
        data.m_lstItems.push_back(item);

        KReportTemplateItemConfig cfg;
        cfg.m_strName = "/RT_HOSPITAL_LOGO";
        cfg.m_mapAttrs["ImageWidth"]  = "120";
        cfg.m_mapAttrs["ImageHeight"] = "80";
        cfg.m_mapAttrs["AlignH"]      = "Center";
        data.m_mapItemConfigs["/RT_HOSPITAL_LOGO"] = cfg;

        KImageBlock block(&data.m_lstItems.front(), &data);

        // Основные геттеры.
        const bool idOk = block.ElementId() == "/RT_HOSPITAL_LOGO";
        const bool nameOk = block.ImageName() == "RT_HOSPITAL_LOGO";   // tr(Name), без перевода = ключ
        const bool sizeOk = block.Width() == 120 && block.Heigth() == 80;
        const bool alignOk = block.GetAlign() == "Center";

        // Url: DataSrc → SplitStr(",") → last токен = "RT_HOSPITAL_LOGO" → PicMap[key].
        // Регистрируем реальный существующий файл прошивки → valid=true.
        const QString realImg = QDir(KSystem::SystemPath())
            .absoluteFilePath("style/X-2600/PyCkeun/scope/genc.ini");   // существует, но не картинка
        // Возьмём реальную картинку из ассетов, если есть; иначе проверим только путь.
        KImageBlock::RegisterPicPath("RT_HOSPITAL_LOGO", "/data/pic/logo.png");
        bool valid = true;
        const QString url = block.Url(valid);
        // Путь возвращается всегда; несуществующий файл → valid=false.
        const bool urlOk = url == "/data/pic/logo.png" && !valid;

        // Незарегистрированный ключ → operator[] вставляет пустой путь, valid=false.
        KReportTemplateItem item2;
        item2.m_strID = "/RT_OTHER"; item2.m_strName = "RT_OTHER";
        item2.m_strDataSrc = "SRC,UNKNOWN_KEY";
        data.m_lstItems.push_back(item2);
        KImageBlock block2(&data.m_lstItems.back(), &data);
        bool v2 = true;
        const QString url2 = block2.Url(v2);
        const bool url2Ok = url2.isEmpty() && !v2;

        // Промах item-config: нет записи → Width/Heigth = -1, GetAlign "".
        KReportTemplateItem orphan; orphan.m_strID = "/NONE"; orphan.m_strName = "N";
        KImageBlock empty(&orphan, &data);
        const bool orphanOk = empty.Width() == -1 && empty.Heigth() == -1
            && empty.GetAlign().empty();

        (void)realImg;
        qInfo() << "id:" << idOk << "имя:" << nameOk << "размер:" << sizeOk
                << "(" << block.Width() << block.Heigth() << ")" << "выравн:" << alignOk;
        qInfo() << "url:" << urlOk << url << "valid:" << valid << "| незарег:" << url2Ok
                << "| промах:" << orphanOk;

        const bool ok = idOk && nameOk && sizeOk && alignOk && urlOk && url2Ok && orphanOk;
        qInfo() << (ok ? "imageblock: PASS" : "imageblock: FAIL");
        return ok ? 0 : 48;
    }

    // Self-test модели табличного блока отчёта (KTableBlock — полиморфный сиблинг
    // KTextBlock/KImageBlock: сетка + бордюр/отступы + маппинг ячейка→под-элемент).
    if (screen == "tableblock") {
        KReportTemplateDataNew data;

        // Блок-таблица: Column="3", ShowTitle="1", тип-алиас → RT_TITLE_TABLE_BLOCK.
        KReportTemplateItem item;
        item.m_strID = "/RT_TBL"; item.m_strName = "RT_TBL";
        item.m_strType = "RT_SUB_DATA_BLOCK";   // алиас → TableType() = RT_TITLE_TABLE_BLOCK
        item.m_strColumn = "3"; item.m_strShowTitle = "1";
        for (int i = 0; i < 7; ++i) {           // 7 детей → ceil(7/3)=3 строки
            KReportTemplateItem child;
            child.m_strID = "c" + std::to_string(i); child.m_strName = child.m_strID;
            item.m_lstSubItems.push_back(child);
        }
        data.m_lstItems.push_back(item);

        KReportTemplateItemConfig cfg;
        cfg.m_strName = "/RT_TBL";
        cfg.m_mapAttrs["BorderWidth"] = "2.5";
        cfg.m_mapAttrs["BorderColor"] = "red";
        cfg.m_mapAttrs["MarginWidth"] = "10,20";   // → 10*0.62, 20*0.62
        cfg.m_mapAttrs["ColumnRatio"] = "25,50,25";
        data.m_mapItemConfigs["/RT_TBL"] = cfg;

        KTableBlock block(&data.m_lstItems.front(), &data);

        // Сетка: 7 детей / 3 столбца → QSize(width=3 строки, height=3 столбца).
        const bool sizeOk = block.Size() == QSize(3, 3);
        const bool idOk = block.ElementId() == "/RT_TBL";
        const bool showTitleOk = block.ShowTitle();                       // "1"
        const bool typeOk = block.TableType() == "RT_TITLE_TABLE_BLOCK";  // ремап алиаса
        const bool titleBlkOk = block.TitleTextBlock().ElementId() == "/RT_TBL";

        // Ячейка→под-элемент: index = row*cols(3)+col.
        KReportTemplateItem *cell = nullptr;
        const bool c00 = block.GetTemplateItemForCell(0, 0, cell) && cell->m_strID == "c0";
        cell = nullptr;
        const bool c20 = block.GetTemplateItemForCell(2, 0, cell) && cell->m_strID == "c6"; // index6
        cell = nullptr;
        // index=7 при 7 детях (0..6) → вне списка → false, cell не тронут.
        const bool cOob = !block.GetTemplateItemForCell(2, 1, cell) && cell == nullptr;      // index7
        const bool cNeg = !block.GetTemplateItemForCell(-1, 0, cell);
        const bool cellOk = c00 && c20 && cOob && cNeg;

        // Бордюр/цвет/отступы/пропорции столбцов.
        const bool bwOk = block.BorderWidth() == 2.5f && block.ShowBorder();
        const bool colorOk = block.BorderColor() == "red";
        const std::vector<float> m = block.Margin();
        const bool marginOk = m.size() == 2
            && qFuzzyCompare(m[0], 10.0f * 0.62f) && qFuzzyCompare(m[1], 20.0f * 0.62f);
        const QVector<QTextLength> cw = block.ColWidthContraints();
        const bool cwOk = cw.size() == 3
            && cw[0].rawValue() == 25 && cw[1].rawValue() == 50 && cw[2].rawValue() == 25
            && cw[0].type() == QTextLength::PercentageLength;

        // Ref-override: Column="2", но RefColumnID→конфиг с RefColumn="4" → 4 столбца.
        KReportTemplateItem item2;
        item2.m_strID = "/RT_TBL2"; item2.m_strName = "RT_TBL2";
        item2.m_strType = "RT_TABLE_BLOCK";   // не алиас → TableType без ремапа
        item2.m_strColumn = "2";
        for (int i = 0; i < 8; ++i) {
            KReportTemplateItem child; child.m_strID = "d" + std::to_string(i);
            item2.m_lstSubItems.push_back(child);
        }
        data.m_lstItems.push_back(item2);
        KReportTemplateItemConfig cfg2; cfg2.m_strName = "/RT_TBL2";
        cfg2.m_mapAttrs["RefColumnID"] = "REFCFG";
        data.m_mapItemConfigs["/RT_TBL2"] = cfg2;
        KReportTemplateItemConfig refcfg; refcfg.m_strName = "REFCFG";
        refcfg.m_mapAttrs["RefColumn"] = "4";
        data.m_mapItemConfigs["REFCFG"] = refcfg;
        KTableBlock block2(&data.m_lstItems.back(), &data);
        const bool refOk = block2.Size() == QSize(2, 4)               // 8/4 → 2 строки, 4 столбца
            && block2.TableType() == "RT_TABLE_BLOCK";

        // Промах item-config + нет Column: attrs пусты → дефолты, Size=(детей,1).
        KReportTemplateItem orphan; orphan.m_strID = "/NONE"; orphan.m_strName = "N";
        KTableBlock empty(&orphan, &data);
        const bool orphanOk = empty.Size() == QSize(0, 1)
            && empty.BorderWidth() == 0.0f && !empty.ShowBorder()
            && empty.BorderColor() == "black" && empty.Margin().empty()
            && empty.ColWidthContraints().isEmpty() && !empty.ShowTitle();

        qInfo() << "size:" << sizeOk << block.Size() << "id:" << idOk << "title:" << showTitleOk
                << "type:" << typeOk << titleBlkOk;
        qInfo() << "ячейки:" << cellOk << "| бордюр:" << bwOk << "цвет:" << colorOk
                << "отступ:" << marginOk << "столбцы:" << cwOk;
        qInfo() << "ref-override:" << refOk << block2.Size() << "| промах:" << orphanOk;

        const bool ok = sizeOk && idOk && showTitleOk && typeOk && titleBlkOk && cellOk
            && bwOk && colorOk && marginOk && cwOk && refOk && orphanOk;
        qInfo() << (ok ? "tableblock: PASS" : "tableblock: FAIL");
        return ok ? 0 : 49;
    }

    // Self-test модели таблицы-с-заголовком (KTitleTableBlock — наследник KTableBlock:
    // +1 строка под заголовок, ячейки сдвинуты, ShowTitle форсирован, Title/SetTitle).
    if (screen == "titletableblock") {
        KReportTemplateDataNew data;

        KReportTemplateItem item;
        item.m_strID = "/RT_TTBL"; item.m_strName = "RT_TTBL";
        item.m_strType = "RT_TABLE_BLOCK"; item.m_strTitle = "Report Title";
        item.m_strColumn = "3"; item.m_strShowTitle = "0";   // 0 — но ShowTitle() форсит true
        for (int i = 0; i < 7; ++i) {                        // база: ceil(7/3)=3 строки
            KReportTemplateItem child;
            child.m_strID = "c" + std::to_string(i); child.m_strName = child.m_strID;
            item.m_lstSubItems.push_back(child);
        }
        data.m_lstItems.push_back(item);

        KTitleTableBlock block(&data.m_lstItems.front(), &data);

        // Размер: база (3,3) + строка заголовка → (width=4, height=3).
        const bool sizeOk = block.Size() == QSize(4, 3);
        // ShowTitle форсирован в true, несмотря на m_strShowTitle=="0".
        const bool showOk = block.ShowTitle();
        // Наследование от KTableBlock: базовые аксессоры работают.
        const bool idOk = block.ElementId() == "/RT_TTBL"
            && block.TableType() == "RT_TABLE_BLOCK";

        // Ячейки сдвинуты: row 0 — заголовок (нет данных), данные с row 1.
        KReportTemplateItem *cell = nullptr;
        const bool titleRow = !block.GetTemplateItemForCell(0, 0, cell) && cell == nullptr; // row-1=-1
        const bool r1c0 = block.GetTemplateItemForCell(1, 0, cell) && cell->m_strID == "c0"; // база(0,0)
        cell = nullptr;
        const bool r3c0 = block.GetTemplateItemForCell(3, 0, cell) && cell->m_strID == "c6"; // база(2,0)=index6
        cell = nullptr;
        const bool oob = !block.GetTemplateItemForCell(3, 1, cell);   // база(2,1)=index7 → нет
        const bool cellOk = titleRow && r1c0 && r3c0 && oob;

        // Title(): читает item->m_strTitle (fromLatin1).
        const bool titleReadOk = block.Title() == "Report Title";
        // SetTitle(): пишет UTF-8 в узел; ASCII → round-trip корректен.
        block.SetTitle("New Title");
        const bool titleSetOk = block.Title() == "New Title"
            && data.m_lstItems.front().m_strTitle == "New Title";

        qInfo() << "size:" << sizeOk << block.Size() << "show:" << showOk << "id/type:" << idOk;
        qInfo() << "ячейки:" << cellOk << "| title-read:" << titleReadOk
                << "title-set:" << titleSetOk;

        const bool ok = sizeOk && showOk && idOk && cellOk && titleReadOk && titleSetOk;
        qInfo() << (ok ? "titletableblock: PASS" : "titletableblock: FAIL");
        return ok ? 0 : 50;
    }

    // Self-test свободных функций report_template (сериализация map ⇄ строка, source-id,
    // генерация ID, предикат bold, резолв заголовка).
    if (screen == "reporttmpl") {
        using namespace report_template;

        // ConvertMapToString: сорт по ключу, "%s|%s;", пустой ключ пропущен.
        std::map<std::string, std::string> m;
        m["b"] = "2"; m["a"] = "1"; m[""] = "skip";   // "" — пропускается
        const std::string s = ConvertMapToString(m);
        const bool m2sOk = s == "a|1;b|2;";            // сортировка + завершающий ';'

        // ConvertStringToMap: инверс; РОВНО 2 токена, непустое значение.
        std::map<std::string, std::string> back;
        ConvertStringToMap(s, back);
        const bool s2mOk = back.size() == 2 && back["a"] == "1" && back["b"] == "2";
        // Мусор отбрасывается: пустое значение ("x|;"), 3 токена ("p|q|r").
        std::map<std::string, std::string> bad;
        ConvertStringToMap("x|;p|q|r;ok|v;", bad);
        const bool badOk = bad.size() == 1 && bad.count("ok") && bad["ok"] == "v";
        // out не очищается (merge).
        std::map<std::string, std::string> merge; merge["keep"] = "0";
        ConvertStringToMap("a|1;", merge);
        const bool mergeOk = merge.size() == 2 && merge["keep"] == "0" && merge["a"] == "1";

        // ConvertToSourceID: out = src + "," + ConvertMapToString(param).
        std::string sid; const bool sidRet = ConvertToSourceID("SRC", m, sid);
        const bool sidOk = sidRet && sid == "SRC,a|1;b|2;";
        std::string sidEmpty; ConvertToSourceID("SRC", {}, sidEmpty);
        const bool sidEmptyOk = sidEmpty == "SRC,";   // завершающая ',' всегда

        // GenerateIDByString: a/sep/b; пустой конец → без sep.
        const bool genOk = GenerateIDByString("/RT_A", "B", "/") == "/RT_A/B"
            && GenerateIDByString("", "B", "/") == "B"
            && GenerateIDByString("A", "", "/") == "A";

        // IsPatientInfoTitleBold: substring m_strID.
        KReportTemplateItem pi; pi.m_strID = "/RT_HEADER/RT_PATIENT_INFO/x";
        KReportTemplateItem ho; ho.m_strID = "/HOSPITAL_OTHER_LOGO";
        KReportTemplateItem no; no.m_strID = "/RT_DIAGNOSIS";
        const bool boldOk = IsPatientInfoTitleBold(pi) && IsPatientInfoTitleBold(ho)
            && !IsPatientInfoTitleBold(no);

        // QueryTemplateItemRealTitle: off-device → out=m_strTitle, false.
        KReportTemplateItem ti; ti.m_strTitle = "TR_X"; ti.m_strName = "RT_RESERVED1";
        std::string rt; const bool rtRet = QueryTemplateItemRealTitle(ti, rt);
        const bool rtOk = !rtRet && rt == "TR_X";

        // Дерево для FindRefItem/HasSameNameInGroup: /A → {/A/B(TB), /A/C(TB — дубль)}.
        KReportTemplateDataNew tree;
        KReportTemplateItem A; A.m_strID = "/A"; A.m_strName = "A"; A.m_strTitle = "TA";
        KReportTemplateItem B; B.m_strID = "/A/B"; B.m_strName = "B"; B.m_strTitle = "TB";
        KReportTemplateItem C; C.m_strID = "/A/C"; C.m_strName = "C"; C.m_strTitle = "TB";
        A.m_lstSubItems.push_back(B); A.m_lstSubItems.push_back(C);
        tree.m_lstItems.push_back(A);

        const KReportTemplateItem *fb = FindConstRefItem(tree, "/A/B");
        const KReportTemplateItem *fa = FindConstRefItem(tree, "/A");
        KReportTemplateItem *fc = FindRefItem(tree, "/A/C");
        const bool findOk = fb && fb->m_strName == "B" && fa && fa->m_strName == "A"
            && fc && fc->m_strName == "C" && FindConstRefItem(tree, "/A/X") == nullptr;

        // UpdateItemID: пересчёт ID поддерева от пустого родителя.
        KReportTemplateItem U; U.m_strName = "ROOT"; U.m_strID = "stale";
        KReportTemplateItem UC; UC.m_strName = "CH"; UC.m_strID = "stale2";
        U.m_lstSubItems.push_back(UC);
        UpdateItemID(U, "");
        const bool updOk = U.m_strID == "ROOT"                       // пустой parent → без sep
            && U.m_lstSubItems.front().m_strID == "ROOT/CH";        // ребёнок от нового ID

        // GetSubData: заглушка → false.
        KReportTemplateDataNew subOut;
        const bool subDataOk = !GetSubData(tree, "k", subOut);

        // HasSameNameInGroup: сравнение с m_strTitle сиблинга, сам себя не считаем.
        const bool dupOk = HasSameNameInGroup(tree, "/A/B", "TB");   // C имеет TB → true
        const bool noDupOk = !HasSameNameInGroup(tree, "/A/B", "TZ")
            && !HasSameNameInGroup(tree, "/A", "TA");                // корень, один сиблинг=сам
        const bool groupOk = dupOk && noDupOk;

        // Мутации/сбор: дерево /P → {X → {X1}, Y}; конфиги /P/X, /P/X/1.
        KReportTemplateDataNew t2;
        KReportTemplateItem P; P.m_strID = "/P"; P.m_strName = "P";
        KReportTemplateItem X; X.m_strID = "/P/X"; X.m_strName = "X"; X.m_strTitle = "TX";
        KReportTemplateItem X1; X1.m_strID = "/P/X/1"; X1.m_strName = "X1";
        X.m_lstSubItems.push_back(X1);
        KReportTemplateItem Y; Y.m_strID = "/P/Y"; Y.m_strName = "Y";
        P.m_lstSubItems.push_back(X); P.m_lstSubItems.push_back(Y);
        t2.m_lstItems.push_back(P);
        t2.m_mapItemConfigs["/P/X"] = KReportTemplateItemConfig();
        t2.m_mapItemConfigs["/P/X/1"] = KReportTemplateItemConfig();
        KReportTemplateItem *pRef = FindRefItem(t2, "/P");

        // GetSubItems(item): flat [X,Y]; recursive pre-order [X,X1,Y]; копии поверхностные.
        std::list<KReportTemplateItem> giFlat, giRec;
        GetSubItems(*pRef, giFlat, false);
        GetSubItems(*pRef, giRec, true);
        const bool giOk = giFlat.size() == 2 && giFlat.front().m_strName == "X"
            && giRec.size() == 3 && std::next(giRec.begin())->m_strName == "X1"
            && giRec.front().m_lstSubItems.empty();          // поверхностная копия

        // GetSubItems(by-id): /P → 2; "" → корень [P]; miss → false; out не очищается.
        std::list<KReportTemplateItem> gid; KReportTemplateItem dummy; dummy.m_strName = "DUMMY";
        gid.push_back(dummy);
        GetSubItems(t2, "/P", gid, false);                   // append → DUMMY,X,Y
        std::list<KReportTemplateItem> groot, gmiss;
        GetSubItems(t2, "", groot, false);
        const bool byIdOk = gid.size() == 3 && gid.front().m_strName == "DUMMY"
            && groot.size() == 1 && groot.front().m_strName == "P"
            && !GetSubItems(t2, "/nope", gmiss, false) && gmiss.empty();

        // RemoveSubItem: матч по m_strName; чистка конфигов-потомков "/P/X/".
        const bool remOk = RemoveSubItem(t2, "/P", "X")      // X по имени → удалён
            && pRef->m_lstSubItems.size() == 1 && pRef->m_lstSubItems.front().m_strName == "Y"
            && t2.m_mapItemConfigs.count("/P/X/1") == 0      // потомок вычищен
            && t2.m_mapItemConfigs.count("/P/X") == 1;       // собственный НЕ тронут
        const bool remMissOk = !RemoveSubItem(t2, "/nope", "X")   // нет родителя
            && !RemoveSubItem(t2, "/P", "ZZZ");                    // нет ребёнка с таким именем

        // AppendSubItem: дедуп по m_strName, дословная копия (m_strID не пересчитан).
        KReportTemplateItem N; N.m_strName = "NEW"; N.m_strID = "verbatim-id";
        const bool appOk = AppendSubItem(t2, "/P", N)
            && pRef->m_lstSubItems.back().m_strName == "NEW"
            && pRef->m_lstSubItems.back().m_strID == "verbatim-id";   // не пересчитан
        KReportTemplateItem dupY; dupY.m_strName = "Y";
        const bool appDupOk = !AppendSubItem(t2, "/P", dupY)         // дубль имени → false
            && !AppendSubItem(t2, "/nope", N);                       // нет родителя → false

        // AppendSubItem(list): множество имён строится ОДИН РАЗ → внутрибатчевые дубли обе.
        std::list<KReportTemplateItem> batch;
        KReportTemplateItem d1; d1.m_strName = "DUP"; KReportTemplateItem d2; d2.m_strName = "DUP";
        KReportTemplateItem existY; existY.m_strName = "Y";
        batch.push_back(d1); batch.push_back(d2); batch.push_back(existY);
        AppendSubItem(t2, "/P", batch);
        int dupCount = 0;
        for (const KReportTemplateItem &c : pRef->m_lstSubItems)
            if (c.m_strName == "DUP") ++dupCount;
        const bool appListOk = dupCount == 2;                        // обе DUP добавлены, Y — нет

        // Customed-семейство: корневые кастомные секции.
        KReportTemplateDataNew t3;
        KReportTemplateItem S1; S1.m_strID = "/S1"; S1.m_strName = "S1"; S1.m_strTitle = "Old";
        KReportTemplateItem S2; S2.m_strID = "/S2"; S2.m_strName = "S2";
        t3.m_lstItems.push_back(S1); t3.m_lstItems.push_back(S2);
        KReportTemplateItemConfig cS1; cS1.m_bUserDefine = true;   // S1 — кастомная
        t3.m_mapItemConfigs["/S1"] = cS1;
        t3.m_mapItemConfigs["/S2"] = KReportTemplateItemConfig();  // S2 — обычная
        t3.m_mapItemConfigs["/S1/child"] = KReportTemplateItemConfig();   // потомок S1

        // GetCustomedSections: только m_bUserDefine=true, верхний уровень.
        std::vector<std::string> secs; secs.push_back("STALE");   // проверим clear
        GetCustomedSections(t3, secs);
        const bool secOk = secs.size() == 1 && secs[0] == "/S1";

        // RenameCustomedItem: пишет m_strTitle.
        const bool renOk = RenameCustomedItem(t3, "/S1", "NewTitle")
            && FindConstRefItem(t3, "/S1")->m_strTitle == "NewTitle"
            && FindConstRefItem(t3, "/S1")->m_strName == "S1"          // m_strName не тронут
            && !RenameCustomedItem(t3, "/nope", "X");                  // miss → false

        // DeleteCustomedItem: только корень, матч по m_strID, чистка потомков-конфигов.
        const bool delGuardOk = !DeleteCustomedItem(t3, "/notroot", S1);   // parentId!="" → false
        const bool delOk = DeleteCustomedItem(t3, "", S1)             // удалить /S1
            && FindConstRefItem(t3, "/S1") == nullptr
            && t3.m_mapItemConfigs.count("/S1") == 0                  // собственный конфиг…
            && t3.m_mapItemConfigs.count("/S1/child") == 0           // …и потомок вычищены
            && t3.m_mapItemConfigs.count("/S2") == 1;                // S2 не тронут

        // AppendCustomedItem: синтез KW_NEW_SECTION_1 + конфиг, мутация item.
        KReportTemplateDataNew t4;
        KReportTemplateItem fresh;
        const bool appCustGuard = !AppendCustomedItem(t4, "/x", fresh);   // не корень → false
        const bool appCustOk = AppendCustomedItem(t4, "", fresh)
            && fresh.m_strName == "KW_NEW_SECTION_1"
            && fresh.m_strID == "/KW_NEW_SECTION_1"
            && fresh.m_strType == "RT_TITLE_TABLE_BLOCK"
            && fresh.m_strShowTitle == "1" && fresh.m_strColumn == "3"
            && t4.m_lstItems.size() == 1
            && t4.m_mapItemConfigs.count("/KW_NEW_SECTION_1") == 1
            && t4.m_mapItemConfigs["/KW_NEW_SECTION_1"].m_bUserDefine
            && t4.m_mapItemConfigs["/KW_NEW_SECTION_1"].m_mapAttrs["FontType"] == "ThirdTitle";
        // Второй вызов → KW_NEW_SECTION_2 (первое имя занято).
        KReportTemplateItem fresh2;
        AppendCustomedItem(t4, "", fresh2);
        const bool appCust2Ok = fresh2.m_strName == "KW_NEW_SECTION_2" && t4.m_lstItems.size() == 2;
        const bool custOk = secOk && renOk && delGuardOk && delOk
            && appCustGuard && appCustOk && appCust2Ok;

        // AppendSubData: заглушка.
        KReportTemplateDataNew sd1, sd2;
        const bool subAppendOk = !AppendSubData(sd1, "k", sd2);

        // GetSubItemsParam: плоский substring-скан m_mapItemConfigs, out не чистится.
        KReportTemplateDataNew t5;
        t5.m_mapItemConfigs["/G"] = KReportTemplateItemConfig();
        t5.m_mapItemConfigs["/G/a"] = KReportTemplateItemConfig();
        t5.m_mapItemConfigs["/G/b"] = KReportTemplateItemConfig();
        t5.m_mapItemConfigs["/H"] = KReportTemplateItemConfig();
        std::map<std::string, KReportTemplateItemConfig> paramOut;
        paramOut["/PRE"] = KReportTemplateItemConfig();          // проверим отсутствие clear
        GetSubItemsParam(t5, "/G", paramOut);                    // /G,/G/a,/G/b (+ /PRE)
        const bool paramOk = paramOut.size() == 4 && paramOut.count("/G")
            && paramOut.count("/G/a") && paramOut.count("/G/b") && paramOut.count("/PRE")
            && !paramOut.count("/H");

        // ConvertToDetail: инверс ConvertToSourceID.
        std::string base; std::map<std::string, std::string> prm;
        const bool detOk = ConvertToDetail("SRC,a|1;b|2;", base, prm)
            && base == "SRC" && prm.size() == 2 && prm["a"] == "1" && prm["b"] == "2";
        std::string base2; std::map<std::string, std::string> prm2; prm2["keep"] = "1";
        const bool detTrailOk = ConvertToDetail("SRC,", base2, prm2)   // 1 часть → param не тронут
            && base2 == "SRC" && prm2.size() == 1 && prm2.count("keep");
        std::string base3; std::map<std::string, std::string> prm3;
        const bool detEmptyOk = !ConvertToDetail("", base3, prm3);     // пусто → false
        // Round-trip ConvertToSourceID → ConvertToDetail.
        std::string sid3; std::map<std::string, std::string> rt3 = {{"x", "9"}};
        ConvertToSourceID("BASE", rt3, sid3);
        std::string rtBase; std::map<std::string, std::string> rtPrm;
        const bool rtDetOk = ConvertToDetail(sid3, rtBase, rtPrm)
            && rtBase == "BASE" && rtPrm["x"] == "9";
        const bool convOk = detOk && detTrailOk && detEmptyOk && rtDetOk;

        // GetSplitLineInfo: gate по SplitLineWidth, дефолты, override.
        std::map<std::string, KReportTemplateItemConfig> slCfgs;
        KReportTemplateItemConfig slFull;
        slFull.m_mapAttrs["SplitLineWidth"] = "2";
        slFull.m_mapAttrs["SplitLineSpace"] = "5";
        slFull.m_mapAttrs["SplitStartIndex"] = "1";
        slFull.m_mapAttrs["SplitLineColor"] = "red";        // override дефолта black
        // SplitLineType не задан → дефолт Horizontal.
        slCfgs["/L"] = slFull;
        KReportTemplateItemConfig slGate;                   // нет SplitLineWidth → gate закрыт
        slGate.m_mapAttrs["SplitLineColor"] = "blue";
        slCfgs["/NG"] = slGate;
        KSplitLineInfo sl;
        GetSplitLineInfo("/L", slCfgs, sl);
        const bool slOk = sl.m_nSplitLineWidth == 2 && sl.m_nSplitLineSpace == 5
            && sl.m_nSplitStartIndex == 1 && sl.m_strSplitLineType == "Horizontal"
            && sl.m_strSplitLineColor == "red";
        KSplitLineInfo slNg; slNg.m_strSplitLineColor = "dirty"; slNg.m_nSplitLineWidth = 99;
        GetSplitLineInfo("/NG", slCfgs, slNg);              // gate закрыт → всё сброшено
        const bool slGateOk = slNg.m_nSplitLineWidth == 0 && slNg.m_strSplitLineColor.empty()
            && slNg.m_strSplitLineType.empty();
        KSplitLineInfo slMiss; GetSplitLineInfo("/nope", slCfgs, slMiss);
        const bool slMissOk = slMiss.m_nSplitLineWidth == 0 && slMiss.m_strSplitLineType.empty();
        const bool splitOk = slOk && slGateOk && slMissOk;

        qInfo() << "map→str:" << m2sOk << s.c_str() << "| str→map:" << s2mOk << "мусор:" << badOk
                << "merge:" << mergeOk;
        qInfo() << "sourceId:" << sidOk << sid.c_str() << "| пустой:" << sidEmptyOk
                << "| genId:" << genOk << "bold:" << boldOk << "title:" << rtOk;
        qInfo() << "find:" << findOk << "update:" << updOk << "subData:" << subDataOk
                << "group:" << groupOk;
        qInfo() << "getSub:" << giOk << byIdOk << "remove:" << remOk << remMissOk
                << "append:" << appOk << appDupOk << "list:" << appListOk;
        qInfo() << "custom:" << "sections:" << secOk << "rename:" << renOk << "delete:" << delOk
                << "append:" << appCustOk << appCust2Ok;
        qInfo() << "subAppend:" << subAppendOk << "param:" << paramOk << "detail:" << convOk
                << "splitLine:" << splitOk;

        const bool ok = m2sOk && s2mOk && badOk && mergeOk && sidOk && sidEmptyOk
            && genOk && boldOk && rtOk && findOk && updOk && subDataOk && groupOk
            && giOk && byIdOk && remOk && remMissOk && appOk && appDupOk && appListOk
            && custOk && subAppendOk && paramOk && convOk && splitOk;
        qInfo() << (ok ? "reporttmpl: PASS" : "reporttmpl: FAIL");
        return ok ? 0 : 51;
    }

    // Self-test перечислителя файлов обследования (PatientExamData — статические файловые
    // методы; device-only резолв id→каталог заменён прямым examDir).
    if (screen == "examdata") {
        QTemporaryDir tmp;
        const bool tmpOk = tmp.isValid();
        const QString qdir = tmp.path() + "/";
        const std::string dir = qdir.toUtf8().constData();
        // Раскладываем файлы: 2 картинки, 2 видео, pdf, посторонний.
        const char *files[] = {"a.jpg", "b.bmp", "m1.mp4", "m2.mkv", "report.pdf", "note.txt"};
        for (const char *f : files) {
            QFile qf(qdir + f); qf.open(QIODevice::WriteOnly); qf.write("x"); qf.close();
        }

        const bool existOk = PatientExamData::IsFileExist(dir + "a.jpg")
            && !PatientExamData::IsFileExist(dir + "zzz.jpg");

        std::vector<std::string> imgs;
        PatientExamData::GetExamDataImage(dir, imgs);
        const bool imgOk = imgs.size() == 2 && imgs[0] == dir + "a.jpg"   // сорт по baseName
            && imgs[1] == dir + "b.bmp";

        std::vector<std::string> vids;
        PatientExamData::GetExamDataVideo(dir, vids);
        const bool vidOk = vids.size() == 2 && vids[0] == dir + "m1.mp4"
            && vids[1] == dir + "m2.mkv";

        std::string pdf;
        const bool pdfOk = PatientExamData::GetExamDataPdf(dir, pdf) == 0
            && pdf == dir + "report.pdf";

        std::vector<std::string> paths;
        const bool pathOk = PatientExamData::GetExamDataPath(dir, paths) == 0
            && paths.size() == 1 && paths[0] == dir;

        std::vector<std::string> all;
        PatientExamData::GetExamDataAll(dir, all);   // 2 картинки + pdf, БЕЗ видео
        const bool allOk = all.size() == 3 && all.back() == dir + "report.pdf";

        // append: out не очищается.
        std::vector<std::string> acc; acc.push_back("PRE");
        PatientExamData::GetExamDataImage(dir, acc);
        const bool appendOk = acc.size() == 3 && acc[0] == "PRE";

        // IsExamVideoExist: есть видео здесь, нет в пустом подкаталоге.
        QDir(qdir).mkdir("empty");
        const bool vExistOk = PatientExamData::IsExamVideoExist(dir)
            && !PatientExamData::IsExamVideoExist(dir + "empty/");

        qInfo() << "tmp:" << tmpOk << "exist:" << existOk << "img:" << imgOk << "vid:" << vidOk;
        qInfo() << "pdf:" << pdfOk << "path:" << pathOk << "all:" << allOk
                << "append:" << appendOk << "vExist:" << vExistOk;

        const bool ok = tmpOk && existOk && imgOk && vidOk && pdfOk && pathOk && allOk
            && appendOk && vExistOk;
        qInfo() << (ok ? "examdata: PASS" : "examdata: FAIL");
        return ok ? 0 : 52;
    }

    // Self-test in-process шины сообщений (KObject/KMessage/KPublishManager — синхронная
    // pub/sub + адресный send + запрос родителю; PostMsg — заглушка).
    if (screen == "kobject") {
        KObjTestSink parent(1, nullptr);       // объект id=1, без родителя
        KObjTestSink child(2, &parent);        // id=2, родитель = parent
        KObjTestSink subA(3, nullptr);
        KObjTestSink subB(4, nullptr);
        KObjTestSink caller(5, nullptr);

        // Реестр: GetKObject по id, GetParentObject.
        const bool regOk = caller.GetKObject(2) == &child && caller.GetKObject(99) == nullptr
            && child.GetParentObject() == &parent;

        // Publish (msgId в (9999,13999]) → оба подписчика получают HandleSubscribeMsg.
        subA.SubscribeMsg(10001);
        subB.SubscribeMsg(10001);
        KMessage pm; pm.m_msgId = 10001;
        caller.PublishMsg(pm);
        const bool pubOk = subA.subCount == 1 && subB.subCount == 1
            && subA.lastSub == 10001 && subB.lastSub == 10001;

        // Отписка одного → второй публикации получит только subB.
        subA.UnSubscribeMsg(10001);
        KMessage pm2; pm2.m_msgId = 10001;
        caller.PublishMsg(pm2);
        const bool unsubOk = subA.subCount == 1 && subB.subCount == 2;

        // Convenience-overload PublishMsg: sender=caller, доставка subB.
        caller.PublishMsg(10001, 7, 42, "x");
        const bool pubConvOk = subB.subCount == 3;

        // SendMsg (адресно): target id=2 → child.HandleMsg, возвращает handled=true.
        const bool sendOk = caller.SendMsg(2, 100, 0, 0, 0, "") && child.msgCount == 1
            && child.lastMsg == 100;
        // SendMsg на незарегистрированный id → false, без доставки.
        const bool sendMissOk = !caller.SendMsg(99, 100, 0, 0, 0, "") && child.msgCount == 1;

        // RequestToParent: child → parent.HandleChildRequest (msgId в (19999,23999]).
        KMessage rq; rq.m_msgId = 20001;
        child.RequestToParent(rq);
        const bool reqOk = parent.reqCount == 1 && parent.lastReq == 20001;

        // PostMsg — заглушка: ничего не доставляется.
        const int beforeSub = subB.subCount, beforeMsg = child.msgCount;
        caller.PostMsg(10001, 0, 0, "");
        KMessage post; post.m_msgId = 10001;
        caller.PostMsg(post);
        const bool postOk = subB.subCount == beforeSub && child.msgCount == beforeMsg;

        // KMessage IsValid/Reset.
        KMessage empty;
        KMessage nonEmpty; nonEmpty.m_msgId = 5;
        KMessage r; r.m_msgId = 5; r.Reset();
        const bool msgOk = !empty.IsValid() && nonEmpty.IsValid()
            && !r.IsValid() && r.m_bHandled;   // Reset ставит handled=true

        qInfo() << "reg:" << regOk << "pub:" << pubOk << "unsub:" << unsubOk
                << "pubConv:" << pubConvOk;
        qInfo() << "send:" << sendOk << sendMissOk << "req:" << reqOk << "post:" << postOk
                << "msg:" << msgOk;

        const bool ok = regOk && pubOk && unsubOk && pubConvOk && sendOk && sendMissOk
            && reqOk && postOk && msgOk;
        qInfo() << (ok ? "kobject: PASS" : "kobject: FAIL");
        return ok ? 0 : 53;
    }

    // Self-test обёртки XML-документа (XmlParser — load/save/root/декларация на QDomDocument).
    if (screen == "xmlparser") {
        QTemporaryDir tmp;
        const QString qdir = tmp.path() + "/";
        auto writeFile = [&](const char *name, const QByteArray &data) {
            QFile f(qdir + name); f.open(QIODevice::WriteOnly); f.write(data); f.close();
        };
        writeFile("good.xml", "<root a=\"1\"><child>hello</child></root>");
        writeFile("bad.xml", "<root><unclosed></root>");

        // Загрузка + доступ к корню/атрибутам/тексту.
        XmlParser p;
        const bool loadOk = p.LoadFromFile((qdir + "good.xml").toStdString())
            && p.GetParseResult().empty();
        const bool rootOk = p.GetRoot().tagName() == "root"
            && p.GetRoot().attribute("a") == "1"
            && p.GetRoot().firstChildElement("child").text() == "hello";

        // Некорректный XML → false + непустое описание.
        XmlParser pb;
        const bool badOk = !pb.LoadFromFile((qdir + "bad.xml").toStdString())
            && !pb.GetParseResult().empty();
        // Отсутствующий файл → false.
        XmlParser pm;
        const bool missOk = !pm.LoadFromFile((qdir + "nope.xml").toStdString());

        // Round-trip: save → load → тот же корень.
        const bool saveOk = p.SaveToFile((qdir + "out.xml").toStdString());
        XmlParser p2;
        const bool rtOk = p2.LoadFromFile((qdir + "out.xml").toStdString())
            && p2.GetRoot().tagName() == "root"
            && p2.GetRoot().firstChildElement("child").text() == "hello";

        // Декларация: <?xml version="1.0" encoding="utf-8"?> (lowercase).
        XmlParser pd;
        pd.SetDeclaration();
        pd.SaveToFile((qdir + "decl.xml").toStdString());
        QFile df(qdir + "decl.xml"); df.open(QIODevice::ReadOnly);
        const QByteArray declOut = df.readAll(); df.close();
        const bool declOk = declOut.contains("version=\"1.0\"")
            && declOut.contains("encoding=\"utf-8\"");

        qInfo() << "load:" << loadOk << "root:" << rootOk << "bad:" << badOk << pb.GetParseResult().c_str()
                << "miss:" << missOk;
        qInfo() << "save:" << saveOk << "roundtrip:" << rtOk << "decl:" << declOk << declOut.trimmed();

        const bool ok = loadOk && rootOk && badOk && missOk && saveOk && rtOk && declOk;
        qInfo() << (ok ? "xmlparser: PASS" : "xmlparser: FAIL");
        return ok ? 0 : 55;
    }

    // Self-test низкоуровневой обёртки SQLite (KDbSqlite — ЯДРО: open/close/exec/error на
    // системном libsqlite3; SQLCipher-шифрование опущено off-device). Временная БД.
    if (screen == "dbsqlite") {
        QTemporaryDir tmp;
        const std::string db = (tmp.path() + "/test.db").toStdString();

        KDbSqlite s;
        const bool preOk = !s.IsOpen();                       // до открытия закрыта
        const int rc = s.Open(db);
        const bool openOk = rc == SQLITE_OK && s.IsOpen() && s.GetDbPath() == db;

        // DDL/DML через Exec.
        const bool ddlOk = s.Exec("CREATE TABLE t(id INTEGER, name TEXT);") == SQLITE_OK
            && s.Exec("INSERT INTO t VALUES(1,'alice');") == SQLITE_OK
            && s.Exec("INSERT INTO t VALUES(2,'bob');") == SQLITE_OK;

        // Битый SQL → ненулевой rc + непустой GetLastErrorMsg.
        const int bad = s.Exec("SELECT * FROM no_such_table;");
        const bool errOk = bad != SQLITE_OK && !s.GetLastErrorMsg().empty();

        // InsertField: "alter table t add age varchar" → колонка добавлена (вставка с ней проходит).
        const bool fieldOk = s.InsertField("t", "age") == SQLITE_OK
            && s.Exec("INSERT INTO t(id,name,age) VALUES(3,'carol','30');") == SQLITE_OK;
        // DeleteRecord: с where и без (все строки).
        const bool delOk = s.DeleteRecord("t", "id=1") == SQLITE_OK
            && s.DeleteRecord("t", "") == SQLITE_OK;

        // GetFieldNameList: колонки t = {id, name, age}.
        std::set<std::string> cols;
        const bool fnlOk = s.GetFieldNameList("t", cols) == SQLITE_OK
            && cols.count("id") && cols.count("name") && cols.count("age");
        // InsertRecord: несуществующий ключ "nocol" ОТФИЛЬТРОВАН → INSERT валиден.
        std::map<std::string, std::string> rec{{"id", "10"}, {"name", "d'ave"}, {"nocol", "zzz"}};
        const bool insRecOk = s.InsertRecord(rec, "t") == SQLITE_OK   // %Q экранирует апостроф
            && s.InsertRecord(rec, "no_such_table") != SQLITE_OK;     // нет таблицы → GetFieldNameList fail
        // UpdateRecord: SET col=%Q, с where и без; несущ. таблица → ошибка.
        const bool updOk = s.UpdateRecord({{"name", "eve"}}, "t", "id=10") == SQLITE_OK
            && s.UpdateRecord({{"name", "all"}}, "t", "") == SQLITE_OK
            && s.UpdateRecord({{"name", "x"}}, "no_such_table", "") != SQLITE_OK;
        // GetRecordsNumber: в t сейчас 1 строка (id=10). where-фильтр считает совпадения.
        const bool cntOk = s.GetRecordsNumber("t", "") == 1
            && s.GetRecordsNumber("t", "id=10") == 1
            && s.GetRecordsNumber("t", "id=999") == 0;

        // Нулевой sql → -4102 (реф. -0x1006).
        const bool nullOk = s.Exec(static_cast<const char *>(nullptr)) == -4102;

        // Exec без открытой БД → SQLITE_ERROR.
        KDbSqlite s2;
        const bool notOpenExecOk = s2.Exec("SELECT 1;") == SQLITE_ERROR;

        // Close → закрыта; повторный Close безопасен.
        const bool closeOk = s.Close() == SQLITE_OK && !s.IsOpen() && s.Close() == SQLITE_OK;

        // SetLogEnabled (статик) — без падений.
        KDbSqlite::SetLogEnabled(true);
        KDbSqlite::SetLogEnabled(false);

        qInfo() << "pre:" << preOk << "open:" << openOk << rc << "ddl:" << ddlOk
                << "err:" << errOk << "field:" << fieldOk << "delete:" << delOk;
        qInfo() << "fieldList:" << fnlOk << cols.size() << "insertRec:" << insRecOk
                << "updateRec:" << updOk << "count:" << cntOk;
        qInfo() << "null:" << nullOk << "notOpenExec:" << notOpenExecOk << "close:" << closeOk;

        const bool ok = preOk && openOk && ddlOk && errOk && fieldOk && delOk
            && fnlOk && insRecOk && updOk && cntOk && nullOk && notOpenExecOk && closeOk;
        qInfo() << (ok ? "dbsqlite: PASS" : "dbsqlite: FAIL");
        return ok ? 0 : 56;
    }

    // Self-test модели текстового блока отчёта (KTextBlock).
    if (screen == "textblock") {
        // Собираем шаблон-данные: элемент "/RT_DIAGNOSIS" + его item-config (с TemplateData/
        // AlignH/LineHeight1/FontType) + именованный стиль "ThirdTitle" (Size/Bold/Italic) +
        // значение данных в m_mapConfigs.
        KReportTemplateDataNew data;
        data.m_mapConfigs["DIAG_KEY"] = "Chronic gastritis";   // значение блока

        KReportTemplateItem item;
        item.m_strID = "/RT_DIAGNOSIS"; item.m_strName = "RT_DIAGNOSIS";
        item.m_strTitle = "TR_Diagnosis"; item.m_strType = "RT_TEXT_BLOCK";
        data.m_lstItems.push_back(item);

        KReportTemplateItemConfig cfg;   // конфиг конкретного элемента (ключ = его ID)
        cfg.m_strName = "/RT_DIAGNOSIS";
        cfg.m_mapAttrs["TemplateData"] = "DIAG_KEY";
        cfg.m_mapAttrs["AlignH"]       = "Center";
        cfg.m_mapAttrs["LineHeight1"]  = "24";
        cfg.m_mapAttrs["FontType"]     = "ThirdTitle";
        data.m_mapItemConfigs["/RT_DIAGNOSIS"] = cfg;

        KReportTemplateItemConfig style;   // именованный шрифтовой стиль
        style.m_strName = "ThirdTitle";
        style.m_mapAttrs["Size"]   = "18";
        style.m_mapAttrs["Bold"]   = "1";
        style.m_mapAttrs["Italic"] = "0";
        data.m_mapItemConfigs["ThirdTitle"] = style;

        KTextBlock block(&data.m_lstItems.front(), &data);

        const bool idOk = block.ElementId() == "/RT_DIAGNOSIS";
        const bool dataOk = block.Data() == "Chronic gastritis";
        // Title: "TR_Diagnosis : " (FormatStr "%s : "); tr() без переводов вернёт ключ.
        const bool titleOk = block.Title() == "TR_Diagnosis : ";
        const bool fullOk = block.FullText() == "TR_Diagnosis : Chronic gastritis";

        // Стиль из FontType="ThirdTitle": Size 18, Bold, не Italic.
        int sz = 0; block.FontSize(sz);
        const bool styleOk = sz == 18 && block.Bold() && !block.Italic();

        // Выравнивание и высота строки.
        QFlags<Qt::AlignmentFlag> al;
        const bool alSet = block.Alignment(al);
        const bool alignOk = alSet && (al & Qt::AlignHCenter);
        int lh = 0;
        const bool lhOk = block.LineHeight(1, lh) && lh == 24 && !block.LineHeight(3, lh);

        // Промах item-config: блок без записи в m_mapItemConfigs → пустой конфиг, Data "".
        KReportTemplateItem orphan; orphan.m_strID = "/NONE"; orphan.m_strTitle = "";
        KTextBlock empty(&orphan, &data);
        const bool orphanOk = empty.Data().empty() && empty.Title().empty();

        qInfo() << "id:" << idOk << "data:" << dataOk << "title:" << block.Title().c_str()
                << titleOk << "full:" << fullOk;
        qInfo() << "стиль(size" << sz << "):" << styleOk << "выравн:" << alignOk
                << "lineheight:" << lhOk << "промах:" << orphanOk;

        const bool ok = idOk && dataOk && titleOk && fullOk && styleOk && alignOk && lhOk && orphanOk;
        qInfo() << (ok ? "textblock: PASS" : "textblock: FAIL");
        return ok ? 0 : 47;
    }

    // Self-test зашифрованных списков моделей (KEncSettings — bitwise NOT CSV).
    if (screen == "encset") {
        // 1. Реальный файл прошивки: genc.ini расшифровывается в CSV моделей.
        const QString real = QDir(KSystem::SystemPath())
            .absoluteFilePath("style/X-2600/PyCkeun/scope/genc.ini");
        bool realOk = false;
        if (QFile::exists(real)) {
            KEncSettings enc(real);
            const QString decoded = enc.value(QString()).toString();
            const QStringList models = enc.getStringList();
            realOk = decoded == "EG-500,EG-500L,EG-X20"
                && models == QStringList{"EG-500", "EG-500L", "EG-X20"};
            qInfo() << "genc.ini →" << decoded;
        } else {
            qInfo() << "genc.ini не найден (пропуск реального):" << real;
            realOk = true;   // не блокируем, если ENDO_ROOT без прошивки
        }

        // 2. Roundtrip во временный файл: список → запись(шифр) → чтение(дешифр).
        QTemporaryDir tmp;
        const QString path = QDir(tmp.path()).absoluteFilePath("test_enc.ini");
        KEncSettings w(path);
        const QStringList src{"EC-500", "EC-500L", "EC-X20"};
        w.loadFileFromList(src);
        const bool rtOk = w.getStringList() == src
            && w.value(QString()).toString() == "EC-500,EC-500L,EC-X20";

        // 3. На диске байты ЗАШИФРОВАНЫ (инверсны исходному тексту).
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        const QByteArray raw = f.readAll();
        f.close();
        const QByteArray plain = QByteArray("EC-500,EC-500L,EC-X20");
        bool cipherOk = raw.size() == plain.size();
        for (int i = 0; cipherOk && i < raw.size(); ++i)
            cipherOk = static_cast<char>(~raw[i]) == plain[i];

        // 4. Пустой/несуществующий файл → value отдаёт дефолт.
        KEncSettings none(QDir(tmp.path()).absoluteFilePath("no.ini"));
        const bool defOk = none.value(QString(), "DEF").toString() == "DEF"
            && none.getDataLen() == 0;

        // 5. loadFileFromUsb: собирает блоб из Model/Num + Model/ID<i>.
        const QString usbIni = QDir(tmp.path()).absoluteFilePath("usb.ini");
        {
            QSettings s(usbIni, QSettings::IniFormat);
            s.setValue("Model/Num", 2);
            s.setValue("Model/ID0", "EB-X20");
            s.setValue("Model/ID1", "EB-X20T");
        }
        const QString target = QDir(tmp.path()).absoluteFilePath("benc.ini");
        KEncSettings usb(target);
        usb.loadFileFromUsb(usbIni);
        const bool usbOk = usb.getStringList() == QStringList{"EB-X20", "EB-X20T"};

        qInfo() << "реальный:" << realOk << "roundtrip:" << rtOk << "шифр на диске:" << cipherOk
                << "дефолт:" << defOk << "usb-импорт:" << usbOk;

        const bool ok = realOk && rtOk && cipherOk && defOk && usbOk;
        qInfo() << (ok ? "encset: PASS" : "encset: FAIL");
        return ok ? 0 : 46;
    }

    // Self-test состояния облачной сессии (KSessionInfo — синглтон в памяти).
    if (screen == "session") {
        KSessionInfo &s = KSessionInfo::GetInstance();

        // 1. Дефолты: флаги 0, строки пусты.
        const bool defOk = s.GetManuLoginFlag() == 0 && s.GetServiceLoginFlag() == 0
            && s.GetManuUid().isEmpty() && s.GetServiceAccessToken().isEmpty();

        // 2. Manu-логин: сеттеры → геттеры.
        s.SetManuUid("u123"); s.SetManuUserName("platform_user");
        s.SetManuAccessToken("tok_manu"); s.SetManuLoginFlag(1);
        const bool manuOk = s.GetManuUid() == "u123" && s.GetManuUserName() == "platform_user"
            && s.GetManuAccessToken() == "tok_manu" && s.GetManuLoginFlag() == 1;

        // 3. Service-канал независим от Manu.
        s.SetServiceUid("svc9"); s.SetServiceUserName("engineer");
        s.SetServiceAccessToken("tok_svc"); s.SetServiceLoginFlag(1);
        const bool svcOk = s.GetServiceUid() == "svc9" && s.GetServiceUserName() == "engineer"
            && s.GetServiceAccessToken() == "tok_svc" && s.GetServiceLoginFlag() == 1
            && s.GetManuUid() == "u123";   // Manu не затронут

        // 4. Синглтон: другой GetInstance() видит то же состояние.
        const bool singletonOk = &KSessionInfo::GetInstance() == &s
            && KSessionInfo::GetInstance().GetManuAccessToken() == "tok_manu";

        // 5. getManager() — единый NAM, не null, стабильный.
        QNetworkAccessManager *m1 = s.getManager();
        QNetworkAccessManager *m2 = s.getManager();
        const bool mgrOk = m1 != nullptr && m1 == m2;

        // 6. Логаут: флаги → 0 (строки реф. не чистит).
        s.SetManuLoginFlag(0); s.SetServiceLoginFlag(0);
        const bool logoutOk = s.GetManuLoginFlag() == 0 && s.GetServiceLoginFlag() == 0
            && s.GetManuUid() == "u123";   // uid остаётся

        qInfo() << "дефолты:" << defOk << "manu:" << manuOk << "service:" << svcOk
                << "синглтон:" << singletonOk << "NAM:" << mgrOk << "логаут:" << logoutOk;

        const bool ok = defOk && manuOk && svcOk && singletonOk && mgrOk && logoutOk;
        qInfo() << (ok ? "session: PASS" : "session: FAIL");
        return ok ? 0 : 45;
    }

    // Self-test построителя SQL-условий (KDbStrHandler) + факта SQLCipher-ключа.
    if (screen == "dbstr") {
        using H = KDbStrHandler;

        // 1. SqliteReplace / SqliteCharsEscape (' → '').
        const bool replOk = H::SqliteReplace("a.b.c", ".", "-") == "a-b-c"
            && H::SqliteReplace("abc", "", "X") == "abc"          // пустой from → без изменений
            && H::SqliteCharsEscape("O'Brien") == "O''Brien"
            && H::SqliteCharsEscape("no quote") == "no quote";

        // 2. BuildSimpleCondition — "field op 'value'"; порядок (field, value, op).
        //    value сырой (без escape); пустой field/op → "".
        const bool simpleOk = H::BuildSimpleCondition("name", "John", "=") == "name = 'John'"
            && H::BuildSimpleCondition("id", "5", ">") == "id > '5'"
            && H::BuildSimpleCondition("", "x", "=").empty()
            && H::BuildSimpleCondition("f", "x", "").empty()
            // value не экранируется — кавычка проходит как есть (особенность реф.)
            && H::BuildSimpleCondition("n", "O'B", "=") == "n = 'O'B'";

        // 3. AND / OR.
        const std::string c1 = H::BuildSimpleCondition("a", "1", "=");
        const std::string c2 = H::BuildSimpleCondition("b", "2", "=");
        const bool logicOk = H::BuildAndCondition(c1, c2) == "(a = '1') and (b = '2')"
            && H::BuildOrCondition(c1, c2) == "(a = '1') or (b = '2')";

        // 4. Факт: наш KEntityManage::OpenDb теперь ставит PRAGMA key (SQLCipher-ключ
        //    реф. "SONOSCOPE_X2000_KEY"). На штатном QSQLITE прагма игнорируется — БД
        //    должна открываться и работать (проверяем сквозной CRUD).
        const QString dbPath = "/tmp/endo_dbstr.db";
        QFile::remove(dbPath);
        const bool openOk = KEntityManage::Instance().OpenDb(dbPath);
        KEntityDoctor ent; ent.CreateTable();
        KDoctorEntry e; e.account = "k"; e.count = "1"; e.time = "2026-07-16";
        const bool crudOk = ent.CreateEntity(e) && ent.GetEntityNumber() == 1;

        qInfo() << "replace/escape:" << replOk << "simple:" << simpleOk << "and/or:" << logicOk
                << "| open+ключ:" << openOk << "crud:" << crudOk;

        const bool ok = replOk && simpleOk && logicOk && openOk && crudOk;
        qInfo() << (ok ? "dbstr: PASS" : "dbstr: FAIL");
        return ok ? 0 : 44;
    }

    // Self-test сущности/CRUD врача (KEntityDoctor + KDoctorDBTableHandler, tb_Doctor).
    if (screen == "doctor") {
        const QString dbPath = "/tmp/endo_doctor.db";
        QFile::remove(dbPath);
        KEntityManage &em = KEntityManage::Instance();
        if (!em.OpenDb(dbPath)) { qWarning() << "OpenDb failed"; return 43; }

        KDoctorDBTableHandler h;
        const bool createOk = KEntityDoctor().CreateTable();

        auto mk = [](const QString &acc, const QString &cnt, const QString &time) {
            KDoctorEntry e;
            e.account = acc; e.passwdLength = "6"; e.count = cnt; e.time = time;
            e.reserved1 = "r1"; e.reserved2 = "r2";
            return e;
        };
        const bool addOk = h.AddNewEntity(mk("dr_ivanov", "5", "2026-07-10 09:00:00"))
            && h.AddNewEntity(mk("dr_petrov", "12", "2026-07-16 08:00:00"))
            && h.AddNewEntity(mk("dr_sidorov", "3", "2026-07-15 10:00:00"));

        const bool countOk = h.GetRecordNumber() == 3;

        // Список аккаунтов; технический id проставлен AUTOINCREMENT.
        const QList<KDoctorEntry> all = h.GetAllEntities();
        const bool listOk = all.size() == 3 && !all[0].id.isEmpty();
        const QList<QString> accts = h.GetAllAccount();
        const bool acctOk = accts.contains("dr_ivanov") && accts.contains("dr_petrov")
            && accts.size() == 3;

        // Поиск по account (0/-1).
        KDoctorEntry byAcc;
        const bool byAccOk = h.GetEntityByAccount("dr_petrov", byAcc) == 0
            && byAcc.count == "12"
            && h.GetEntityByAccount("нет", byAcc) == -1;

        // Недавние: ORDER BY time DESC, count DESC → petrov(16-е) > sidorov(15) > ivanov(10).
        const QList<QString> recent = h.GetRecentUseAccount();
        const bool recentOk = recent == QList<QString>{"dr_petrov", "dr_sidorov", "dr_ivanov"};
        const QList<QString> recent2 = h.GetRecentUseAccount(2);
        const bool limitOk = recent2 == QList<QString>{"dr_petrov", "dr_sidorov"};

        // GetEntity по id + Update + Delete.
        const QString id1 = all[0].id;
        KDoctorEntry got;
        const bool getOk = h.GetEntity(id1, got) == 0 && got.account == "dr_ivanov"
            && h.GetEntity("9999", got) == -1;
        got.count = "99";
        const bool updOk = h.UpdateEntity(id1, got);
        KDoctorEntry re; h.GetEntity(id1, re);
        const bool updChkOk = re.count == "99";
        const bool delOk = h.DeleteEntity(id1) && h.GetRecordNumber() == 2
            && h.GetEntity(id1, got) == -1;

        qInfo() << "create:" << createOk << "add:" << addOk << "count:" << countOk
                << "list:" << listOk << acctOk << "| account:" << byAccOk
                << "недавние:" << recentOk << "limit:" << limitOk;
        qInfo() << "get:" << getOk << "update:" << updOk << updChkOk << "delete:" << delOk;

        const bool ok = createOk && addOk && countOk && listOk && acctOk && byAccOk
            && recentOk && limitOk && getOk && updOk && updChkOk && delOk;
        qInfo() << (ok ? "doctor: PASS" : "doctor: FAIL");
        return ok ? 0 : 43;
    }

    // Self-test сущности/CRUD пациента (KEntityPatient + KPatientListDBTableHandler).
    if (screen == "patient") {
        const QString dbPath = "/tmp/endo_patient.db";
        QFile::remove(dbPath);
        KEntityManage &em = KEntityManage::Instance();
        if (!em.OpenDb(dbPath)) { qWarning() << "OpenDb failed"; return 42; }

        KEntityPatient ent;   // соединение endo_main
        const bool createOk = ent.CreateTable();

        KPatientListDBTableHandler h;

        // Добавляем трёх пациентов через хендлер.
        auto mk = [](const QString &pid, const QString &name, const QString &sex) {
            KPatientEntry e;
            e.patientID = pid; e.patientName = name; e.patientSex = sex;
            e.patientBirthday = "1980-01-01"; e.applicantDate = "2026-07-16";
            e.applicants = "Dr.Ivanov"; e.sickBedId = "B12"; e.telephoneNumber = "555-01";
            e.registerNumber = "R-" + pid; e.patientAge = "46"; e.examStatus = "0";
            return e;
        };
        const bool addOk = h.AddNewPatientEntity(mk("P001", "Иванов Иван", "M"))
            && h.AddNewPatientEntity(mk("P002", "Петрова Мария", "F"))
            && h.AddNewPatientEntity(mk("P003", "Сидоров Пётр", "M"));

        const bool countOk = h.GetRecordNumber() == 3 && ent.GetEntityNumber() == 3;

        // Все записи: id проставлен AUTOINCREMENT (бизнес-ключ PatientID сохранён).
        const QList<KPatientEntry> all = h.GetPageRecordFromDb();
        const bool listOk = all.size() == 3 && !all[0].id.isEmpty()
            && all[0].patientID == "P001" && all[1].patientName == "Петрова Мария";
        const QString id2 = all[1].id;   // технический id второго пациента

        // GetEntity по техническому id: код 0 при успехе, -1 при отсутствии.
        KPatientEntry got;
        const bool getOk = h.GetEntity(id2, got) == 0 && got.patientID == "P002"
            && got.patientSex == "F"
            && h.GetEntity("9999", got) == -1;

        // UpdateExamStatus по id (реф. читает → правит ExamStatus → пишет).
        const bool updOk = h.UpdateExamStatus(id2, 2);
        KPatientEntry re;
        h.GetEntity(id2, re);
        const bool statusOk = re.examStatus == "2" && re.patientName == "Петрова Мария";

        // Update полей.
        re.telephoneNumber = "555-99";
        const bool upd2Ok = h.UpdatePatientEntity(id2, re);
        KPatientEntry re2; h.GetEntity(id2, re2);
        const bool upd2ChkOk = re2.telephoneNumber == "555-99";

        // DeleteEntity убирает запись; DeleteEntites — заглушка реф. (ничего не делает).
        const bool delOk = h.DeleteEntity(id2) && h.GetRecordNumber() == 2
            && h.GetEntity(id2, got) == -1;
        const bool stubOk = !h.DeleteEntites({all[0].id, all[2].id})   // заглушка → false
            && h.GetRecordNumber() == 2;                               // ничего не удалила

        qInfo() << "create:" << createOk << "add:" << addOk << "count:" << countOk
                << "list:" << listOk << "| get:" << getOk << "status:" << statusOk
                << "update:" << upd2Ok << upd2ChkOk << "| delete:" << delOk << "заглушка:" << stubOk;

        const bool ok = createOk && addOk && countOk && listOk && getOk && statusOk
            && upd2Ok && upd2ChkOk && delOk && stubOk;
        qInfo() << (ok ? "patient: PASS" : "patient: FAIL");
        return ok ? 0 : 42;
    }

    // Self-test экранного секундомера (KStopWatch — конечный автомат, offscreen).
    if (screen == "stopwatch") {
        KStopWatch sw;   // ctor → InitStopWatch: state Stop, "00:00:00"

        // Сигнал ловим в счётчик смен состояния.
        QList<int> states;
        QObject::connect(&sw, &KStopWatch::StopWatchStateChanged,
                         [&states](int s) { states.append(s); });

        const bool initOk = sw.State() == KStopWatch::Stop && sw.TimeText() == "00:00:00";

        // 1. Старт (F1) → Run; тики UpdateTime считают секунды.
        sw.HandleKeyPress(0x01000030);   // F1
        const bool startOk = sw.State() == KStopWatch::Run;
        sw.UpdateTime(); sw.UpdateTime(); sw.UpdateTime();
        const bool countOk = sw.TimeText() == "00:00:03";

        // 2. Пауза (Space) → Pause; тик во время паузы игнорировать не обязан
        //    (UpdateTime зовётся таймером, который остановлен) — проверяем сам переход.
        sw.HandleKeyPress(0x20);         // Space
        const bool pauseOk = sw.State() == KStopWatch::Pause;
        // 3. Resume (Space) → Run.
        sw.HandleKeyPress(0x20);
        const bool resumeOk = sw.State() == KStopWatch::Run;
        sw.UpdateTime();
        const bool contOk = sw.TimeText() == "00:00:04";

        // 4. Стоп (F1 из Run) → Stop + сброс на 00:00:00.
        sw.HandleKeyPress(0x01000030);
        const bool stopOk = sw.State() == KStopWatch::Stop && sw.TimeText() == "00:00:00";

        // 5. Прочие клавиши — без эффекта; в Stop пауза ничего не делает.
        sw.HandleKeyPress(0x42);         // 'B'
        sw.HandleStopWatchPauseState();
        const bool noopOk = sw.State() == KStopWatch::Stop;

        // Сигнал испускался только на Run/Stop переходах через Runing (не Pause).
        const bool sigOk = states == QList<int>{KStopWatch::Run, KStopWatch::Stop};

        qInfo() << "init:" << initOk << "старт:" << startOk << "счёт:" << countOk
                << sw.TimeText() << "| пауза:" << pauseOk << "resume:" << resumeOk << contOk;
        qInfo() << "стоп+сброс:" << stopOk << "no-op:" << noopOk << "сигналы:" << sigOk << states;

        const bool ok = initOk && startOk && countOk && pauseOk && resumeOk && contOk
            && stopOk && noopOk && sigOk;
        qInfo() << (ok ? "stopwatch: PASS" : "stopwatch: FAIL");
        return ok ? 0 : 41;
    }

    // Self-test строковых/DICOM-утилит пациента (KPatientStringOperation + KDbStringOperation).
    if (screen == "patstr") {
        using P = KPatientStringOperation;

        // 1. StringReplace / StringTrim (фикс. набор " \r\n\t").
        std::string r = "a.b.c"; P::StringReplace(r, ".", "-");
        std::string t = " \t x y \r\n"; P::StringTrim(t);
        std::string allws = " \t\r\n"; P::StringTrim(allws);
        const bool strOk = r == "a-b-c" && t == "x y" && allws.empty();

        // 2. ReplaceInvalidCharInFolderName — набор \ / : * ? " < > |.
        std::string fn = "a/b:c*d?"; P::ReplaceInvalidCharInFolderName(fn, "_");
        const bool invOk = fn == "a_b_c_d_";

        // 3. GetSOPClassUID.
        const bool sopOk = P::GetSOPClassUID(0, true) == "1.2.840.10008.5.1.4.1.1.3.1"
            && P::GetSOPClassUID(0, false) == "1.2.840.10008.5.1.4.1.1.77.1.1"
            && P::GetSOPClassUID(1, false) == "1.2.840.10008.5.1.4.1.1.7"
            && P::GetSOPClassUID(2, false) == "1.2.840.10008.5.1.4.1.1.6.2"
            && P::GetSOPClassUID(99, false).empty();

        // 4. SplitDicomPatientName — порядок реф. token0→p4, token1→p2, token2→p3.
        std::string p2, p3, p4;
        P::SplitDicomPatientName("Family^Given^Middle", p2, p3, p4);
        const bool splitOk = p4 == "Family" && p2 == "Given" && p3 == "Middle";
        std::string q2, q3, q4;
        P::SplitDicomPatientName("NoCaret", q2, q3, q4);   // без '^' → p2 = вся строка
        const bool split2Ok = q2 == "NoCaret" && q3.empty() && q4.empty();

        // 5. AssembleDicomFilePath — flag управляет расширением .dcm.
        const bool pathOk = P::AssembleDicomFilePath("/d", "f", false) == "/d/f.dcm"
            && P::AssembleDicomFilePath("/d", "f", true) == "/d/f";

        // 6. GetISOCharactersetOfDicom — таблица + флаг распознавания.
        bool rec = false;
        const bool isoOk = P::GetISOCharactersetOfDicom("ISO_IR 192", rec) == "utf-8" && rec
            && P::GetISOCharactersetOfDicom("ISO_IR 100", rec) == "ISO-8859-1"
            && P::GetISOCharactersetOfDicom("НетТакого", rec).empty() && !rec;

        // 7. ConvertCharacterset (iconv): пустой → true no-op; ASCII utf-8→utf-8 сохраняется.
        std::string empty; const bool convEmpty = P::ConvertCharacterset("utf-8", "utf-8", empty);
        std::string ascii = "hello";
        const bool convAscii = P::ConvertCharacterset("utf-8", "utf-8", ascii) && ascii == "hello";
        std::string u = "world"; const bool utf8Ok = P::ConvertCharactersetToUTF8(u);

        // 8. GenerateUniqueIdentifier — суффикс по типу (финал DCMTK — заглушка).
        const bool uidOk = P::GenerateUniqueIdentifier("Patient") == ".1.1"
            && P::GenerateUniqueIdentifier("Querylist") == ".1.10"
            && P::GenerateUniqueIdentifier("НетТакого").empty();

        // 9. KDbStringOperation: строковые делегируют KPatient; iconv — заглушки true.
        std::string dr = "x/y/z"; KDbStringOperation::StringReplace(dr, "/", "-");
        std::string di = "нетронуто";
        const bool dbOk = dr == "x-y-z"
            && KDbStringOperation::ConvertCharacterset("gbk", "utf-8", di) && di == "нетронуто"
            && KDbStringOperation::ConvertCharactersetToUTF8(di);   // заглушка, не меняет
        bool drec = false;
        const bool dbIsoOk = KDbStringOperation::GetISOCharactersetOfDicom("ISO_IR 144", drec)
            == "ISO-8859-5";

        qInfo() << "строки:" << strOk << "invalid:" << invOk << "SOP:" << sopOk
                << "split:" << splitOk << split2Ok << "path:" << pathOk;
        qInfo() << "ISO:" << isoOk << "iconv:" << convEmpty << convAscii << utf8Ok
                << "UID:" << uidOk << "| KDb:" << dbOk << dbIsoOk;

        const bool ok = strOk && invOk && sopOk && splitOk && split2Ok && pathOk && isoOk
            && convEmpty && convAscii && utf8Ok && uidOk && dbOk && dbIsoOk;
        qInfo() << (ok ? "patstr: PASS" : "patstr: FAIL");
        return ok ? 0 : 40;
    }

    // Self-test слоя ini машинного контроля (KControlINI).
    // Пишет <root>/data/protected/kmachinecontrol/control.ini → tmp ENDO_ROOT!
    if (screen == "controlini") {
        // 1. Пути: каталог kmachinecontrol создаётся, имена файлов верны.
        const QString ctrl = KControlINI::ControlINIPath();
        const bool pathOk = ctrl.endsWith("data/protected/kmachinecontrol/control.ini")
            && KControlINI::PlainINIpath().endsWith("kmachinecontrol/plain.ini")
            && KControlINI::MatchProListIni().endsWith("kmachinecontrol/matchprolist.ini")
            && KControlINI::HistoryLicenseRecord().endsWith("kmachinecontrol/licensehistory.ini")
            && QDir(QFileInfo(ctrl).absolutePath()).exists();   // каталог создан

        // 2. Дефолты на пустом файле.
        _MC_Time t0;
        KControlINI::ReadMcTime(t0);
        const bool defOk = !t0.controlTime && t0.deadline == "2099-01-01" && t0.remainDays == 0
            && !KControlINI::IsStartTimeControl() && !KControlINI::IsStartEndoControl()
            && KControlINI::GetDeadline() == "2099-01-01" && KControlINI::GetRemainDays() == 0
            && KControlINI::GetMatchEndos().isEmpty();

        // 3. WriteMcTime → ReadMcTime roundtrip.
        _MC_Time t; t.controlTime = true; t.deadline = "2027-03-15"; t.remainDays = 42;
        KControlINI::WriteMcTime(t);
        _MC_Time t2;
        KControlINI::ReadMcTime(t2);
        const bool timeOk = t2.controlTime && t2.deadline == "2027-03-15" && t2.remainDays == 42
            && KControlINI::IsStartTimeControl()          // те же ключи через поэлементный доступ
            && KControlINI::GetDeadline() == "2027-03-15" && KControlINI::GetRemainDays() == 42;

        // 4. WriteMcEndo → ReadMcEndo (QStringList).
        _MC_Endo e; e.controlEndo = true; e.endos = QStringList{"SN001", "SN002", "SN003"};
        KControlINI::WriteMcEndo(e);
        _MC_Endo e2;
        KControlINI::ReadMcEndo(e2);
        const bool endoOk = e2.controlEndo && e2.endos == QStringList{"SN001", "SN002", "SN003"}
            && KControlINI::IsStartEndoControl()
            && KControlINI::GetMatchEndos() == QStringList{"SN001", "SN002", "SN003"};

        // 5. Поэлементные сеттеры пишут те же ключи, что видит блочное чтение.
        KControlINI::SetRemainDays(7);
        KControlINI::SetDeadline("2030-01-01");
        KControlINI::StartTimeControl(false);
        KControlINI::SetMatchEndos(QStringList{"X"});
        _MC_Time t3; KControlINI::ReadMcTime(t3);
        _MC_Endo e3; KControlINI::ReadMcEndo(e3);
        const bool setOk = t3.remainDays == 7 && t3.deadline == "2030-01-01" && !t3.controlTime
            && e3.endos == QStringList{"X"};

        // 6. Формат файла — QSettings (bool как true/false, НЕ KConfig True/False).
        QFile f(ctrl);
        f.open(QIODevice::ReadOnly);
        const QString text = QString::fromUtf8(f.readAll());
        f.close();
        const bool fmtOk = text.contains("Control_endo=true")   // QSettings-стиль
            && !text.contains("True");                          // не KConfig-стиль

        qInfo() << "пути:" << pathOk << "дефолты:" << defOk << "| time:" << timeOk
                << "endo:" << endoOk << "сеттеры:" << setOk << "| формат QSettings:" << fmtOk;

        const bool ok = pathOk && defOk && timeOk && endoOk && setOk && fmtOk;
        qInfo() << (ok ? "controlini: PASS" : "controlini: FAIL");
        return ok ? 0 : 39;
    }

    // Self-test файловых/дисковых утилит (KDbFileOperation) — чистая логика.
    if (screen == "dbfileop") {
        QTemporaryDir tmp;
        const QString root = tmp.path();
        const QString sub  = QDir(root).absoluteFilePath("a/b/c");
        const std::string f1 = QDir(root).absoluteFilePath("x.jpg").toStdString();
        const std::string f2 = QDir(root).absoluteFilePath("y.txt").toStdString();

        // 1. Создание каталога, файлов, проверки существования и размера.
        const bool mk = KDbFileOperation::CreateFolder(sub.toStdString());
        { QFile f(QString::fromStdString(f1)); f.open(QIODevice::WriteOnly); f.write("12345"); f.close(); }
        { QFile f(QString::fromStdString(f2)); f.open(QIODevice::WriteOnly); f.write("hi"); f.close(); }
        const bool existOk = mk && KDbFileOperation::IsFileExist(f1)
            && KDbFileOperation::IsFileDirExist(sub.toStdString())
            && !KDbFileOperation::IsFileExist(f1 + ".none")
            && KDbFileOperation::GetFileSize(f1) == 5
            && KDbFileOperation::GetFileSize("/no/such") == -1
            && KDbFileOperation::IsPatientDataExist(f2);

        // 2. Копирование, удаление файла.
        const std::string f1copy = QDir(root).absoluteFilePath("x2.jpg").toStdString();
        const bool copyOk = KDbFileOperation::CopyFileToFile(f1, f1copy)
            && KDbFileOperation::IsFileExist(f1copy)
            && KDbFileOperation::RemoveFile(f1copy)
            && !KDbFileOperation::IsFileExist(f1copy);

        // 3. Листинг по маске.
        std::vector<std::string> jpgs;
        KDbFileOperation::GetFilesByFilter(root.toStdString(), "*.jpg", jpgs);
        const bool listOk = jpgs.size() == 1 && jpgs[0] == "x.jpg";

        // 4. basename / имя последнего каталога.
        const bool nameOk = KDbFileOperation::GetFileNameWithoutDir("/a/b/file.txt") == "file.txt"
            && KDbFileOperation::GetLastDirName("/a/b/c") == "c";

        // 5. StringReplace — все вхождения; пустой from → no-op.
        std::string s = "a.b.c";
        KDbFileOperation::StringReplace(s, ".", "-");
        std::string s2 = "abc";
        KDbFileOperation::StringReplace(s2, "", "X");
        const bool replOk = s == "a-b-c" && s2 == "abc";

        // 6. GetNumOfSpaces (частичная семантика).
        const bool spOk = KDbFileOperation::GetNumOfSpaces("a b c", 5) == 2
            && KDbFileOperation::GetNumOfSpaces("a b c", 2) == 1
            && KDbFileOperation::GetNumOfSpaces(nullptr, 5) == 0;

        // 7. Ёмкость ФС (statfs) — total > 0, free > 0, free <= total.
        double total = 0, free = 0;
        KDbFileOperation::GetCapacityByPath(root.toUtf8().constData(), total, free);
        double free2 = 0, total2 = 0;
        KDbFileOperation::GetSpecifyFreeCapacity(root.toUtf8().constData(), free2);
        KDbFileOperation::GetSpecifyTotalCapacity(root.toUtf8().constData(), total2);
        KDbFileOperation::GetCapacityByPath("/no/such/path", total2 = 9, free2 = 9);
        const bool capOk = total > 0 && free > 0 && free <= total
            && total2 == 0 && free2 == 0;   // несуществующий путь → 0/0

        // 8. Удаление каталога рекурсивно.
        const bool delOk = KDbFileOperation::DeleteFolder(QDir(root).absoluteFilePath("a").toStdString())
            && !KDbFileOperation::IsFileDirExist(sub.toStdString());

        qInfo() << "существование:" << existOk << "копир/удал:" << copyOk << "листинг:" << listOk
                << "имена:" << nameOk;
        qInfo() << "замена:" << replOk << "пробелы:" << spOk << "ёмкость:" << capOk
                << "(total" << total << "free" << free << ")" << "удал.кат:" << delOk;

        const bool ok = existOk && copyOk && listOk && nameOk && replOk && spOk && capOk && delOk;
        qInfo() << (ok ? "dbfileop: PASS" : "dbfileop: FAIL");
        return ok ? 0 : 38;
    }

    // Self-test доступа производителя (KManuPwdMng) — пишет [Manu] в system.ini,
    // брать временный ENDO_ROOT!
    if (screen == "manupwd") {
        KManuPwdMng &m = KManuPwdMng::GetInstance();

        // 1. getPwd на ФИКСИРОВАННОЙ дате — сверка формулы a*month*n → 4 цифры.
        //    2026-07: a=26, m=7. n=51647 → S=26*7*51647=9399754 → цифры d5d4d3d2=9975.
        const QDate d(2026, 7, 1);
        const bool pwdOk = KManuPwdMng::getPwd(51647, d) == "9975"
            && KManuPwdMng::getPwd(32711, d) == QStringLiteral("%1%2%3%4")
                   .arg((26*7*32711/10000)%10).arg((26*7*32711/1000)%10)
                   .arg((26*7*32711/100)%10).arg((26*7*32711/10)%10)
            // ведущие нули сохраняются (ровно 4 символа)
            && KManuPwdMng::getPwd(1, d).length() == 4;
        // Год, кратный 100 → a=55 (реф.).
        const bool centuryOk = KManuPwdMng::getPwd(51647, QDate(2100, 7, 1))
            == QStringLiteral("%1%2%3%4").arg((55*7*51647/10000)%10).arg((55*7*51647/1000)%10)
                   .arg((55*7*51647/100)%10).arg((55*7*51647/10)%10);

        // 2. Полные пароли: префикс + getPwd(currentDate) + суффикс.
        const QString today4 = KManuPwdMng::getPwd(51647);
        const bool fullOk = m.GetPassWord() == "se" + today4 + "mnf"
            && m.GetAdmPassWord().startsWith("hd") && m.GetAdmPassWord().endsWith("adm")
            && m.GetServicePassWord().startsWith("se") && m.GetServicePassWord().endsWith("srv")
            && m.GetPassWord().length() == 9;

        // 3. GenerateLicense: детерминирован, пустой sn → дефолт, '/' → '-'.
        const QString lic = m.GenerateLicense("AB/12", 37);
        const bool licOk = !lic.isEmpty()
            && lic == m.GenerateLicense("AB-12", 37)          // '/' и '-' эквивалентны
            && m.GenerateLicense("", 5) == m.GenerateLicense("201707182011", 5)  // дефолтный sn
            && m.GenerateLicense("X", 40) != m.GenerateLicense("X", 41);          // код влияет

        // 4. CheckPermission: отсчёт оставшихся дней от отметки.
        KSystemSet &ss = KSystemSet::GetInstance();
        const QDate today = QDate::currentDate();
        ss.SetManuEnable(true);
        ss.SetManuLeftTime(59);
        ss.SetManuMarkTime(today.addDays(-10));   // прошло 10 дней
        m.CheckPermission();
        const bool countOk = ss.GetManuEnable() && ss.GetManuLeftTime() == 49
            && ss.GetManuMarkTime() == today;
        // Истечение: осталось меньше прошедшего → гасим доступ.
        ss.SetManuLeftTime(3);
        ss.SetManuMarkTime(today.addDays(-10));
        m.CheckPermission();
        const bool expireOk = !ss.GetManuEnable() && ss.GetManuLeftTime() == 0;
        // Выключенный доступ CheckPermission не трогает.
        ss.SetManuEnable(false);
        ss.SetManuLeftTime(42);
        m.CheckPermission();
        const bool offOk = ss.GetManuLeftTime() == 42;

        // 5. UpdateSystemTime: двигает отметку только при включённом доступе.
        ss.SetManuEnable(true);
        ss.SetManuMarkTime(today);
        m.UpdateSystemTime(today.addDays(5));
        const bool updOk = ss.GetManuMarkTime() == today.addDays(5);
        ss.SetManuEnable(false);
        ss.SetManuMarkTime(today);
        m.UpdateSystemTime(today.addDays(5));
        const bool updOffOk = ss.GetManuMarkTime() == today;   // выключен → не трогает

        // 6. Реестр ключей: антиповтор.
        ss.SetManuLicenseKey(100);
        ss.SetManuLicenseKey(200);
        ss.SetManuLicenseKey(100);   // дубль игнорируется
        const bool keyOk = ss.GetManuLicenseKeyList() == QList<int>{100, 200};

        qInfo() << "getPwd:" << pwdOk << "век:" << centuryOk << "| пароли:" << fullOk
                << m.GetPassWord() << "| лицензия:" << licOk;
        qInfo() << "countdown:" << countOk << "истечение:" << expireOk << "выкл:" << offOk
                << "| время:" << updOk << updOffOk << "| ключи:" << keyOk;

        const bool ok = pwdOk && centuryOk && fullOk && licOk && countOk && expireOk && offOk
            && updOk && updOffOk && keyOk;
        qInfo() << (ok ? "manupwd: PASS" : "manupwd: FAIL");
        return ok ? 0 : 37;
    }

    // Self-test параметров шаблона (KTemplateParamCfg — 3-й наследник KMeaXMLBase).
    // В прошивке класс МЁРТВЫЙ (нет xref и нет ReportTemplateParam.xml) → фикстура своя.
    if (screen == "templateparam") {
        QTemporaryDir tmp;
        const QString base = tmp.path() + "/";
        const QString dir  = base + "report/ReportTemplate";
        QDir().mkpath(dir);
        const QString file = dir + "/ReportTemplateParam.xml";
        {
            QFile f(file);
            f.open(QIODevice::WriteOnly);
            f.write("<root>\n"
                    "  <Font>\n"
                    "    <Item Name=\"Size\" Value=\"16\"/>\n"
                    "    <Item Name=\"Bold\" Value=\"1\"/>\n"
                    "    <Item Name=\"НетЗначения\" Value=\"\"/>\n"   // Value пуст → пропуск
                    "    <Item Value=\"безымянный\"/>\n"              // Name пуст → пропуск
                    "    <Other Name=\"x\" Value=\"y\"/>\n"           // не Item → пропуск
                    "  </Font>\n"
                    "  <ЛюбоеИмя>\n"                                   // имя группы произвольно
                    "    <Item Name=\"k\" Value=\"v\"/>\n"
                    "  </ЛюбоеИмя>\n"
                    "  <Пустая>\n"                                     // без валидных Item…
                    "    <Item Name=\"\" Value=\"\"/>\n"
                    "  </Пустая>\n"                                    // …→ группа отбрасывается
                    "</root>\n");
            f.close();
        }

        KTemplateParamCfg cfg;
        // ОТЛИЧИЕ от сиблингов: arg1 реально используется как базовый каталог.
        cfg.Check(base.toStdString(), "игнорируется");
        const bool pathOk = QString::fromStdString(cfg.ParamFile())
                                .contains("report/ReportTemplate")
            && QString::fromStdString(cfg.ParamFile()).endsWith("ReportTemplateParam.xml");

        const int lc = cfg.LoadCache();
        const auto &p = cfg.Params();
        const bool loadOk = lc == 1 && p.size() == 2          // «Пустая» отброшена
            && p.count("Font") == 1 && p.count("ЛюбоеИмя") == 1
            && p.count("Пустая") == 0
            && p.at("Font").size() == 2;                       // пропущены Item без Name/Value

        std::string v;
        const bool getOk = cfg.GetTemplateParam("Font", "Size", v) == 1 && v == "16"
            && cfg.GetTemplateParam("ЛюбоеИмя", "k", v) == 1 && v == "v";
        // Промах: честный 0, out НЕ трогается (в отличие от KTemplateLibCfg::GetTemplateLib).
        std::string keep = "не трогать";
        const bool missOk = cfg.GetTemplateParam("НетГруппы", "Size", keep) == 0
            && cfg.GetTemplateParam("Font", "НетКлюча", keep) == 0
            && keep == "не трогать";

        // Коды: нет файла → -40; XML без корня "root" → 0.
        std::map<std::string, std::map<std::string, std::string>> m;
        const int missRet = KTemplateParamCfg::ParseParamFile("/no/such.xml", m);
        const QString noRoot = QDir(tmp.path()).absoluteFilePath("noroot.xml");
        { QFile f(noRoot); f.open(QIODevice::WriteOnly); f.write("<other/>"); f.close(); }
        const int noRootRet = KTemplateParamCfg::ParseParamFile(noRoot.toStdString(), m);
        const bool retOk = missRet == -40 && noRootRet == 0;

        // GetModuleVersion не переопределён → -1 (наследуется от базы).
        const bool verOk = cfg.GetModuleVersion() == -1;

        qInfo() << "путь:" << pathOk << "| LoadCache:" << lc << loadOk << "групп:" << p.size()
                << "| Get:" << getOk << "промах не трогает out:" << missOk
                << "| коды:" << retOk << "| версия:" << verOk;

        const bool ok = pathOk && loadOk && getOk && missOk && retOk && verOk;
        qInfo() << (ok ? "templateparam: PASS" : "templateparam: FAIL");
        return ok ? 0 : 36;
    }

    // Self-test библиотеки шаблонов (KTemplateLibCfg + report_template::*).
    if (screen == "templatelib") {
        // 1. Чистые функции ID: join/split и их краевые случаи (реф.).
        const std::string sep = report_template::STR_PATH_SEPARATOR;
        const bool idOk = report_template::GenerateIDByPath({"A", "B", "C"}, sep) == "A/B/C"
            && report_template::GenerateIDByPath({}, sep).empty()
            && report_template::RevertPathByID("A/B/C", sep) == std::vector<std::string>{"A","B","C"}
            // пустые токены в начале сохраняются, хвостового пустого НЕТ
            && report_template::RevertPathByID("/A", sep) == std::vector<std::string>{"", "A"}
            && report_template::RevertPathByID("A/", sep) == std::vector<std::string>{"A"}
            && report_template::RevertPathByID("", sep).empty();
        std::string parent;
        report_template::GetParentItemID("/A/B", parent);
        const bool parentOk = parent == "/A";

        // 2. MergeSubItem: дедуп по m_strName (НЕ по ID), dst побеждает,
        //    новые клонируются с исходным m_strID, out = ID новых + потомков.
        std::list<KReportTemplateItem> dst, src;
        KReportTemplateItem d1;
        d1.m_strName = "A"; d1.m_strID = "/A"; d1.m_strTitle = "dst-заголовок";
        dst.push_back(d1);
        KReportTemplateItem s1;                       // то же имя → слияние
        s1.m_strName = "A"; s1.m_strID = "/A"; s1.m_strTitle = "src-заголовок";
        KReportTemplateItem s1c;                      // новый ребёнок внутри A
        s1c.m_strName = "A1"; s1c.m_strID = "/A/A1";
        s1.m_lstSubItems.push_back(s1c);
        KReportTemplateItem s2;                       // новое имя → клон
        s2.m_strName = "B"; s2.m_strID = "/B";
        KReportTemplateItem s2c;
        s2c.m_strName = "B1"; s2c.m_strID = "/B/B1";
        s2.m_lstSubItems.push_back(s2c);
        src.push_back(s1); src.push_back(s2);

        std::list<std::string> out;
        report_template::MergeSubItem(dst, src, out);
        const bool mergeOk = dst.size() == 2
            && dst.front().m_strTitle == "dst-заголовок"        // dst побеждает
            && dst.front().m_lstSubItems.size() == 1            // рекурсия в под-элементы
            && dst.back().m_strName == "B" && dst.back().m_strID == "/B"
            // out — только НОВЫЕ: /A/A1 (новый ребёнок), /B и его потомок /B/B1
            && out == std::list<std::string>{"/A/A1", "/B", "/B/B1"};

        // 3. MergeData: для обеих map побеждает src.
        KReportTemplateDataNew dd, ss;
        dd.m_mapConfigs["k"] = "dst"; dd.m_mapConfigs["only-dst"] = "1";
        ss.m_mapConfigs["k"] = "src";
        KReportTemplateItemConfig ic; ic.m_strName = "/X"; ic.m_mapAttrs["a"] = "src";
        ss.m_mapItemConfigs["/X"] = ic;
        std::list<std::string> ids;
        report_template::MergeData(dd, ss, ids);
        const bool mdOk = dd.m_mapConfigs["k"] == "src"          // src побеждает
            && dd.m_mapConfigs["only-dst"] == "1"                // чужие ключи целы
            && dd.m_mapItemConfigs["/X"].m_mapAttrs["a"] == "src";

        // 4. Реальная прошивка: SubContentList.xml → плоский пул блоков.
        KTemplateLibCfg lib;
        lib.Check("", "");
        const int lc = lib.LoadCache();
        const KReportTemplateDataNew &pool = lib.Data();
        const bool poolOk = lc == 1 && !pool.m_lstItems.empty()
            && !pool.m_mapItemConfigs.empty();

        // 5. TemplateTypes.xml → 5 групп (ReportTemplateNP-*).
        const int lg = lib.LoadCacheGroup();
        const std::map<std::string, KReportTemplateDataNew> &libs = lib.TemplateLibs();
        const bool groupOk = lg == 1 && libs.size() == 5
            && libs.count("ReportTemplateNP-1x4") == 1
            && libs.count("ReportTemplateNP-nx3") == 1
            && !libs.at("ReportTemplateNP-1x4").m_lstItems.empty();

        // 6. GetTemplateLib: при ПРОМАХЕ реф. отдаёт m_data, а не nullptr.
        const bool getOk = lib.GetTemplateLib("ReportTemplateNP-1x4") != nullptr
            && lib.GetTemplateLib("нет такой") == &lib.Data();

        // 7. Коды ошибок: нет файла → -40; XML без корня "root" → 0 (не -40).
        KReportTemplateDataNew tmp2;
        const int missRet = lib.LoadTemplateLib("/no/such/file.xml", tmp2, "");
        QTemporaryDir td;
        const QString noRoot = QDir(td.path()).absoluteFilePath("noroot.xml");
        { QFile f(noRoot); f.open(QIODevice::WriteOnly); f.write("<other/>"); f.close(); }
        const int noRootRet = lib.LoadTemplateLib(noRoot.toStdString(), tmp2, "");
        const bool retOk = missRet == -40 && noRootRet == 0;

        // 8. RemoveNotUserItem: чистит configs, оставляет только UserDefine,
        //    дерево — по префиксам длиной >= 2.
        KReportTemplateDataNew rd;
        rd.m_mapConfigs["c"] = "будет удалён";
        KReportTemplateItemConfig user; user.m_bUserDefine = true; user.m_strName = "/A/B";
        KReportTemplateItemConfig sys;  sys.m_bUserDefine = false; sys.m_strName = "/A/C";
        rd.m_mapItemConfigs["/A/B"] = user;
        rd.m_mapItemConfigs["/A/C"] = sys;
        KReportTemplateItem ra; ra.m_strName = "A"; ra.m_strID = "/A";
        KReportTemplateItem rb; rb.m_strName = "B"; rb.m_strID = "/A/B";
        KReportTemplateItem rc; rc.m_strName = "C"; rc.m_strID = "/A/C";
        ra.m_lstSubItems.push_back(rb); ra.m_lstSubItems.push_back(rc);
        rd.m_lstItems.push_back(ra);
        lib.RemoveNotUserItem(rd);
        // keep = {"/A" (префикс длины 2 от ["","A","B"]), "/A/B"} → "/A/C" вылетает.
        const bool rmOk = rd.m_mapConfigs.empty()
            && rd.m_mapItemConfigs.size() == 1 && rd.m_mapItemConfigs.count("/A/B") == 1
            && rd.m_lstItems.size() == 1
            && rd.m_lstItems.front().m_lstSubItems.size() == 1
            && rd.m_lstItems.front().m_lstSubItems.front().m_strID == "/A/B";

        qInfo() << "ID join/split:" << idOk << "родитель:" << parentOk << "| merge:" << mergeOk
                << "MergeData:" << mdOk;
        qInfo() << "пул блоков:" << poolOk << "элементов:" << pool.m_lstItems.size()
                << "| групп:" << libs.size() << groupOk << "| GetTemplateLib:" << getOk
                << "| коды:" << retOk << "| RemoveNotUserItem:" << rmOk;

        const bool ok = idOk && parentOk && mergeOk && mdOk && poolOk && groupOk && getOk
            && retOk && rmOk;
        qInfo() << (ok ? "templatelib: PASS" : "templatelib: FAIL");
        return ok ? 0 : 35;
    }

    // Self-test генератора номеров осмотра (KExamNoGenerate).
    // Пишет <root>/data/protected/ExamListId.ini → брать временный ENDO_ROOT!
    if (screen == "examno") {
        const QString ini = QDir(KSystem::ProtectedPath()).absoluteFilePath("ExamListId.ini");
        const QString date = QDate::currentDate().toString("yyyyMMdd");

        // 1. Файла нет: InitConfigFile создаёт каталог и ПУСТОЙ файл; индекс → дефолт 0.
        KExamNoGenerate::InitConfigFile();
        const bool initOk = QFile::exists(ini) && KExamNoGenerate::GetExamIdIndex() == 0;

        // 2. MakeExamId: инкремент только в памяти, на диск НЕ пишет.
        KSystemStatus::GetInstance().SetViewType(0);          // эндоскоп → без суффикса
        const std::string id1 = KExamNoGenerate::MakeExamId();
        const bool fmtOk = id1 == (date + "0001").toStdString();
        const bool notSavedOk = KExamNoGenerate::GetExamIdIndex() == 0;   // диск не тронут

        // Повторный MakeExamId без коммита даёт ТОТ ЖЕ номер (читает индекс с диска).
        const bool sameOk = KExamNoGenerate::MakeExamId() == id1;

        // 3. SetExamId — коммит текущего индекса на диск.
        KExamNoGenerate::SetExamId();
        const bool savedOk = KExamNoGenerate::GetExamIdIndex() == 1;
        const std::string id2 = KExamNoGenerate::MakeExamId();
        const bool nextOk = id2 == (date + "0002").toStdString();

        // 4. Суффикс 'R' — при любом ненулевом ViewType (камерный режим).
        KSystemStatus::GetInstance().SetViewType(1);
        const bool rOk = KExamNoGenerate::MakeExamId() == (date + "0002R").toStdString();
        KSystemStatus::GetInstance().SetViewType(7);          // реф. — cbnz, не «==1»
        const bool rAnyOk = KExamNoGenerate::MakeExamId() == (date + "0002R").toStdString();
        KSystemStatus::GetInstance().SetViewType(0);

        // 5. Переполнение: реф. берёт ОСТАТОК от 9999, а не сбрасывает в 1.
        KExamNoGenerate::SetExamId(9999);
        const bool wrapOk = KExamNoGenerate::MakeExamId() == (date + "0001").toStdString();
        KExamNoGenerate::SetExamId(19997);   // +1 = 19998; 19998 % 9999 == 0
        const bool wrapZeroOk = KExamNoGenerate::MakeExamId() == (date + "0000").toStdString();

        // 6. SetExamId(<0) → 0; формат файла — KConfig.
        KExamNoGenerate::SetExamId(-5);
        const bool negOk = KExamNoGenerate::GetExamIdIndex() == 0;
        QFile f(ini);
        f.open(QIODevice::ReadOnly);
        const QString text = QString::fromUtf8(f.readAll());
        f.close();
        const bool fileOk = text.contains("[ExamId]") && text.contains("ExamIdIndex=0");

        // 7. IsValidExamId — заглушка `return true` (аргумент не читается).
        const bool validOk = KExamNoGenerate::IsValidExamId("что угодно")
            && KExamNoGenerate::IsValidExamId("");

        qInfo() << "init:" << initOk << "формат:" << fmtOk << id1.c_str()
                << "| не пишет на диск:" << notSavedOk << "повтор тот же:" << sameOk;
        qInfo() << "коммит:" << savedOk << "следующий:" << nextOk << "| суффикс R:" << rOk
                << "любой ViewType:" << rAnyOk;
        qInfo() << "остаток 9999:" << wrapOk << "остаток 0:" << wrapZeroOk
                << "| отриц.→0:" << negOk << "файл:" << fileOk << "| IsValid:" << validOk;

        const bool ok = initOk && fmtOk && notSavedOk && sameOk && savedOk && nextOk && rOk
            && rAnyOk && wrapOk && wrapZeroOk && negOk && fileOk && validOk;
        qInfo() << (ok ? "examno: PASS" : "examno: FAIL");
        return ok ? 0 : 34;
    }

    // Self-test строковых утилит (KMeaStringUtil) — закрепляет НЕинтуитивную
    // семантику реф., на которую завязана отчётная ветка.
    if (screen == "strutil") {
        KMeaStringUtil u;   // реф.: методы НЕ static, класс пустой

        // 1. SplitStr — разделитель это НАБОР символов; пустые токены пропускаются.
        const std::vector<std::string> a = u.SplitStr("25,50,25", ",");
        const std::vector<std::string> b = u.SplitStr("a,,b", ",");
        const std::vector<std::string> c = u.SplitStr("a,b;c", ",;");   // набор из двух
        const bool splitOk = a == std::vector<std::string>{"25", "50", "25"}
            && b == std::vector<std::string>{"a", "b"}                  // пустой токен выпал
            && c == std::vector<std::string>{"a", "b", "c"}
            && u.SplitStr("нет разделителя", ",") == std::vector<std::string>{"нет разделителя"}
            && u.SplitStr("", ",").empty()
            && u.SplitStr("abc", "").empty();          // пустой набор → ПУСТОЙ вектор

        // 2. SplitStr2 — разделитель это ПОДСТРОКА (в отличие от SplitStr).
        const bool split2Ok = u.SplitStr2("a::b::c", "::") == std::vector<std::string>{"a", "b", "c"}
            // Подстрока ",;" в "a,b" не найдена → весь вход одним токеном…
            && u.SplitStr2("a,b", ",;") == std::vector<std::string>{"a,b"}
            // …тогда как для SplitStr ",;" — это НАБОР, и оба символа-разделителя найдены.
            && u.SplitStr("a,b", ",;") == std::vector<std::string>{"a", "b"}
            && u.SplitStr2("abc", "").empty();   // пустой разделитель → ПУСТОЙ вектор

        // 3. Конверсии: без валидации, без исключений.
        const bool convOk = u.ConvertStringToInt("42") == 42
            && u.ConvertStringToInt("abc") == 0 && u.ConvertStringToInt("") == 0
            && u.ConvertStringToInt("12abc") == 12 && u.ConvertStringToInt("0x10") == 0
            && qAbs(u.ConvertStringToDouble("3.5") - 3.5) < 1e-9
            && u.ConvertStringToDouble("abc") == 0.0
            && u.ConvertIntToString(-7) == "-7"
            // stringstream, а не to_string: 6 значащих цифр
            && u.ConvertDoubleToString(100.0) == "100"
            && u.ConvertDoubleToString(1234567.0) == "1.23457e+06"
            && u.ConvertIntToFormatString(42, 5) == "00042";

        // 4. Тримминг: TrimBeginEndStr — ТОЛЬКО пробел; TrimAllStr — отовсюду.
        std::string t1 = "  x  ";
        const std::string t1r = u.TrimBeginEndStrRef(t1);
        std::string t2 = " a b\tc\nd\re ";
        u.TrimAllStr(t2);
        const bool trimOk = t1r == "x" && t1 == "x"                 // тримит in-place
            && u.TrimBeginEndStr("\t x \n") == "\t x \n"            // \t\n НЕ тримятся!
            && u.TrimBeginEndStr("   ").empty()
            && t2 == "abcd\re";                                     // \r НЕ удаляется

        // 5. Префикс/суффикс: пустой → true. Регистрозависимо.
        const bool edgeOk = u.IsBeginWith("hello", "he") && !u.IsBeginWith("hello", "HE")
            && u.IsBeginWith("hello", "") && !u.IsBeginWith("h", "hello")
            && u.IsEndWith("hello", "lo") && u.IsEndWith("hello", "")
            && u.IsEqual("ABC", "abc", false) && !u.IsEqual("ABC", "abc", true);

        // 6. ReplaceStr — все вхождения; пустой from → no-op.
        std::string r1 = "a-b-c";
        u.ReplaceStr(r1, "-", "+");
        std::string r2 = "abc";
        u.ReplaceStr(r2, "", "X");
        // ReplaceIllegalChar — только ПЕРВОЕ вхождение каждого символа.
        std::string r3 = "a/b/c:d";
        u.ReplaceIllegalChar(r3, "_");
        const bool replOk = r1 == "a+b+c" && r2 == "abc" && r3 == "a_b/c_d";

        // 7. DeleteChars + FormatStr.
        char buf[] = "a1b2c3";
        u.DeleteChars(buf, "123");
        u.DeleteChars(nullptr, "x");   // nullptr → no-op, не падаем
        const bool miscOk = std::string(buf) == "abc"
            && u.FormatStr("%s=%d", "n", 5) == "n=5";

        // 8. ReadChars: append (не очищает), пустые токены сохраняются,
        //    хвост без ';' теряется.
        std::vector<std::string> keys{"старый"}, vals;
        char kv[] = "k1:v1;k2:v2;k3:потеряется";
        u.ReadChars(kv, keys, vals);
        const bool rcOk = keys == std::vector<std::string>{"старый", "k1", "k2", "k3"}
            && vals == std::vector<std::string>{"v1", "v2"};   // третий value потерян

        // 9. SearchStr — группа между left/right.
        const bool srchOk = u.SearchStr("<a>text</a>", "<a>", "</a>") == "text"
            && u.SearchStr("нет", "<a>", "</a>").empty();

        // 10. IsChineseChar — многобайтные символы; выход за границы → false.
        const std::string cn = "ф";   // UTF-8: 2 байта, оба со старшим битом
        const bool cnOk = u.IsChineseChar(cn, 0) && u.IsChineseChar(cn, 1)
            && !u.IsChineseChar("a", 0) && !u.IsChineseChar(cn, -1)
            && !u.IsChineseChar(cn, 99) && !u.IsChineseChar("", 0);

        qInfo() << "split:" << splitOk << "split2:" << split2Ok << "конверсии:" << convOk
                << "тримминг:" << trimOk << "края:" << edgeOk;
        qInfo() << "замены:" << replOk << "прочее:" << miscOk << "ReadChars:" << rcOk
                << "SearchStr:" << srchOk << "многобайтные:" << cnOk;

        const bool ok = splitOk && split2Ok && convOk && trimOk && edgeOk && replOk
            && miscOk && rcOk && srchOk && cnOk;
        qInfo() << (ok ? "strutil: PASS" : "strutil: FAIL");
        return ok ? 0 : 33;
    }

    // Self-test загрузчика шаблонов отчёта (KTemplateCfg, ветка FullTemplate).
    if (screen == "templatecfg") {
        KTemplateCfg cfg;
        cfg.Check("", "");            // задаёт каталоги RO/user
        cfg.LoadCache();              // реф. — заглушка

        // 1. Каталог RO-ветки: 5 реальных шаблонов прошивки.
        std::vector<std::string> libs = cfg.GetLibTemplateFiles();
        std::sort(libs.begin(), libs.end());
        const bool filesOk = libs.size() == 5 && libs[0] == "NP-1x4" && libs[1] == "NP-2x2"
            && libs[2] == "NP-2x3" && libs[3] == "NP-4x1" && libs[4] == "NP-nx3";
        const bool nameOk = KTemplateCfg::GetTemplateFileName("NP-1x4") == "Template(NP-1x4).xml";

        // 2. Разбор реального Template(NP-1x4).xml: user-ветки нет → фолбэк на RO.
        KReportTemplateDataNew data;
        const int ret = cfg.GetTemplateCfg("NP-1x4", data);
        const bool cfgOk = data.m_mapConfigs.size() == 10
            && data.m_mapConfigs["CalcApp"] == "NP-1x4"
            && data.m_mapConfigs["ReportName"] == "NP-1x4 Report"
            && data.m_mapConfigs["FontSize"] == "16";

        // Дерево Content: рекурсия + путь m_strID = parentPath + "/" + Name.
        const KReportTemplateItem &head = data.m_lstItems.front();
        const bool treeOk = !data.m_lstItems.empty() && head.m_strName == "RT_HEADER"
            && head.m_strID == "/RT_HEADER" && head.m_strType == "RT_TABLE_BLOCK"
            && head.m_strColumn == "1" && !head.m_lstSubItems.empty()
            && head.m_lstSubItems.front().m_strName == "RT_HOSPITAL_TOP"
            && head.m_lstSubItems.front().m_strID == "/RT_HEADER/RT_HOSPITAL_TOP";
        // Вложенность 3-го уровня + DataSrc как строка "<источник>,<поле>".
        const KReportTemplateItem &logo = head.m_lstSubItems.front().m_lstSubItems.front();
        const bool deepOk = logo.m_strName == "RT_HOSPITAL_LOGO"
            && logo.m_strType == "RT_IMAGE_BLOCK"
            && logo.m_strDataSrc == "RT_DATASOURCE_PERIPHERAL,RT_HOSPITAL_LOGO"
            && logo.m_strID == "/RT_HEADER/RT_HOSPITAL_TOP/RT_HOSPITAL_LOGO"
            && logo.m_lstSubItems.empty();

        // Все узлы дерева (рекурсивный подсчёт) — сверка с числом <Item> в Content.
        std::function<int(const std::list<KReportTemplateItem> &)> countTree =
            [&](const std::list<KReportTemplateItem> &l) {
                int n = 0;
                for (const KReportTemplateItem &i : l) { n += 1 + countTree(i.m_lstSubItems); }
                return n;
            };
        // Сверено независимым разбором Template(NP-1x4).xml: <Content> = 6 корневых
        // Item + вложенные = 59; <TemplateConfig> = 10; <ItemConfig> = 34.
        const int nodes = countTree(data.m_lstItems);
        const bool countOk = nodes == 59 && data.m_lstItems.size() == 6
            && data.m_mapItemConfigs.size() == 34;

        // 3. ItemConfig: атрибуты НЕ типизированы — все лежат строками в generic-map
        //    (включая сам Name); в одной map и пути, и именованные стили.
        const KReportTemplateItemConfig &glob = data.m_mapItemConfigs["/"];
        const bool icfgOk = glob.m_mapAttrs.at("SplitLineType") == "Horizontal"
            && glob.m_mapAttrs.at("SplitLineWidth") == "2"        // строка, не int
            && glob.m_mapAttrs.count("Name") == 1                  // Name тоже в map
            && !glob.m_bUserDefine
            && data.m_mapItemConfigs["/RT_ADDITION"].m_mapAttrs.at("Section") == "Footer"
            && data.m_mapItemConfigs.count("FirstTitle") == 1;     // стиль без '/'
        const KReportTemplateItemConfig &style = data.m_mapItemConfigs["FirstTitle"];
        const bool styleOk = style.m_mapAttrs.count("Size") == 1 && style.m_strName == "FirstTitle";

        // 4. Кэш: второй запрос идёт из памяти; GetSubTemplateData — только кэш.
        KReportTemplateDataNew again;
        const bool cacheOk = cfg.GetTemplateCfg("NP-1x4", again) == 1
            && again.m_mapConfigs["CalcApp"] == "NP-1x4"
            && cfg.GetSubTemplateData("NP-1x4", again) == 1
            && cfg.GetSubTemplateData("NP-2x2", again) != 1;   // не в кэше → не с диска

        // 5. Roundtrip: сохранить в user-ветку и перечитать (в tmp ENDO_ROOT).
        QTemporaryDir tmp;
        const std::string out = QDir(tmp.path()).absoluteFilePath("Template(NP-1x4).xml")
                                    .toStdString();
        const bool saveOk = KTemplateCfg::SaveTemplateFile(out, data) == 1;
        KReportTemplateDataNew back;
        const bool rtOk = KTemplateCfg::ParseTemplateFile(out, back) == 1
            && back.m_mapConfigs == data.m_mapConfigs
            && countTree(back.m_lstItems) == nodes
            && back.m_lstItems.front().m_strID == "/RT_HEADER"
            && back.m_mapItemConfigs.size() == data.m_mapItemConfigs.size()
            && back.m_mapItemConfigs["/"].m_mapAttrs == glob.m_mapAttrs;

        // 6. Все 5 шаблонов прошивки разбираются.
        int parsed = 0;
        for (const std::string &n : libs) {
            KReportTemplateDataNew d;
            if (cfg.GetTemplateCfg(n, d) == 1 && !d.m_lstItems.empty()) ++parsed;
        }

        qInfo() << "шаблоны:" << libs.size() << "имя файла:" << nameOk << "| GetTemplateCfg:" << ret
                << "| TemplateConfig:" << cfgOk << "дерево:" << treeOk << "глубина:" << deepOk
                << "узлов Content:" << nodes << countOk;
        qInfo() << "ItemConfig:" << icfgOk << "стиль:" << styleOk << "| кэш:" << cacheOk
                << "| save:" << saveOk << "roundtrip:" << rtOk << "| разобрано 5/5:" << parsed;

        const bool ok = filesOk && nameOk && ret == 1 && cfgOk && treeOk && deepOk && countOk
            && icfgOk && styleOk && cacheOk && saveOk && rtOk && parsed == 5;
        qInfo() << (ok ? "templatecfg: PASS" : "templatecfg: FAIL");
        return ok ? 0 : 32;
    }

    // Self-test фундамента XML-подсистемы (KMeaXMLBase + KEnvConfig).
    if (screen == "meaxml") {
        // Тестовый наследник: реф. контракт — Check/LoadCache чисто виртуальные.
        struct TestCfg : public KMeaXMLBase {
            std::string lib, usr;
            int Check(const std::string &a, const std::string &b) override {
                // 1:1 с KTemplateCfg::Check: lib — от RO-корня (с "mainapp/"),
                // usr — от user-корня (БЕЗ "mainapp/" — асимметрия оригинала).
                lib = KEnvConfig::GetInstance().GetReadOnlyBaseDir()
                    + "/mainapp/patient/report/template/FullTemplate/" + a;
                usr = KEnvConfig::GetInstance().GetUsrDir()
                    + "/patient/report/template/FullTemplate/" + b;
                return 1;
            }
            int LoadCache() override { return 1; }   // реф. — заглушка return 1
        };
        TestCfg cfg;

        // 1. KEnvConfig: RO-корень = syspreset, user-корень = userpreset.
        const std::string ro = KEnvConfig::GetInstance().GetReadOnlyBaseDir();
        const std::string usr = KEnvConfig::GetInstance().GetUsrDir();
        const bool envOk = QString::fromStdString(ro).endsWith("system/presetdata/syspreset")
            && QString::fromStdString(usr).endsWith("system/presetdata/userpreset")
            && QString::fromStdString(KEnvConfig::GetInstance().GetBaseDir()).endsWith("data");

        // 2. Реальный файл прошивки: syspreset/mainapp/.../FullTemplate/Template(NP-1x4).xml
        cfg.Check("Template(NP-1x4).xml", "Template(NP-1x4).xml");
        const bool libExists = KMeaXMLBase::IsFileExist(cfg.lib);
        QDomDocument doc;
        const int loadRet = KMeaXMLBase::LoadXMLFile(cfg.lib, doc);
        const bool parseOk = KMeaXMLBase::ParseXML(cfg.lib) == 1;

        // Схема реф.: <root> с детьми TemplateConfig / Content / ItemConfig.
        const QDomElement root = doc.documentElement();
        QDomElement tcfg, content, icfg;
        const bool schemaOk = cfg.FindByName(root, "TemplateConfig", tcfg)
            && cfg.FindByName(root, "Content", content)
            && cfg.FindByName(root, "ItemConfig", icfg)
            && !cfg.FindByName(root, "NoSuchNode", tcfg);

        // 3. Коды ошибок: отсутствующий файл и битый XML → -40 (реф. — не отдельный код).
        const int missRet = KMeaXMLBase::LoadXMLFile("/no/such/file.xml", doc);
        QTemporaryDir tmp;
        const QString badPath = QDir(tmp.path()).absoluteFilePath("bad.xml");
        { QFile b(badPath); b.open(QIODevice::WriteOnly); b.write("<root><unclosed>"); b.close(); }
        const int badRet = KMeaXMLBase::ParseXML(badPath.toStdString());
        const bool errOk = missRet == -40 && badRet == -40
            && !KMeaXMLBase::IsFileExist("/no/such/file.xml");

        // 4. GetModuleVersion == -1 → CheckVersion сводится к загрузке (реф.).
        const bool verOk = cfg.GetModuleVersion() == -1
            && cfg.CheckVersion(cfg.lib) == 1
            && cfg.CheckVersion("/no/such/file.xml") == -40
            && cfg.IsValid();

        // 5. Хелперы обхода на своём документе: whitespace-only текст игнорируется
        //    (у реф. pugi таких узлов нет), CDATA не считается данными.
        QDomDocument d2;
        d2.setContent(QString("<Root>\n  <A x=\"1\">text</A>\n  <A x=\"2\"/>\n"
                              "  <B>\n  </B>\n  <C><![CDATA[cdata]]></C>\n</Root>"));
        const QDomElement r2 = d2.documentElement();
        QDomElement found;
        const bool helpOk = cfg.FindByProperty(r2, "A", "x", "2", found)
            && found.attribute("x") == "2"
            && cfg.FindByValue(r2, "A", "text", found) && found.attribute("x") == "1"
            && !cfg.FindByValue(r2, "A", "нет", found);
        QDomElement bEl, cEl;
        cfg.FindByName(r2, "B", bEl);
        cfg.FindByName(r2, "C", cEl);
        const bool wsOk = cfg.FindDataNode(bEl).isNull()    // только пробелы → нет данных
            && cfg.FindDataNode(cEl).isNull();             // CDATA данными не считается

        // 6. SetNodeValue/SetNodeProperty: создание и перезапись.
        cfg.SetNodeValue(bEl, "новое");
        cfg.SetNodeProperty(bEl, "attr", "v");
        QDomElement aEl;
        cfg.FindByName(r2, "A", aEl);
        cfg.SetNodeValue(aEl, "изменено");
        const bool setOk = !cfg.FindDataNode(bEl).isNull()
            && cfg.FindDataNode(bEl).toText().data() == "новое"
            && bEl.attribute("attr") == "v"
            && cfg.FindDataNode(aEl).toText().data() == "изменено";

        // 7. ReplaceUserByLib: каталог создаётся, user получает копию, старый → .bak.
        const QString uDir = QDir(tmp.path()).absoluteFilePath("deep/user");
        const QString uFile = QDir(uDir).absoluteFilePath("Template(NP-1x4).xml");
        { QDir().mkpath(uDir); QFile o(uFile); o.open(QIODevice::WriteOnly); o.write("old"); o.close(); }
        const int repRet = KMeaXMLBase::ReplaceUserByLib(cfg.lib, uFile.toStdString());
        const bool repOk = repRet == 1 && QFile::exists(uFile) && QFile::exists(uFile + ".bak")
            && QFileInfo(uFile).size() == QFileInfo(QString::fromStdString(cfg.lib)).size()
            && KMeaXMLBase::ParseXML(uFile.toStdString()) == 1;
        // Тот же вызов, когда user-файла нет (rename провалится — реф. игнорирует).
        const QString uNew = QDir(tmp.path()).absoluteFilePath("fresh/dir/t.xml");
        const bool freshOk = KMeaXMLBase::ReplaceUserByLib(cfg.lib, uNew.toStdString()) == 1
            && QFile::exists(uNew);

        qInfo() << "KEnvConfig:" << envOk << "| lib на месте:" << libExists
                << "load:" << loadRet << "parse:" << parseOk << "| схема root:" << schemaOk;
        qInfo() << "коды ошибок:" << errOk << "| версия:" << verOk << "| поиск:" << helpOk
                << "| whitespace/CDATA:" << wsOk << "| set:" << setOk
                << "| ReplaceUserByLib:" << repOk << "новый:" << freshOk;

        const bool ok = envOk && libExists && loadRet == 1 && parseOk && schemaOk && errOk
            && verOk && helpOk && wsOk && setOk && repOk && freshOk;
        qInfo() << (ok ? "meaxml: PASS" : "meaxml: FAIL");
        return ok ? 0 : 31;
    }

    // Self-test конфигов списков пациентов/worklist (фасады над KConfig).
    // Пишет файлы в <root>/data/protected → брать временный ENDO_ROOT!
    if (screen == "listsetup") {
        KPatientListConfigSetupHandler *p = KPatientListConfigSetupHandler::GetInstance();
        KWorkListConfigSetupHandler    *w = KWorkListConfigSetupHandler::GetInstance();
        const QString pIni = QDir(KSystem::ProtectedPath()).absoluteFilePath("patientsetup.ini");
        const QString wIni = QDir(KSystem::ProtectedPath()).absoluteFilePath("worklistsetup.ini");
        // Реф. ctor создаёт пустой файл, если его нет.
        const bool created = QFile::exists(pIni) && QFile::exists(wIni);

        // 1. Дефолты: у Patient ВСЕ bool-колонки true (в отличие от KExamListConfigHandler),
        //    заголовки self-define полей — "".
        const bool pDefOk = p->IsShowPatietID() && p->IsShowApplicant() && p->IsShowApplicantDate()
            && p->IsShowBirthday() && p->IsShowTelephone() && p->IsShowSickbedNum()
            && p->IsShowRegisterNumer() && p->IsShowSelfDefineField1() && p->IsShowSelfDefineField2()
            && p->GetSelfDefineField1Title().empty() && p->GetSelfDefineField2Title().empty();
        // Worklist: bool → true, строки → "", Equipment → 0.
        const bool wDefOk = w->IsShowPatientID() && w->IsShowPatientName()
            && w->IsShowRegisterNumber() && w->IsShowPlantime() && w->IsShowInspectEquipment()
            && w->GetShowPatientID().empty() && w->GetShowPatientName().empty()
            && w->GetShowRegisterNumber().empty() && w->GetShowInspectEquipment() == 0;

        // 2. GetColumnIsShow — ровно 7 записей, имена колонок ≠ ключам .ini.
        p->SetIsShowTelephone(false);
        std::map<std::string, int> cols;
        p->GetColumnIsShow(cols);
        const bool colsOk = cols.size() == 7 && cols["PatientID"] == 1
            && cols["TelephoneNumber"] == 0 && cols["SickBedId"] == 1
            && cols.count("PatientBirthday") == 1 && cols.count("RegisterNumber") == 1
            && cols.count("Applicants") == 1 && cols.count("ApplicantDate") == 1
            && cols.count("userdefined1") == 0;   // self-define полей тут нет

        // 3. Запись без SaveConfig на диск не летит (KConfig правит только память).
        p->SetIsShowBirthday(false);
        p->SetIsShowSelfDefineField2(true);       // явный true → в файле "True"
        p->SetSelfDefineField1Title("Отделение");
        const bool notYet = KConfig(pIni.toStdString()).ReadBool("ShowOnMainUi", "birthday", true);
        p->SaveConfig();

        w->SetIsShowPatientID(false);
        w->SetShowPatientID("P-42");
        w->SetShowInspectEquipment(3);
        const QDate d1(2026, 7, 1), d2(2026, 7, 15);
        w->SetShowPlantime(d1, d2);
        w->SaveConfig();

        // 4. Персист: перечитать файлы независимым KConfig.
        KConfig pc(pIni.toStdString());
        const bool pSaveOk = !pc.ReadBool("ShowOnMainUi", "birthday", true)
            && !pc.ReadBool("ShowOnMainUi", "telephone", true)
            && pc.ReadBool("ShowOnMainUi", "userdefined2", false)
            && pc.ReadString("ShowOnMainUi", "userdefinedtitle1", "") == "Отделение"
            // Дефолты НЕ персистятся: в файле только явно записанные ключи —
            // нетронутый patientid отсутствует и читается как дефолт.
            && !pc.HasItem("ShowOnMainUi", "patientid")
            && p->IsShowPatietID();
        KConfig wc(wIni.toStdString());
        const bool wSaveOk = !wc.ReadBool("ShowOnMainUi", "IsPatientidOn", true)
            && wc.ReadString("ShowOnMainUi", "patientid", "") == "P-42"
            && wc.ReadInt("ShowOnMainUi", "Equipment", 0) == 3
            && wc.ReadString("ShowOnMainUi", "plantimestart", "") == "2026-07-01"
            && wc.ReadString("ShowOnMainUi", "plantimeend", "") == "2026-07-15";

        // 5. Формат на диске — движок KConfig: секция [ShowOnMainUi], bool как True/False.
        QFile pf(pIni);
        pf.open(QIODevice::ReadOnly);
        const QString ptext = QString::fromUtf8(pf.readAll());
        pf.close();
        const bool fmtOk = ptext.contains("[ShowOnMainUi]")
            && ptext.contains("birthday=False") && ptext.contains("userdefined2=True")
            && !ptext.contains("patientid");   // нетронутый ключ в файл не попадает

        // 6. Plantime — обратный разбор в QDate ("yyyy-MM-dd").
        QDate g1, g2;
        w->GetShowPlantime(g1, g2);
        const bool dateOk = g1 == d1 && g2 == d2;

        qInfo() << "файлы созданы:" << created << "| дефолты patient:" << pDefOk
                << "worklist:" << wDefOk << "| колонки:" << colsOk
                << "| до SaveConfig на диске старое:" << notYet;
        qInfo() << "персист patient:" << pSaveOk << "worklist:" << wSaveOk
                << "| формат True/False:" << fmtOk << "| даты:" << dateOk;

        const bool ok = created && pDefOk && wDefOk && colsOk && notYet && pSaveOk
            && wSaveOk && fmtOk && dateOk;
        qInfo() << (ok ? "listsetup: PASS" : "listsetup: FAIL");
        return ok ? 0 : 30;
    }

    // Self-test INI-движка ядра (KConfig) — семантика 1:1 с дизасмом.
    if (screen == "kconfig") {
        QTemporaryDir tmp;
        const std::string path = QDir(tmp.path()).absoluteFilePath("t.ini").toStdString();

        // 1. Несуществующий файл: ctor молчит, всё читается как дефолт.
        {
            KConfig c(path);
            const bool missOk = !c.IsFileExisted()
                && c.ReadBool("No", "Key", true) && !c.ReadBool("No", "Key", false)
                && c.ReadInt("No", "Key", 42) == 42
                && c.ReadString("No", "Key", "def") == "def"
                && !c.HasItem("No", "Key")
                && c.GetKeysFromSection("No").empty();
            if (!missOk) { qInfo() << "kconfig: FAIL (дефолты)"; return 29; }
        }

        // 2. Запись + Save: bool → "True"/"False" (НЕ 1/0 и не true/false),
        //    числа — ostringstream (6 знач. цифр), пробелов вокруг '=' нет.
        {
            KConfig c(path);
            c.WriteData("Sec", "flagT", true);
            c.WriteData("Sec", "flagF", false);
            c.WriteData("Sec", "num", 7);
            c.WriteData("Sec", "pi", 3.14159265);      // → 3.14159 (6 знач. цифр)
            c.WriteData("Sec", "big", 1234567.0);      // → 1.23457e+06
            c.WriteData("Sec", "str", "hello");
            c.WriteData("Alpha", "k", "v");            // проверка лексикогр. порядка
            const bool notYet = !c.IsFileExisted();    // WriteData на диск не пишет
            if (!c.Save()) { qInfo() << "kconfig: FAIL (Save)"; return 29; }
            if (!notYet)   { qInfo() << "kconfig: FAIL (WriteData писал на диск)"; return 29; }
        }

        QFile f(QString::fromStdString(path));
        f.open(QIODevice::ReadOnly);
        const QString text = QString::fromUtf8(f.readAll());
        f.close();

        // Формат файла: секции лексикографически (Alpha < Sec), пустая строка
        // после каждой секции (включая последнюю), 'key=value' без пробелов.
        const QString expect =
            "[Alpha]\nk=v\n\n"
            "[Sec]\nbig=1.23457e+06\nflagF=False\nflagT=True\nnum=7\npi=3.14159\nstr=hello\n\n";
        const bool fmtOk = text == expect;
        if (!fmtOk) qInfo() << "формат разошёлся:\n" << text.toUtf8().constData();

        // 3. Roundtrip + парсинг bool: false ТОЛЬКО для FALSE/F/NO/N/0 (регистронезав.),
        //    всё прочее — включая "" и "2" — true.
        {
            KConfig c(path);
            const bool rtOk = c.ReadBool("Sec", "flagT", false) && !c.ReadBool("Sec", "flagF", true)
                && c.ReadInt("Sec", "num", 0) == 7
                && c.ReadString("Sec", "str", "") == "hello"
                && qAbs(c.ReadDouble("Sec", "pi", 0.0) - 3.14159) < 1e-9;
            const std::vector<std::string> keys = c.GetKeysFromSection("Sec");
            const bool keysOk = keys.size() == 6 && keys[0] == "big" && keys[5] == "str";
            if (!rtOk || !keysOk) { qInfo() << "kconfig: FAIL (roundtrip)"; return 29; }
        }

        // 4. Парсинг: '#'-комментарий (вместе с \n), trim, дубликат ключа (побеждает
        //    последний), текст до первой '[' игнорируется, ';' и '//' — НЕ комментарии.
        const std::string p2 = QDir(tmp.path()).absoluteFilePath("p.ini").toStdString();
        {
            QFile g(QString::fromStdString(p2));
            g.open(QIODevice::WriteOnly);
            g.write("мусор до секции\n"       // текст до первой '[' игнорируется
                    "[S]\n"
                    "  a  =  1  \n"          // trim обоих концов ключа и значения
                    "b=first\n"
                    "b=second\n"             // дубликат → побеждает последний
                    "f=;notcomment\n"        // ';' — НЕ комментарий (только '#')
                    "# c=commented\n"        // строка-комментарий целиком
                    "d=x#y\n"                // '#' в значении съедается вместе с \n…
                    "e=tail\n");             // …→ эта строка склеивается с 'd'
            g.close();

            KConfig c(p2);
            const bool parseOk = c.ReadString("S", "a", "") == "1"
                && c.ReadString("S", "b", "") == "second"
                && c.ReadString("S", "f", "") == ";notcomment"
                && !c.HasItem("S", "c")
                // Особенность реф.: EraseComment срезает и '\n', поэтому 'd' поглощает
                // следующую строку, а ключа 'e' не возникает вовсе.
                && c.ReadString("S", "d", "") == "xe=tail"
                && !c.HasItem("S", "e")
                && c.ReadBool("S", "nokey", true);           // отсутствует → дефолт
            // Регистрозависимость секций/ключей (std::map).
            const bool caseOk = !c.HasItem("s", "a") && !c.HasItem("S", "A");
            // FromStringToBool: "" и "2" → true; "no"/"F" → false.
            KConfig d(p2);
            d.WriteData("B", "empty", "");
            d.WriteData("B", "two", "2");
            d.WriteData("B", "no", "no");
            d.WriteData("B", "eff", "F");
            const bool boolOk = d.ReadBool("B", "empty", false) && d.ReadBool("B", "two", false)
                && !d.ReadBool("B", "no", true) && !d.ReadBool("B", "eff", true);
            if (!parseOk || !caseOk || !boolOk) {
                qInfo() << "разбор:" << parseOk << "регистр:" << caseOk << "bool:" << boolOk;
                qInfo() << "kconfig: FAIL (парсинг)";
                return 29;
            }
        }

        // 5. ReadDataWithoutDefaultValue: при отсутствии ключа out НЕ трогается.
        {
            KConfig c(path);
            int keep = 555;
            const bool miss = c.ReadDataWithoutDefaultValue("Sec", "nokey", keep);
            int got = 0;
            const bool hit = c.ReadDataWithoutDefaultValue("Sec", "num", got);
            if (miss || keep != 555 || !hit || got != 7) {
                qInfo() << "kconfig: FAIL (ReadDataWithoutDefaultValue)";
                return 29;
            }
        }

        qInfo() << "формат ок:" << fmtOk;
        qInfo() << (fmtOk ? "kconfig: PASS" : "kconfig: FAIL");
        return fmtOk ? 0 : 29;
    }

    // Self-test конфига DICOM-сервисов (link-dicom.json, KSysDICOMData/KDICOMConf).
    if (screen == "dcmconf") {
        // Пишет файл → берём временный ENDO_ROOT.
        KSysDICOMData data;   // реф. ctor зовёт Load(); файла нет → дефолты
        const QString path = data.ConfigFilePath();
        const bool inUserSet = path.endsWith("data/setdata/userset/link-dicom.json");

        // 1. Дефолты Clear() — 1:1 с реф. (Local 104/60/"AE", списки пусты).
        const KDICOMLocalConf def = data.GetDICOMLocalConf();
        const bool defOk = def.port == 104 && def.connectTimeout == 60 && def.ae == "AE"
            && !def.uploadPDFReport && !def.snapSyncUploadImage
            && data.GetDICOMStorageConfList().isEmpty()
            && data.GetDICOMWorkListConfList().isEmpty()
            && data.GetDICOMMPPSConfList().isEmpty()
            && data.GetDICOMCMTStorageConfList().isEmpty();

        // 2. Собираем конфиг и сохраняем через Save(conf) — сериализация из объекта.
        KDICOMConf conf;
        KDICOMLocalConf local;
        local.port = 11112; local.connectTimeout = 30; local.name = "X2600";
        local.ae = "ENDO_AE"; local.uploadPDFReport = true; local.snapSyncUploadImage = true;
        conf.SetDICOMLocalConf(local);

        KDICOMStorageConf st1;                       // включённый + вложенный commit
        st1.isEnable = true; st1.port = 104; st1.ip = "192.168.1.10";
        st1.name = "PACS"; st1.ae = "STORESCP"; st1.addTime = "2026-07-15 10:00:00";
        st1.commitStorage.isEnable = true; st1.commitStorage.port = 105;
        st1.commitStorage.ip = "192.168.1.11"; st1.commitStorage.ae = "COMMITSCP";
        KDICOMStorageConf st2;                       // выключенный → фильтр onlyEnable
        st2.isEnable = false; st2.port = 106; st2.ip = "192.168.1.12"; st2.name = "PACS2";
        conf.SetDICOMStorageConfList({st1, st2});

        KDICOMWorkListConf wl;
        wl.isEnable = true; wl.port = 107; wl.ip = "192.168.1.20"; wl.ae = "WLSCP";
        wl.maxDownloadNum = 50; wl.reqProcDesc = 1;  // реф.: ReqProcDesc — int
        conf.SetDICOMWorkListConfList({wl});

        KDICOMMPPSConf mp;
        mp.isEnable = true; mp.port = 108; mp.ip = "192.168.1.30"; mp.ae = "MPPSSCP";
        conf.SetDICOMMPPSConfList({mp});

        KDICOMCommitStorageConf cm;
        cm.isEnable = true; cm.port = 109; cm.ip = "192.168.1.40"; cm.ae = "CMTSCP";
        conf.SetDICOMCMTStorageConfList({cm});

        data.Save(conf);
        const bool written = QFile::exists(path);

        // 3. Roundtrip: новый объект читает файл в ctor → сверка с исходным conf.
        KSysDICOMData re;
        const bool localOk = re.GetDICOMLocalConf() == local;
        const QList<KDICOMStorageConf> rst = re.GetDICOMStorageConfList();
        const bool stOk = rst.size() == 2 && rst[0] == st1 && rst[1] == st2
            && rst[0].commitStorage.port == 105 && rst[0].commitStorage.ae == "COMMITSCP";
        const QList<KDICOMWorkListConf> rwl = re.GetDICOMWorkListConfList();
        const bool wlOk = rwl.size() == 1 && rwl[0] == wl && rwl[0].reqProcDesc == 1
            && rwl[0].maxDownloadNum == 50;
        const bool mpOk = re.GetDICOMMPPSConfList().size() == 1
            && re.GetDICOMMPPSConfList()[0] == mp;
        const bool cmOk = re.GetDICOMCMTStorageConfList().size() == 1
            && re.GetDICOMCMTStorageConfList()[0] == cm;

        // 4. Фильтр onlyEnable (реф. ldrb IsEnable/cbz).
        const bool filtOk = re.GetDICOMStorageConfList(true).size() == 1
            && re.GetDICOMStorageConfList(true)[0].name == "PACS"
            && re.GetDICOMStorageConfList(false).size() == 2;

        // 5. Схема документа: корень Local + 4 массива, вложенный CommitStorage в Storage.
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        const bool schemaOk = root.value("Local").isObject()
            && root.value("MPPS").isArray() && root.value("WorkList").isArray()
            && root.value("Storage").isArray() && root.value("CommitStorage").isArray()
            && root.value("Storage").toArray()[0].toObject().value("CommitStorage").isObject()
            && root.value("Local").toObject().value("Port").toInt() == 11112
            && root.value("WorkList").toArray()[0].toObject().value("ReqProcDesc").isDouble();

        qInfo() << "path:" << path << "в userset:" << inUserSet << "записан:" << written;
        qInfo() << "дефолты:" << defOk << "local:" << localOk << "storage:" << stOk
                << "worklist:" << wlOk << "mpps:" << mpOk << "commit:" << cmOk
                << "фильтр IsEnable:" << filtOk << "схема:" << schemaOk;

        const bool ok = inUserSet && written && defOk && localOk && stOk && wlOk
            && mpOk && cmOk && filtOk && schemaOk;
        qInfo() << (ok ? "dcmconf: PASS" : "dcmconf: FAIL");
        return ok ? 0 : 28;
    }

    // Self-test конфигурации брендов/стилей (stylelist.ini).
    if (screen == "style") {
        KStyleConfig &st = KStyleConfig::GetInstance();
        const bool loaded = st.Load();
        qInfo() << "бренды:" << st.GetStyleList();
        qInfo() << "текущий (по умолч.):" << st.GetCurrentStyle();
        st.SetCurrentStyle("PyCkeun");   // РуСкейн (X-2600)
        const QString path = st.GetStylePath("X-2600", "PyCkeun");
        const bool pathExists = QDir(path).exists();
        qInfo() << "PyCkeun путь:" << path << "существует:" << pathExists;

        const bool ok = loaded && st.GetStyleList().size() == 4 &&
                        st.GetStyleList().contains("PyCkeun") &&
                        st.GetStyleList().first() == "SonoScapeCN" &&
                        st.IsStyleValid("SonoScapeHK") && !st.IsStyleValid("NoSuchBrand") &&
                        st.GetCurrentStyle() == "PyCkeun" && pathExists;
        qInfo() << (ok ? "style: PASS" : "style: FAIL");
        return ok ? 0 : 22;
    }

    // Self-test scope-info (реф. KEncStyle::getScopeDefault{Round,Octangle}Cut) —
    // <style>/<brand>/scope/video.ini, секции hex-кода имени скопа + [Default] фолбэк.
    if (screen == "scopecut") {
        KStyleConfig &st = KStyleConfig::GetInstance();
        const QString series = "X-2500", brand = "SonoScape";

        // Hex-кодировка имени (ConvertSrc2Enc).
        const bool encOk = KStyleConfig::EncodeScopeName("EC-X20") == "45432d583230" &&
                           KStyleConfig::EncodeScopeName("EB-X20") == "45422d583230";

        // Per-scope значения (EB-X20 отличается от Default).
        const int rEB = st.GetScopeDefaultCut(series, brand, "EB-X20", 0);  // round 458
        const int oEB = st.GetScopeDefaultCut(series, brand, "EB-X20", 1);  // oct 3932220
        // Несуществующий скоп → фолбэк [Default] (round 746 / oct 5898330).
        const int rDef = st.GetScopeDefaultCut(series, brand, "NO-SUCH", 0);
        const int oDef = st.GetScopeDefaultCut(series, brand, "NO-SUCH", 1);
        const bool cutOk = rEB == 458 && oEB == 3932220 && rDef == 746 && oDef == 5898330;

        // Полная запись + распаковка octangleCut в параметры геометрии (p2<<16|p3).
        const auto si = st.GetScopeInfo(series, brand, "EB-X20");
        const int p2 = si.octangleCut >> 16, p3 = si.octangleCut & 0xffff;
        const bool infoOk = si.valid && si.sensorType == "OH01A" &&
                            si.shapeType == "OCTANGLE_AND_ROUND" &&
                            si.videoSize == QRect(0, 0, 864, 1056) &&
                            p2 == 60 && p3 == 60;   // 3932220 = 0x3C003C

        // Доп. поля video.ini (рег. KEncStyle::getScopeRotateType/GetScopeZoomRatio/
        // getScopeParaDefault/getIsDefaultMatch — off-device по имени скопа).
        const bool extraOk = si.rotateType == 0 && si.workLength == 600 &&
                             si.defaultMatch == true &&
                             qAbs(si.zoomRatio - 1.14f) < 1e-4f;

        qInfo() << "enc EC-X20=" << KStyleConfig::EncodeScopeName("EC-X20")
                << "| EB round=" << rEB << "oct=" << oEB
                << "| Default round=" << rDef << "oct=" << oDef;
        qInfo() << "info valid=" << si.valid << "sensor=" << si.sensorType
                << "shape=" << si.shapeType << "video=" << si.videoSize
                << "oct p2/p3=" << p2 << "/" << p3;
        qInfo() << "extra rotate=" << si.rotateType << "zoom=" << si.zoomRatio
                << "workLen=" << si.workLength << "defMatch=" << si.defaultMatch;
        const bool ok = encOk && cutOk && infoOk && extraOk;
        qInfo() << (ok ? "scopecut: PASS" : "scopecut: FAIL");
        return ok ? 0 : 22;
    }

    // Self-test конвертеров фиксированной точки KVideoProxy (реф. 1:1 с дизасмом).
    if (screen == "fxpt") {
        KVideoProxy vp;

        // Float2FixedPointNumber(f, a, b): Q(a).(b), scale=2^b, потолок 2^(a+b)−1.
        const bool f2fOk =
            vp.Float2FixedPointNumber(1.5f, 2, 8) == 384 &&   // 1.5·256, потолок 1023
            vp.Float2FixedPointNumber(-0.5f, 2, 8) == -128 && // −(0.5·256)
            vp.Float2FixedPointNumber(10.0f, 2, 8) == 1023 && // насыщение (2560→1023)
            vp.Float2FixedPointNumber(0.0f, 2, 8) == 0;

        // FixedPointNumber2Float(x) = (x&0xfff)/4096.
        auto near = [](double a, double b){ return qAbs(a - b) < 1e-9; };
        const bool p2fOk =
            near(vp.FixedPointNumber2Float(2048), 0.5) &&
            near(vp.FixedPointNumber2Float(1024), 0.25) &&
            near(vp.FixedPointNumber2Float(4095), 4095.0/4096.0) &&
            near(vp.FixedPointNumber2Float(0), 0.0) &&
            near(vp.FixedPointNumber2Float(0x1000), 0.0);   // старшие биты игнорятся

        // IncreaseValue/DecreaseValue — клампы.
        int v = 5;  vp.IncreaseValue(v, 10);  const bool inc1 = (v == 6);
        v = 10;     vp.IncreaseValue(v, 10);  const bool inc2 = (v == 10);   // потолок
        v = 5;      vp.DecreaseValue(v);      const bool dec1 = (v == 4);
        v = 0;      vp.DecreaseValue(v);      const bool dec2 = (v == 0);    // пол
        const bool clampOk = inc1 && inc2 && dec1 && dec2;

        qInfo() << "Float2Fixed 1.5/Q2.8=" << vp.Float2FixedPointNumber(1.5f, 2, 8)
                << "| Fixed2Float 2048=" << vp.FixedPointNumber2Float(2048)
                << "| f2f" << f2fOk << "p2f" << p2fOk << "clamp" << clampOk;

        // Тонкие обёртки-оркестраторы → KPlControl (проверка через trace).
        KPlControl pl;  pl.EnableTrace(true);  vp.AttachPl(&pl);
        pl.ClearTrace();
        vp.SetColorRLevel(0x11);       // → REG_TONE_R 0xa1870004
        vp.SetColorBLevel(0x22);       // → REG_TONE_B 0xa1870008
        vp.SetColroCLevel(0x33);       // → REG_TONE_C 0xa1870000
        vp.SetCutPara(0xa, 0xb);       // → REG_CUT_PARA 0xa1860018 = b|(a<<16)
        vp.SetRealtimeVideoState(1);   // → 0xa0080024
        vp.SetVideoDisPlay(2);         // → 0xa0080028
        const auto &tw = pl.Trace();
        const bool wrapOk = tw.size() == 6 &&
            tw[0].first == 0xa1870004 && tw[0].second == 0x11 &&
            tw[1].first == 0xa1870008 && tw[1].second == 0x22 &&
            tw[2].first == 0xa1870000 && tw[2].second == 0x33 &&
            tw[3].first == 0xa1860018 && tw[3].second == (0xbu | (0xau << 16)) &&
            tw[4].first == 0xa0080024 && tw[4].second == 1 &&
            tw[5].first == 0xa0080028 && tw[5].second == 2;
        qInfo() << "wrappers writes:" << tw.size() << "(exp 6)" << (wrapOk ? "OK" : "MISMATCH");

        // Зеркало/поворот — командные последовательности в REG_CAM_CMD (0xa0048010).
        pl.ClearTrace();
        vp.SetHorizontalMirror(1);     // не 4 → ничего
        vp.SetHorizontalMirror(4);     // 2 команды
        vp.SetRotateType(0);           // 0 → ничего
        vp.SetRotateType(2);           // 3 команды
        const auto &tm = pl.Trace();
        const bool mrOk = tm.size() == 5 &&
            tm[0].first == 0xa0048010 && tm[0].second == 0x00370141 &&
            tm[1].first == 0xa0048010 && tm[1].second == 0x00500703 &&
            tm[2].first == 0xa0048010 && tm[2].second == 0x003811a8 &&
            tm[3].first == 0xa0048010 && tm[3].second == 0x00381317 &&
            tm[4].first == 0xa0048010 && tm[4].second == 0x003820b0;
        qInfo() << "mirror/rotate writes:" << tm.size() << "(exp 5)" << (mrOk ? "OK" : "MISMATCH");

        // SetMonitorCtrl: REG_MONITOR_CTRL = ((v·100000)<<4)|3.
        pl.ClearTrace();
        vp.SetMonitorCtrl(2);          // ((200000)<<4)|3 = 0x30d403
        const auto &tmc = pl.Trace();
        const bool monOk = tmc.size() == 1 &&
            tmc[0].first == 0xa0060040 && tmc[0].second == (((2u*100000u)<<4)|3u);
        qInfo() << "monitorCtrl:" << QString::number(tmc.isEmpty()?0:tmc[0].second,16)
                << "(exp 30d403)" << (monOk ? "OK" : "MISMATCH");

        // getAecValue: выдержка (мс) → код AEC, формула по режиму тракта AE.
        vp.SetAecAgcRouteMode(0);
        const bool aec0 = vp.getAecValue(10.0f) == 10666;  // t·72000·16/1080
        vp.SetAecAgcRouteMode(1);
        const bool aec1 = vp.getAecValue(10.0f) == 503;    // t·40000/794
        vp.SetAecAgcRouteMode(2);
        const bool aec2 = vp.getAecValue(10.0f) == 922;    // 2307−(t·72000−112)/520
        vp.SetAecAgcRouteMode(3);
        const bool aec3 = vp.getAecValue(10.0f) == 522;    // t·40000/765
        vp.SetAecAgcRouteMode(7);
        const bool aecDef = vp.getAecValue(10.0f) == 10;   // неизвестный режим → 10
        const bool aecOk = aec0 && aec1 && aec2 && aec3 && aecDef;
        qInfo() << "getAecValue(10ms) m0..3:" << 10666 << 503 << 922 << 522
                << (aecOk ? "OK" : "MISMATCH");

        // SetAECValue камера (I2C): команды байтов + keep-alive repeatCnt.
        // Пишет на вызовах 1,2,3 (смена + первые 2 повтора) и 191-м (repeatCnt>0xbc).
        vp.SetAecAgcRouteMode(2);
        pl.ClearTrace();
        for (int i = 0; i < 191; ++i)
            vp.SetAECValue(0x123);
        const auto &ta = pl.Trace();
        const bool aecSeqOk = ta.size() == 16 &&
            ta[0].first == 0xa0048074 && ta[0].second == 0xc23 &&
            ta[1].first == 0xa0048070 && ta[1].second == 0x1014 &&
            ta[2].first == 0xa0048074 && ta[2].second == 0x80000d01 &&
            ta[3].first == 0xa0048070 && ta[3].second == 0x1014;
        qInfo() << "cameraAEC writes over 191 calls:" << ta.size() << "(exp 16)"
                << (aecSeqOk ? "OK" : "MISMATCH");

        // SetAGCValue камера: младший байт (0xa00), старшие 3 бита (0xb00|бит31).
        pl.ClearTrace();
        vp.SetAGCValue(0x7ff);
        const auto &tg = pl.Trace();
        const bool agcOk = tg.size() == 4 &&
            tg[0].first == 0xa0048074 && tg[0].second == 0xaff &&
            tg[2].first == 0xa0048074 && tg[2].second == 0x80000b07;
        qInfo() << "cameraAGC:" << tg.size() << "writes" << (agcOk ? "OK" : "MISMATCH");

        // Эндоскопный тракт: пара (AEC,AGC) одним регистром REG_AEC_AGC.
        vp.SetAecAgcRouteMode(0);
        pl.ClearTrace();
        vp.SetAECAndAGCValue(0x111, 0x22);
        const auto &tp = pl.Trace();
        const bool pairOk = tp.size() == 1 &&
            tp[0].first == 0xa0048020 && tp[0].second == (0x111u | (0x22u << 16));
        qInfo() << "endo AEC+AGC pair:" << (pairOk ? "OK" : "MISMATCH");

        // SetFreezeCalResolution: без layout — ошибка (0 записей); с layout —
        // ScalerIn/Out/Ratio(Q5.8)+VideoLoc из PIP-окна [VIDEO]/IMAGE_PIP.
        pl.ClearTrace();
        vp.SetFreezeCalResolution(1280, 960);   // layout ещё не выбран
        const bool fzErrOk = pl.Trace().isEmpty();
        KDisplayOption::Instance().SelectLayout(QSize(1920, 1080), QSize(1280, 960));
        pl.ClearTrace();
        vp.SetFreezeCalResolution(1280, 960);   // PIP=@Rect(16 80 288 216)
        const auto &tf = pl.Trace();
        // 1280/288 и 960/216 = 4.444 → Q5.8: 4.444·256 = 1137.
        const bool fzOk = fzErrOk && tf.size() == 5 &&
            tf[0].first == 0xa191000c && tf[0].second == (1280u | (960u << 16)) &&
            tf[1].first == 0xa1910010 && tf[1].second == (288u  | (216u << 16)) &&
            tf[2].first == 0xa1910008 && tf[2].second == (1137u | (1137u << 16)) &&
            tf[3].first == 0xa1800024 && tf[3].second == (16u   | (288u << 16)) &&
            tf[4].first == 0xa1800028 && tf[4].second == (80u   | (216u << 16));
        qInfo() << "freezeCalResolution writes:" << tf.size() << "(exp 5)"
                << (fzOk ? "OK" : "MISMATCH");

        // Тонкие обёртки к KPlControl/KVideoParam (реф. Set*Level/Size/AwbCut/CHb).
        pl.ClearTrace();
        vp.SetImgDenoiseLevel(3);      // → REG_DENOISE_LEVEL 0xa1940008 = 3
        vp.SetLensSize(0x10, 0x20);    // → REG_LENS_SIZE 0xa189000c = 0x10|(0x20<<16)
        vp.SetEnhanceSize(1, 2);       // реф. пустой → без записи
        vp.SetAwbCut(0x40, 0x80);      // → REG_AWB_CUT 0xa1840018 = 0x40|(0x80<<16)
        vp.SendCHbLevel(0);            // → REG_CHB_ENABLE 0xa1900008 = 0 (выкл)
        const auto &tt = pl.Trace();
        const bool thinOk = tt.size() == 4 &&
            tt[0].first == 0xa1940008 && tt[0].second == 3 &&
            tt[1].first == 0xa189000c && tt[1].second == (0x10u | (0x20u << 16)) &&
            tt[2].first == 0xa1840018 && tt[2].second == (0x40u | (0x80u << 16)) &&
            tt[3].first == 0xa1900008 && tt[3].second == 0;
        qInfo() << "thin wrappers writes:" << tt.size() << "(exp 4)"
                << (thinOk ? "OK" : "MISMATCH");

        // SetBrightnessEQLevel: 0 → выкл (enable=0); ≠0 → вкл (enable=1) + LUT.
        pl.ClearTrace();
        vp.SetBrightnessEQLevel(0);    // 1 запись: 0xa1950000 = 0
        const int offWrites = pl.Trace().size();
        pl.ClearTrace();
        vp.SetBrightnessEQLevel(2);    // enable=1 (0xa1950000) + LUT-записи
        const auto &tb = pl.Trace();
        const bool beqOk = offWrites == 1 &&
            !tb.isEmpty() && tb[0].first == 0xa1950000 && tb[0].second == 1;
        qInfo() << "brightnessEQ off/on writes:" << offWrites << "/" << tb.size()
                << (beqOk ? "OK" : "MISMATCH");

        // SetContrastLevel: 0xff циклит [0..2], пишет гамма-LUT (перезагрузка).
        vp.SetContrastLevel(0);
        vp.SetContrastLevel(0xff);
        const bool ctrOk = KVideoParam::Instance().ContrastLevel() == 1;  // 0→1 по кругу
        qInfo() << "contrast cycle 0→" << KVideoParam::Instance().ContrastLevel()
                << (ctrOk ? "OK" : "MISMATCH");

        // SetDemoire: тоггл. Из выкл (0) → вкл: SetDemoireEN(0xa18501cc)=1 +
        // image-enh=0; повторный вызов → выкл: EN=0 + восстановление image-enh уровня.
        KVideoParam::Instance().SetDemoire(0);
        pl.ClearTrace();
        vp.SetDemoire();               // → вкл: EN=1, image-enh уровень 0
        const auto &td1 = pl.Trace();
        const bool dm1 = td1.size() == 2 &&
            td1[0].first == 0xa18501cc && td1[0].second == 1 &&
            td1[1].first == 0xa1850058 &&
            KVideoParam::Instance().DemoireStatus() == 1;
        pl.ClearTrace();
        vp.SetDemoire();               // → выкл: EN=0
        const auto &td2 = pl.Trace();
        const bool dm2 = td2.size() == 2 &&
            td2[0].first == 0xa18501cc && td2[0].second == 0 &&
            KVideoParam::Instance().DemoireStatus() == 0;
        const bool demoireOk = dm1 && dm2;
        qInfo() << "demoire toggle on/off:" << td1.size() << td2.size()
                << (demoireOk ? "OK" : "MISMATCH");

        // Dehaze/HDR: тогглы + взаимоисключение (включение одного гасит другой).
        KVideoParam &kvp = KVideoParam::Instance();
        kvp.SetDehaze(0); kvp.SetHDR(0);
        vp.SetDehazeSwitch(0xff);      // 0→1 по кругу
        const bool dh1 = kvp.DehazeStatus() == 1;
        vp.SetHDRStatus(1);            // включить HDR → Dehaze гаснет
        const bool dh2 = kvp.HDRStatus() == 1 && kvp.DehazeStatus() == 0;
        vp.SetDehazeStatus(1);         // включить Dehaze → HDR гаснет
        const bool dh3 = kvp.DehazeStatus() == 1 && kvp.HDRStatus() == 0;
        vp.SetHDRSwitch(0xff);         // 0→1 по кругу → Dehaze гаснет
        const bool dh4 = kvp.HDRStatus() == 1 && kvp.DehazeStatus() == 0;
        vp.SetDehazeSwitch(0xff); vp.SetDehazeSwitch(0xff);  // 0→1→0 (был 0)
        const bool dh5 = kvp.DehazeStatus() == 0;
        const bool dehazeHdrOk = dh1 && dh2 && dh3 && dh4 && dh5;
        qInfo() << "dehaze/HDR mutual-excl:" << dh1 << dh2 << dh3 << dh4 << dh5
                << (dehazeHdrOk ? "OK" : "MISMATCH");

        // SendRBCValue: три тон-гейна пакетом → REG_TONE_R/B/C (0xa1870004/8/0).
        pl.ClearTrace();
        vp.SendRBCValue(0x11, 0x22, 0x33);
        const auto &tr = pl.Trace();
        const bool rbcOk = tr.size() == 3 &&
            tr[0].first == 0xa1870004 && tr[0].second == 0x11 &&
            tr[1].first == 0xa1870008 && tr[1].second == 0x22 &&
            tr[2].first == 0xa1870000 && tr[2].second == 0x33;
        qInfo() << "SendRBCValue writes:" << tr.size() << "(exp 3)" << (rbcOk ? "OK" : "MISMATCH");

        // SwitchCHbStatus вкл: цвет=0, тон=(8,8,8), CHb вкл (0xa1900008=1 + значение).
        KVideoParam::Instance().SetRGain(0x5); KVideoParam::Instance().SetBGain(0x6);
        KVideoParam::Instance().SetSGain(0x7);
        pl.ClearTrace();
        vp.SwitchCHbStatus(1);
        const auto &tc1 = pl.Trace();
        // тон (8,8,8) → 3 записи + CHb вкл (2 записи: enable=1 + значение); цвет-энх — 0 записей (SetColorEnhParam(false)?)
        bool chbTone888 = false, chbOn = false;
        for (const auto &w : tc1) {
            if (w.first == 0xa1870004 && w.second == 8) chbTone888 = true;
            if (w.first == 0xa1900008 && w.second == 1) chbOn = true;
        }
        // выкл: восстановить тон из гейнов (5,6,7) + CHb выкл (0xa1900008=0).
        pl.ClearTrace();
        vp.SwitchCHbStatus(0);
        const auto &tc0 = pl.Trace();
        bool chbRestore = false, chbOff = false;
        for (const auto &w : tc0) {
            if (w.first == 0xa1870004 && w.second == 5) chbRestore = true;
            if (w.first == 0xa1900008 && w.second == 0) chbOff = true;
        }
        const bool chbOk = chbTone888 && chbOn && chbRestore && chbOff;
        qInfo() << "SwitchCHbStatus on/off:" << chbTone888 << chbOn << chbRestore << chbOff
                << (chbOk ? "OK" : "MISMATCH");

        const bool ok = f2fOk && p2fOk && clampOk && wrapOk && mrOk && monOk
                        && aecOk && aecSeqOk && agcOk && pairOk && fzOk
                        && thinOk && beqOk && ctrOk && demoireOk && dehazeHdrOk
                        && rbcOk && chbOk;
        qInfo() << (ok ? "fxpt: PASS" : "fxpt: FAIL");
        return ok ? 0 : 23;
    }

    // Self-test продуктовой конфигурации (project.ini + per-модель product.ini).
    if (screen == "project") {
        const QString sysd = QDir(KSystem::SystemPath()).absoluteFilePath("");
        KProjectSet &ps = KProjectSet::GetInstance();
        const bool pOk = ps.LoadProject(QDir(sysd).absoluteFilePath("display/project.ini"));
        const bool cOk = ps.LoadProductConfig(
            QDir(sysd).absoluteFilePath("display/X-2600/X-2600B/product.ini"));
        qInfo() << "проект:" << ps.GetProjectName() << "ID:" << ps.GetProjectID()
                << "тема:" << ps.GetThemeName() << "langMode:" << ps.LanguageMode();
        qInfo() << "серии:" << ps.GetProductSeriesList();
        qInfo() << "модели X-2600:" << ps.GetProductModelList("X-2600");
        qInfo() << "флаги ZOOM/CHB/RECORD:" << ps.IsZoomEnable() << ps.IsChbEnable()
                << ps.IsVideoRecordEnable();
        qInfo() << "лимиты ImgEnh/ColEnh/RBC:" << ps.GetImgEnhLevel() << ps.GetColEnhLevel()
                << ps.GetColRBCMax() << ps.GetColRBCMin()
                << "Zoom:" << ps.GetZoomMin() << ".." << ps.GetZoomMax();
        qInfo() << "PAPP00/06/80:" << ps.IsShowPAPP(0) << ps.IsShowPAPP(6) << ps.IsShowPAPP(80);

        const bool ok = pOk && cOk &&
            ps.GetProjectName() == "X-2000" && ps.GetProjectID() == 56 &&
            ps.GetThemeName() == "black" && ps.LanguageMode() == 8 &&
            ps.GetProductSeriesList().size() == 6 &&
            ps.GetProductSeriesList().first() == "X-2600" &&
            ps.GetProductModelList("X-2600").contains("X-2600B") &&
            !ps.IsZoomEnable() && !ps.IsChbEnable() && !ps.IsVideoRecordEnable() &&
            ps.GetImgEnhLevel() == 16 && ps.GetColEnhLevel() == 16 &&
            ps.GetColRBCMax() == 15 && ps.GetColRBCMin() == -15 &&
            qAbs(ps.GetZoomMax() - 4.0) < 1e-6 &&
            ps.IsShowPAPP(0) && ps.IsShowPAPP(6) && ps.IsShowPAPP(80);
        qInfo() << (ok ? "project: PASS" : "project: FAIL");
        return ok ? 0 : 21;
    }

    // Self-test матрицы совместимости версий (matchedversion.ini).
    if (screen == "version") {
        KUpdateConf &uc = KUpdateConf::GetInstance();
        qInfo() << "matchedversion.ini NUM:" << uc.MatchedNum();
        const auto hmi = uc.GetMatchedVersion("hmi");      // 2 допустимые версии
        const auto kernel = uc.GetMatchedVersion("kernel");// 1 версия
        qInfo() << "hmi допустимые:" << hmi;
        qInfo() << "kernel:" << kernel << "camera:" << uc.GetMatchedVersion("camera");
        // Проверка совместимости.
        const bool m1 = uc.IsVersionMatched("hmi", "56.11.00.00.02.03");  // вторая допустимая
        const bool m2 = uc.IsVersionMatched("kernel", "56.15.01.00.00.10");
        const bool m3 = uc.IsVersionMatched("camera", "99.99.99.99.99.99"); // несовместимо
        qInfo() << "hmi(2-я)=" << m1 << "kernel=" << m2 << "camera(чужая)=" << m3;

        // Интеграция: установленные версии (KVersionConfig) vs matched (совместимость).
        const QString vpath = "/tmp/endo_version.ini";
        QFile::remove(vpath);
        KVersionConfig &vc = KVersionConfig::GetInstance();
        vc.SetConfigFile(vpath);
        vc.SetVersion("kernel", "56.15.01.00.00.10");    // совместимо
        vc.SetVersion("hmi", "56.11.00.00.02.03");        // совместимо (2-я допустимая)
        vc.SetVersion("camera", "99.99.99.99.99.99");     // НЕсовместимо
        const bool compKernel = vc.IsComponentCompatible("kernel");
        const bool compHmi = vc.IsComponentCompatible("hmi");
        const bool compCamera = vc.IsComponentCompatible("camera");
        const bool allOk = vc.IsCompatible({"kernel", "hmi"});
        qInfo() << "установлено kernel:" << vc.GetVersion("kernel")
                << "совместимо:" << compKernel << "| hmi:" << compHmi
                << "| camera:" << compCamera << "| kernel+hmi:" << allOk;

        const bool ok = uc.MatchedNum() == 14 && hmi.size() == 2 &&
                        hmi.contains("56.21.00.00.01.17") &&
                        hmi.contains("56.11.00.00.02.03") &&
                        kernel.size() == 1 && kernel.first() == "56.15.01.00.00.10" &&
                        m1 && m2 && !m3 &&
                        compKernel && compHmi && !compCamera && allOk;
        qInfo() << (ok ? "version: PASS" : "version: FAIL");
        return ok ? 0 : 16;
    }

    // Self-test источника холодного света: VLS-конфиги + LED-параметры.
    if (screen == "coldlight") {
        const QString base = QDir(KSystem::SystemPath()).absoluteFilePath("coldlight");
        KColdLightConfig &cl = KColdLightConfig::GetInstance();
        // VLS-конфиги (комбо спектральных режимов) из per-продукт coldlight.ini.
        const bool vlsOk = cl.LoadVLSConfig(
            QDir(base).absoluteFilePath("X-2600/X-2600B/coldlight.ini"));
        const auto combos = cl.GetVLSConfigDisplayList();
        qInfo() << "VLS configs:" << cl.VLSConfigNum();
        for (int i = 0; i < combos.size(); ++i)
            qInfo() << "  VLSConfig" << i << "=" << combos[i];
        cl.SetUserVLSConfig(0, 1);   // combo0=[WL,SFI,VIST], mode index 1 → SFI
        qInfo() << "выбран режим:" << cl.CurrentMode();

        // LED-параметры (26 float per модель×режим) из coldlightCommPara.ini.
        const bool cpOk = cl.LoadCommPara(
            QDir(base).absoluteFilePath("coldlightCommPara.ini"));
        const auto p = cl.GetLightParam("EB-X20", "WL");
        qInfo() << "EB-X20/WL параметров:" << p.size()
                << "первые:" << (p.size() >= 3 ? QString("%1,%2,%3").arg(p[0]).arg(p[1]).arg(p[2]) : "");

        const bool ok = vlsOk && cl.VLSConfigNum() == 4 &&
                        combos[0] == QStringList{"WL","SFI","VIST"} &&
                        combos[2] == QStringList{"SFI","VIST"} &&   // "SFI,NULL,VIST" → NULL убран
                        cl.CurrentMode() == "SFI" &&
                        cpOk && p.size() == 26 && qAbs(p[1] - 0.2f) < 1e-4;
        qInfo() << (ok ? "coldlight: PASS" : "coldlight: FAIL");
        return ok ? 0 : 15;
    }

    // Self-test KUserSet: полный парсинг osd.ini (силы усиления/zoom/кнопки).
    if (screen == "userset") {
        const QString ini = outFile.isEmpty()
            ? QDir(KSystem::SystemPath()).absoluteFilePath(
                  "presetdata/userpreset/X-2600/X-2600B/osd.ini")
            : outFile;
        _KUserConf c;
        KUserSet::Instance().ReadVideoParamConfig(&c, ini);
        qInfo() << "osd.ini:" << ini;
        qInfo() << "ImgEnhStrA:" << c.imgEnhStrA[0] << c.imgEnhStrA[1] << c.imgEnhStrA[2];
        qInfo() << "zoomScale:" << c.zoomScale[0] << c.zoomScale[1] << c.zoomScale[2];
        qInfo() << "btnA long/short:" << c.btnALong << c.btnAShort
                << " footSwitch1/2:" << c.footSwitch1 << c.footSwitch2;
        qInfo() << "contrast:" << c.contrastLevel << "denoise:" << c.imgDenoise
                << "brightEQ:" << c.brightnessEQ;
        const bool ok =
            c.imgEnhStrA[0] == 4 && c.imgEnhStrA[1] == 9 && c.imgEnhStrA[2] == 15 &&
            c.imgEnhStrB[2] == 15 && c.imgEnhStrEdge[2] == 15 &&
            qAbs(c.zoomScale[0] - 1.0f) < 1e-4 && qAbs(c.zoomScale[1] - 1.2f) < 1e-4 &&
            qAbs(c.zoomScale[2] - 1.4f) < 1e-4 &&
            c.btnALong == 9 && c.btnAShort == 6 && c.btnBLong == 5 && c.btnBShort == 7 &&
            c.footSwitch2 == 1 && c.contrastLevel == 1 && c.imgDenoise == 1;
        qInfo() << (ok ? "userset: PASS" : "userset: FAIL");
        return ok ? 0 : 14;
    }

    // Self-test тезауруса: словарь шаблонов диагнозов → автозаполнение отчёта.
    if (screen == "thesaurus") {
        KThesaurusOpt th;   // lang=ch по умолчанию
        const bool loaded = th.Load(KThesaurusOpt::Gastroscopy);
        qInfo() << "Gastroscopy.xml файл:" << th.GetFileNameByEndoscopeType(KThesaurusOpt::Gastroscopy);
        qInfo() << "загружено записей:" << th.Records().size()
                << "групп:" << th.Groups().size();
        // Найти запись по диагнозу и проверить непустые поля.
        KThesaurusOpt::Record rec;
        const bool found = th.RecordByDisease(
            QString::fromUtf8("急性食管炎"), rec);   // "острый эзофагит"
        qInfo() << "найдено 'острый эзофагит':" << found
                << "grid:" << rec.grid
                << "examfinding непустое:" << !rec.examFinding.isEmpty();

        // Интеграция с отчётом: тезаурус → поля ReportEntity.
        ReportEntity re;
        if (found) {
            re.diseaseName = rec.diseaName;         // RT_DISEASE_NAME
            re.surgeryFinding = rec.examFinding;    // RT_SURGERY_FINDING
            re.diagnosis = rec.diagResult;          // RT_DIAGNOSIS
        }
        qInfo() << "отчёт заполнен: diseaseName непустое=" << !re.diseaseName.isEmpty()
                << "diagnosis непустое=" << !re.diagnosis.isEmpty();

        // Add/Del роундтрип (в памяти).
        const int before = th.Records().size();
        const QString grid = th.AddDiseaseContent("测试", "TestDisease", "finding", "result");
        KThesaurusOpt::Record added;
        const bool addOk = th.RecordByGrid(grid, added) && added.diseaName == "TestDisease";
        const bool delOk = th.DelDiseaseContentByGrid(grid) && th.Records().size() == before;

        const bool ok = loaded && th.Records().size() > 0 && th.Groups().size() > 0 &&
                        found && !rec.examFinding.isEmpty() && !re.diagnosis.isEmpty() &&
                        addOk && delOk;
        qInfo() << "add/del roundtrip:" << (addOk && delOk);
        qInfo() << (ok ? "thesaurus: PASS" : "thesaurus: FAIL");
        return ok ? 0 : 13;
    }

    // Self-test демо-источника отчёта: образцовые данные + DemoImage → превью.
    if (screen == "dsdemo") {
        KRTDataSourceDemo dsd;
        KReportDataSource ds;
        dsd.Build(ds, 4);
        KReportTemplateManager mgr;
        const auto items = mgr.LoadTemplate("NP-2x2");
        KDocumentGenerator gen;
        const QString html = gen.Generate(items, ds);
        const int imgCount = html.count("<img");
        // Проверить, что демо-снимки указывают на реальные DemoImage-ассеты.
        const QString img0 = ds.GetValue("RT_DATASOURCE_PERIPHERAL", "RT_TEST_IMAGE0");
        const bool demoImgExists = QFile::exists(img0);
        qInfo() << "демо: пациент=" << ds.GetValue("RT_DATASOURCE_PATIENT", "RT_PATIENT_NAME")
                << "| <img>=" << imgCount << "| Image0:" << img0 << "существует:" << demoImgExists;
        const bool ok = ds.GetValue("RT_DATASOURCE_PATIENT", "RT_PATIENT_NAME") == "Demo Patient" &&
                        html.contains("Demo Patient") && html.contains("Sample conclusion") &&
                        imgCount == 4 && img0.contains("DemoImage/Image0.png") && demoImgExists;
        qInfo() << (ok ? "dsdemo: PASS" : "dsdemo: FAIL");
        return ok ? 0 : 29;
    }

    // Self-test реального источника данных отчёта: БД → датасорс → генератор.
    if (screen == "dsreal") {
        const QString dbMain = "/tmp/endo_dsr_main.db";
        const QString dbRep = "/tmp/endo_dsr_rep.db";
        QFile::remove(dbMain); QFile::remove(dbRep);
        // Заполнить БД пациента/осмотра (endo_main) и отчёта (endo_report).
        KEntityManage &em = KEntityManage::Instance();
        em.OpenDb(dbMain);
        em.AddPatientEntity({"P001", "Ivanov Ivan", "M", "45", "1980-05-01"});
        KEntityExam ex; ex.CreateTable();
        { ExamListEntity e; e.examId = "A0001"; e.accessionNumber = "A0001";
          e.patientId = "P001"; e.examDate = "2026-07-14"; ex.CreateEntity(e); }
        KEntityReport &er = KEntityReport::Instance();
        er.OpenDb(dbRep);
        { ReportEntity r; r.accessionNumber = "A0001"; r.diseaseName = "Gastritis";
          r.diagnosis = "Chronic gastritis"; r.surgeryFinding = "Mucosal erythema";
          r.suggestion = "Recheck 6mo"; r.examView = "Stomach"; er.SaveReport(r); }

        // Собрать датасорс из БД (реф. KRTDataSourceReal).
        KRTDataSourceReal dsr;
        KReportDataSource ds;
        const bool built = dsr.Build("P001", "A0001", ds);
        // Снимки (peripheral) — 4 для 2x2.
        for (int i = 0; i < 4; ++i)
            ds.SetImage(i, QString("/data/exam/A0001/%1.jpeg").arg(i), QString("M%1").arg(i));

        // Генерация отчёта из РЕАЛЬНЫХ данных.
        KReportTemplateManager mgr;
        const auto items = mgr.LoadTemplate("NP-2x2");
        KDocumentGenerator gen;
        const QString html = gen.Generate(items, ds);

        qInfo() << "датасорс собран:" << built
                << "| пациент:" << ds.GetValue("RT_DATASOURCE_PATIENT", "RT_PATIENT_NAME")
                << "| диагноз:" << ds.GetValue("RT_DATASOURCE_PATIENT", "RT_DIAGNOSIS");
        const bool ok = built &&
            ds.GetValue("RT_DATASOURCE_PATIENT", "RT_PATIENT_NAME") == "Ivanov Ivan" &&
            ds.GetValue("RT_DATASOURCE_PATIENT", "RT_DIAGNOSIS") == "Chronic gastritis" &&
            ds.GetValue("RT_DATASOURCE_PATIENT", "RT_SURGERY_FINDING") == "Mucosal erythema" &&
            ds.GetValue("RT_DATASOURCE_PATIENT", "RT_EXAMDATE") == "2026-07-14" &&
            html.contains("Ivanov Ivan") && html.contains("Chronic gastritis") &&
            html.count("<img") == 4;
        er.CloseDb(); em.CloseDb();
        qInfo() << (ok ? "dsreal: PASS" : "dsreal: FAIL");
        return ok ? 0 : 28;
    }

    // Self-test отчётов: шаблон (XML) → генератор HTML + БД tb_Report.
    if (screen == "report") {
        KReportTemplateManager mgr;
        const QStringList tpls = mgr.TemplateNames();
        qInfo() << "templates:" << tpls;
        const QVector<ReportItem> items = mgr.LoadTemplate("NP-2x2");
        qInfo() << "NP-2x2 root blocks:" << items.size();

        // Источник данных: пациент + 4 снимка (2x2).
        KReportDataSource ds;
        ds.SetPatient("RT_PATIENT_NAME", "Ivanov Ivan");
        ds.SetPatient("RT_PATIENT_ID", "P001");
        ds.SetPatient("RT_PATIENT_GENDER", "M");
        ds.SetPatient("RT_DIAGNOSIS", "Gastritis");
        ds.SetPatient("RT_SUGGESTION", "Recheck in 6 months");
        for (int i = 0; i < 4; ++i)
            ds.SetImage(i, QString("/data/exam/A0001/%1.jpeg").arg(i),
                        QString("Mark%1").arg(i));

        KDocumentGenerator gen;
        const QString html = gen.Generate(items, ds);
        const QString outHtml = outFile.isEmpty() ? "/tmp/endo_report.html" : outFile;
        { QFile hf(outHtml); if (hf.open(QIODevice::WriteOnly)) hf.write(html.toUtf8()); }

        const int imgCount = html.count("<img");
        // ItemConfig-раскладка: Image2x2 задаёт ImageWidth=197, AlignH=Center; ExamInfo — Section=Body.
        const bool cfgOk = html.contains("width:197px") &&
                           html.contains("text-align:center") &&
                           html.contains("data-section=\"Body\"");
        const bool genOk = !items.isEmpty() &&
                           html.contains("Ivanov Ivan") &&
                           html.contains("Gastritis") &&
                           html.contains("/data/exam/A0001/0.jpeg") && imgCount == 4 && cfgOk;
        qInfo() << "ItemConfig применён (width197/center/section):" << cfgOk;
        qInfo() << "HTML:" << html.size() << "bytes, <img>=" << imgCount
                << "→" << outHtml;

        // БД отчёта: сохранить/прочитать.
        const QString dbPath = "/tmp/endo_report.db";
        QFile::remove(dbPath);
        KEntityReport &er = KEntityReport::Instance();
        er.OpenDb(dbPath);
        ReportEntity re;
        re.accessionNumber = "A0001"; re.templateName = "NP-2x2";
        re.diagnosis = "Gastritis"; re.suggestion = "Recheck in 6 months";
        re.examView = "Esophagus, Stomach";
        const bool save = er.SaveReport(re);
        ReportEntity rd;
        const bool got = er.GetReport("A0001", rd);
        const int n = er.GetReportNumber();
        er.CloseDb();
        qInfo() << "Report DB: save=" << save << "count=" << n
                << "diagnosis=" << rd.diagnosis << "template=" << rd.templateName;

        const bool ok = genOk && save && got && n == 1 &&
                        rd.diagnosis == "Gastritis" && rd.templateName == "NP-2x2";
        qInfo() << (ok ? "report: PASS" : "report: FAIL");
        return ok ? 0 : 11;
    }

    // Self-test DICOM: парсинг WorklistFieldMap.xml + БД tb_DcmWorklist/tb_DcmStore.
    if (screen == "dicom") {
        const QString xml = QDir(KSystem::UserPresetPath())
            .absoluteFilePath("dicom/WorklistFieldMap.xml");
        KDicomFieldMap fm;
        const bool xmlOk = fm.Load(xml);
        qInfo() << "WorklistFieldMap.xml:" << xml;
        qInfo() << "  loaded=" << xmlOk << "record=" << fm.RecordType()
                << "fields=" << fm.Fields().size() << "columns=" << fm.ColumnNames().size();
        // Проверка известных колонок из маппинга
        const auto cols = fm.ColumnNames();
        const bool hasKey = cols.contains("AccessionNumber") &&
                            cols.contains("PatientID") && cols.contains("StudyInstanceUID") &&
                            cols.contains("Modality");

        const QString dbPath = outFile.isEmpty() ? "/tmp/endo_dicom_test.db" : outFile;
        QFile::remove(dbPath);
        KEntityDicom &ed = KEntityDicom::Instance();
        if (!ed.OpenDb(dbPath, xml)) { qWarning() << "OpenDb failed"; return 9; }

        // Worklist: вставить задачу, прочитать, посчитать.
        QMap<QString, QString> wl;
        wl["AccessionNumber"] = "ACC12345";
        wl["PatientID"]       = "P001";
        wl["PatientName"]     = "Ivanov^Ivan";
        wl["PatientSex"]      = "M";
        wl["Modality"]        = "ES";
        wl["StudyInstanceUID"] = "1.2.840.113619.2.1.1";
        wl["ScheduledProcedureStepStartDate"] = "20260714";
        const bool wlAdd = ed.CreateWorklistEntity(wl);
        const auto got = ed.GetWorklistEntity("ACC12345");
        const int wlN = ed.GetWorklistNumber();

        // Store queue: поставить снимок в очередь, обновить статус.
        DcmStoreEntity se;
        se.sopInstanceUID = "1.2.840.113619.2.5.1";
        se.examId = "ACC12345"; se.studyInstanceUID = "1.2.840.113619.2.1.1";
        se.seriesInstanceUID = "1.2.840.113619.2.4.1";
        se.filePath = "/data/ACC12345/1.dcm"; se.serverName = "PACS1"; se.sendStatus = 0;
        const bool stAdd = ed.CreateStoreEntity(se);
        ed.UpdateStoreStatus("1.2.840.113619.2.5.1", 2, 1);
        const auto stList = ed.GetStoreListByExam("ACC12345");
        const int stN = ed.GetStoreNumber();

        qInfo() << "Worklist: add=" << wlAdd << "count=" << wlN
                << "PatientName=" << got.value("PatientName")
                << "Modality=" << got.value("Modality");
        qInfo() << "Store: add=" << stAdd << "count=" << stN
                << "status=" << (stList.isEmpty() ? -1 : stList.first().sendStatus)
                << "retry=" << (stList.isEmpty() ? -1 : stList.first().retryCount);

        // Study/Series иерархия (Study→Series→SOP).
        DcmStudyEntity study;
        study.studyInstanceUID = "1.2.840.113619.2.1.1"; study.studyID = "S1";
        study.studyDate = "20260714"; study.modality = "ES"; study.patientId = "P001";
        ed.CreateStudyEntity(study);
        DcmSeriesEntity ser;
        ser.seriesInstanceUID = "1.2.840.113619.2.4.1"; ser.studyInstanceUID = study.studyInstanceUID;
        ser.seriesNumber = 1; ser.modality = "ES"; ser.numberOfInstances = 5;
        ed.CreateSeriesEntity(ser);
        DcmStudyEntity gotStudy;
        const bool studyOk = ed.GetStudyEntity(study.studyInstanceUID, gotStudy) &&
                             gotStudy.patientId == "P001" && gotStudy.modality == "ES";
        const auto studies = ed.GetStudiesByPatient("P001");
        const auto series = ed.GetSeriesByStudy(study.studyInstanceUID);
        qInfo() << "study: получен=" << studyOk << "у P001:" << studies.size()
                << "серий в study:" << series.size()
                << "инстансов в серии:" << (series.isEmpty() ? -1 : series.first().numberOfInstances);

        // MPPS (жизненный цикл: IN PROGRESS → COMPLETED) + Storage Commitment.
        DcmMppsEntity mpps;
        mpps.mppsUID = "1.2.840.113619.2.9.1"; mpps.examId = "ACC12345";
        mpps.stepID = "PPS1"; mpps.status = "IN PROGRESS";
        mpps.startDate = "20260714"; mpps.startTime = "120000";
        ed.CreateMppsEntity(mpps);
        ed.UpdateMppsStatus(mpps.mppsUID, "COMPLETED", "20260714", "121500");
        DcmMppsEntity gotMpps; const bool mppsOk = ed.GetMppsEntity(mpps.mppsUID, gotMpps);
        DcmCommitEntity commit;
        commit.transactionUID = "1.2.840.113619.2.10.1"; commit.examId = "ACC12345";
        commit.sopInstanceUID = "1.2.840.113619.2.5.1"; commit.commitStatus = 0;
        ed.CreateCommitEntity(commit);
        ed.UpdateCommitStatus(commit.transactionUID, 1);   // подтверждён
        DcmCommitEntity gotCommit; const bool commitOk = ed.GetCommitEntity(commit.transactionUID, gotCommit);
        qInfo() << "MPPS статус:" << gotMpps.status << "end:" << gotMpps.endTime
                << "| Commit статус:" << gotCommit.commitStatus;

        // MPPS field-map (мульти-record: PerformedProcedureStep/ProcedureCode/Series/…).
        KDicomFieldMap mpm;
        const QString mppsXml = QDir(KSystem::UserPresetPath())
            .absoluteFilePath("dicom/MppsSetFieldMap.xml");
        const bool mpmOk = mpm.Load(mppsXml);
        bool ppsFound = false;
        const auto pps = mpm.RecordByType("PerformedProcedureStep", &ppsFound);
        bool procFound = false;
        const auto proc = mpm.RecordByType("ProcedureCode", &procFound);
        qInfo() << "MppsSetFieldMap: loaded=" << mpmOk << "записей=" << mpm.RecordCount()
                << "| PPS полей=" << pps.fields.size()
                << "| ProcedureCode SubGroup=" << proc.subGroup;
        // Тот же парсер обрабатывает и MppsCreateFieldMap.xml (Create-вариант).
        KDicomFieldMap mpmCreate;
        const bool mpmCreateOk = mpmCreate.Load(QDir(KSystem::UserPresetPath())
            .absoluteFilePath("dicom/MppsCreateFieldMap.xml")) && mpmCreate.RecordCount() >= 1;
        const bool mpmCheck = mpmOk && mpm.RecordCount() >= 3 &&
                              ppsFound && pps.fields.size() >= 4 &&
                              procFound && proc.subGroup == "DCM_ProcedureCodeSequence" &&
                              mpm.Fields().size() > pps.fields.size() &&  // все записи плоско
                              mpmCreateOk;

        const bool ok = xmlOk && hasKey && wlAdd && wlN == 1 &&
                        got.value("PatientName") == "Ivanov^Ivan" &&
                        got.value("Modality") == "ES" && stAdd && stN == 1 &&
                        !stList.isEmpty() && stList.first().sendStatus == 2 &&
                        stList.first().retryCount == 1 &&
                        studyOk && studies.size() == 1 && series.size() == 1 &&
                        series.first().numberOfInstances == 5 && ed.GetStudyNumber() == 1 &&
                        mppsOk && gotMpps.status == "COMPLETED" && gotMpps.endTime == "121500" &&
                        commitOk && gotCommit.commitStatus == 1 &&
                        mpmCheck;
        ed.CloseDb();
        qInfo() << (ok ? "dicom: PASS" : "dicom: FAIL");
        return ok ? 0 : 10;
    }

    // Self-test KDccuParam: запись/чтение параметров в dccuparam.ini (реф. ключи).
    if (screen == "dccu") {
        KDccuParam &d = KDccuParam::GetInstance();
        qInfo() << "dccuparam.ini:" << d.ConfigFile();
        d.SetAECControl(1);
        d.SetAutoAEC(0);
        d.SetAGCMax(240);
        d.SetManualAGC(2.5);
        d.SetAwbUp(120);
        d.SetRBSatiation(64);
        d.SetGammaRatio(0.85);
        // Роундтрип чтения
        const bool rt = d.GetAECControl() == 1 && d.GetAutoAEC() == 0 &&
                        d.GetAGCMax() == 240 && qAbs(d.GetManualAGC() - 2.5) < 1e-6 &&
                        d.GetAwbUp() == 120 && d.GetRBSatiation() == 64 &&
                        qAbs(d.GetGammaRatio() - 0.85) < 1e-6;
        qInfo() << "DccuParam roundtrip:" << rt
                << " AEC/Control=" << d.GetAECControl()
                << " AGC/Max=" << d.GetAGCMax()
                << " AGC/Manual=" << d.GetManualAGC()
                << " RB/Satiation=" << d.GetRBSatiation();
        qInfo() << (rt ? "dccu: PASS" : "dccu: FAIL");
        return rt ? 0 : 7;
    }

    QWidget *w = nullptr;
    if (screen == "desktop") {
        auto *desktop = new KUIDesktop;
        // Для проверки списка снимков: папка осмотра из ENDO_EXAM.
        const QString exam = QProcessEnvironment::systemEnvironment().value("ENDO_EXAM");
        if (!exam.isEmpty() && desktop->ImgList())
            desktop->ImgList()->SetExamFolder(exam);
        w = desktop;
    } else {
        qWarning() << "неизвестный экран:" << screen;
        return 2;
    }

    if (!outFile.isEmpty()) {
        w->show();                 // реализовать для корректного grab
        app.processEvents();
        const QPixmap shot = w->grab();
        if (shot.save(outFile))
            qInfo() << "saved" << outFile << shot.size();
        else
            qWarning() << "не удалось сохранить" << outFile;
        return 0;
    }

    w->show();
    return app.exec();
}
