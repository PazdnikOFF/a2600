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
#include "alg/AlgParaManager.h"
#include "ctrl/KPlControl.h"
#include "ctrl/KDccuParam.h"
#include "video/KVideoParam.h"
#include "video/KVideoSet.h"
#include "video/KVideoCal.h"
#include "sys/KSystem.h"
#include "dicom/KDicomFieldMap.h"
#include "dicom/KEntityDicom.h"
#include "report/KReportTemplate.h"
#include "report/KReportDataSource.h"
#include "report/KDocumentGenerator.h"
#include "report/KEntityReport.h"
#include "report/KThesaurusOpt.h"
#include "report/KRTDataSourceReal.h"
#include "report/KRTDataSourceDemo.h"
#include "sys/KAccount.h"
#include "sys/KSystemSet.h"
#include "sys/KUserSet.h"
#include "ctrl/KColdLightConfig.h"
#include "sys/KUpdateConf.h"
#include "sys/KUpdateManifest.h"
#include "sys/KVersionConfig.h"
#include "sys/KProjectSet.h"
#include "sys/KStyleConfig.h"
#include "sys/KStatisticConfig.h"
#include "sys/KSystemStatus.h"

#include <QDir>
#include <QFile>
#include <QSettings>
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
        // gamma(1)+zoom(1)+ccm1en(1)+ccm1matrix(4)+chb-on(2: en+val)+chb-off(1) = 10
        bool batchOk = tb.size() == 1 + 1 + 1 + 4 + 2 + 1 && chb == 0x34803c0 &&
                       tb[0].first == 0xa1830000 && tb[1].first == 0xa18d0004 &&
                       tb[1].second == 0x123 && tb[2].first == 0xa1880000 &&
                       tb[3].first == 0xa1880004 && tb[3].second == 0x100 &&
                       tb[7].first == 0xa1900008 && tb[7].second == 1 &&
                       tb[8].first == 0xa1900018 && tb[8].second == (unsigned)chb &&
                       tb[9].first == 0xa1900008 && tb[9].second == 0;
        qInfo() << "batch writes:" << tb.size() << "(exp 10) chb=0x" + QString::number(chb,16)
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
            const int n = qMin(rb.hb.size(), qMin(rb.hr.size(), rb.s.size()));
            pl.SetRbcLut(rb.hb.constData(), rb.hr.constData(), rb.s.constData(), n);
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
        pl.SetKneeLut(knee.constData(), knee.size());
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

        // Iris table (config-driven, 8040 значений → 1005 записей, упаковка 8/регистр).
        const auto iris = alg.LoadIrisTable("IMX274", "1920X1080");
        pl.ClearTrace();
        pl.SetIrisTable(iris.constData(), iris.size(), 0);   // shift 0
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
        pl.ReadBrightnessHistogramValue(hist, 256);   // на десктопе: только триггер-запись
        pl.ReadIrisValue();                            // read → 0 (нет /dev/mem)
        const auto &tx = pl.Trace();
        bool auxOk = tx.size() == 3 &&
                     tx[0].first == 0xa004a02c && tx[0].second == (0x12u | (0x34u<<8)) &&
                     tx[1].first == 0xa18d0008 && tx[1].second == (20u | (0x128u<<16)) &&
                     tx[2].first == 0xa18a0010 && tx[2].second == 1;
        qInfo() << "aux writes:" << tx.size() << "(exp 3, aurora/capture/hist-trig)"
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
        KPlControl::DenoiseData d;
        d.dpc[0] = d.dpc[1] = dp.dpcT1; d.dpc[2] = d.dpc[3] = dp.dpcT2;
        d.kernelG = dp.kernelG.constData();  d.kernelGCount = dp.kernelG.size();
        d.kernelRB = dp.kernelRB.constData(); d.kernelRBCount = dp.kernelRB.size();
        d.lut = dp.lut.constData();          d.lutCount = dp.lut.size();
        pl.ClearTrace();
        pl.SetDenoiseLut(d);
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

        const bool ok = itemsOk && decideOk && flagsOk;
        qInfo() << (ok ? "update: PASS" : "update: FAIL");
        return ok ? 0 : 30;
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

        const bool ok = xmlOk && hasKey && wlAdd && wlN == 1 &&
                        got.value("PatientName") == "Ivanov^Ivan" &&
                        got.value("Modality") == "ES" && stAdd && stN == 1 &&
                        !stList.isEmpty() && stList.first().sendStatus == 2 &&
                        stList.first().retryCount == 1 &&
                        studyOk && studies.size() == 1 && series.size() == 1 &&
                        series.first().numberOfInstances == 5 && ed.GetStudyNumber() == 1 &&
                        mppsOk && gotMpps.status == "COMPLETED" && gotMpps.endTime == "121500" &&
                        commitOk && gotCommit.commitStatus == 1;
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
