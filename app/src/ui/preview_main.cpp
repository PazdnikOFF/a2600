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
#include "alg/AlgParaManager.h"
#include "ctrl/KPlControl.h"
#include "ctrl/KDccuParam.h"
#include "video/KVideoParam.h"
#include "sys/KSystem.h"
#include "dicom/KDicomFieldMap.h"
#include "dicom/KEntityDicom.h"
#include "report/KReportTemplate.h"
#include "report/KReportDataSource.h"
#include "report/KDocumentGenerator.h"
#include "report/KEntityReport.h"
#include "sys/KAccount.h"
#include "sys/KSystemSet.h"

#include <QDir>
#include <QFile>
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
        pl.ClearTrace();
        pl.SetGammaEnable(true);
        pl.SetZoomValue(0x123);
        pl.SetCCM1(1);
        const unsigned ccm1[9] = {0x0100,0,0, 0,0x0100,0, 0,0,0x0100};
        pl.SetCCM1Matrix(ccm1, 9);
        pl.SetChbStatus(0x55);
        const auto &tb = pl.Trace();
        // gamma(1) + zoom(1) + ccm1en(1) + ccm1matrix(4 пары) + chb(3) = 10
        bool batchOk = tb.size() == 1 + 1 + 1 + 4 + 3 &&
                       tb[0].first == 0xa1830000 && tb[1].first == 0xa18d0004 &&
                       tb[1].second == 0x123 && tb[2].first == 0xa1880000 &&
                       tb[3].first == 0xa1880004 && tb[3].second == 0x100 &&
                       tb[7].first == 0xa1900008 && tb[7].second == 1 &&
                       tb[8].first == 0xa1900018 && tb[8].second == 0x55 &&
                       tb[9].first == 0xa1900008 && tb[9].second == 0;
        qInfo() << "batch writes:" << tb.size() << "(exp 10)" << (batchOk ? "OK" : "MISMATCH");
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
        const bool genOk = !items.isEmpty() &&
                           html.contains("Ivanov Ivan") &&
                           html.contains("Gastritis") &&
                           html.contains("/data/exam/A0001/0.jpeg") && imgCount == 4;
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
        ed.CloseDb();

        const bool ok = xmlOk && hasKey && wlAdd && wlN == 1 &&
                        got.value("PatientName") == "Ivanov^Ivan" &&
                        got.value("Modality") == "ES" && stAdd && stN == 1 &&
                        !stList.isEmpty() && stList.first().sendStatus == 2 &&
                        stList.first().retryCount == 1;
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
