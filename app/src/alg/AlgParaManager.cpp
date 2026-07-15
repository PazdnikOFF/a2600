#include "alg/AlgParaManager.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <cmath>

AlgParaManager &AlgParaManager::GetInstance()
{
    static AlgParaManager inst;   // реф. статический pAlgParaManager
    return inst;
}

QString AlgParaManager::GetSensorConfigFile(const QString &category,
                                            const QString &sensor) const
{
    return QDir(KSystem::VideoConfPath()).absoluteFilePath(category + "/" + sensor);
}

AlgParaManager::GammaParam AlgParaManager::LoadGammaPara(const QString &sensor,
                                                        const QString &scope) const
{
    // videoconf/Gamma/<SENSOR>_GammaParam[_<SCOPE>].ini
    const QString dir = QDir(KSystem::VideoConfPath()).absoluteFilePath("Gamma");
    QString file = QString("%1/%2_GammaParam").arg(dir, sensor);
    if (!scope.isEmpty() && QFileInfo::exists(file + "_" + scope + ".ini"))
        file += "_" + scope;
    file += ".ini";

    GammaParam p;
    QSettings ini(file, QSettings::IniFormat);
    // Секция [V01] (первый профиль)
    p.bp       = ini.value("V01/bp", p.bp).toDouble();
    p.gamma    = ini.value("V01/gamma", p.gamma).toDouble();
    p.inputmax = ini.value("V01/inputmax", p.inputmax).toInt();
    return p;
}

QVector<int> AlgParaManager::CalGammaLut(const GammaParam &p)
{
    // Гамма-кривая с вычитанием чёрной точки bp; вход/выход 0..inputmax-1.
    const int N = p.inputmax > 1 ? p.inputmax : 1024;
    const double outMax = N - 1;
    QVector<int> lut(N);
    for (int i = 0; i < N; ++i) {
        double x = double(i) / (N - 1);            // нормализованный вход
        x = (x - p.bp) / (1.0 - p.bp);             // вычитание чёрной точки
        if (x < 0.0) x = 0.0;
        const double y = std::pow(x, 1.0 / p.gamma);
        lut[i] = int(std::lround(y * outMax));
    }
    return lut;
}

bool AlgParaManager::LoadColEnhPara(const QString &sensor)
{
    // videoconf/ColEnh/<sensor>/ — параметры цветоусиления (загрузка в PL).
    return QFileInfo::exists(GetSensorConfigFile("ColEnh", sensor));
}

AlgParaManager::CcmMatrix AlgParaManager::LoadCcmMatrix(const QString &sensor,
                                                        const QString &res,
                                                        const QString &scope) const
{
    // videoconf/Ccm/V1/<SENSOR>_<WxH>_<SCOPE>.txt — 9 hex-значений (знаковые 16-бит, Q9).
    CcmMatrix ccm;
    const QString file = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("Ccm/V1/%1_%2_%3.txt").arg(sensor, res, scope));
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return ccm;
    int idx = 0;
    while (idx < 9 && !f.atEnd()) {
        const QString tok = QString::fromLatin1(f.readLine()).trimmed();
        if (tok.isEmpty()) continue;
        bool ok = false;
        int v = tok.toInt(&ok, 16);           // hex
        if (!ok) continue;
        if (v > 0x7fff) v -= 0x10000;         // знаковое 16-бит
        ccm.m[idx++] = v;
    }
    ccm.valid = (idx == 9);
    return ccm;
}

namespace {
// Чтение файла «hex по строке» → массив int (пустые строки пропускаются).
QVector<int> ReadHexLines(const QString &path)
{
    QVector<int> out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    while (!f.atEnd()) {
        const QString tok = QString::fromLatin1(f.readLine()).trimmed();
        if (tok.isEmpty()) continue;
        bool ok = false;
        const int v = tok.toInt(&ok, 16);
        if (ok) out.append(v);
    }
    return out;
}
}

void AlgParaManager::LoadColEnhLevels(const QString &sensor)
{
    // videoconf/ColEnh/<sensor>/colenh_level.txt — значение ColorEnh на уровень.
    colEnhLevels_ = ReadHexLines(GetSensorConfigFile("ColEnh", sensor) + "/colenh_level.txt");
}

int AlgParaManager::ColEnhLevelValue(int level) const
{
    if (level < 0 || level >= colEnhLevels_.size()) return 0;
    return colEnhLevels_.at(level);
}

void AlgParaManager::LoadImgEnhLevels(const QString &mode, const QString &subdir, char ch)
{
    // videoconf/ImgEnh/<mode>/<subdir>/level_<ch>.txt — значение ImageEnh на уровень.
    const QString path = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("ImgEnh/%1/%2/level_%3.txt").arg(mode, subdir).arg(ch));
    imgEnhLevels_ = ReadHexLines(path);
}

int AlgParaManager::ImgEnhLevelValue(int level) const
{
    if (level < 0 || level >= imgEnhLevels_.size()) return 0;
    return imgEnhLevels_.at(level);
}

namespace {
// Разбор строки hex-токенов (разделители — пробелы/переводы строк).
QVector<int> ParseHexTokens(const QString &text)
{
    QVector<int> out;
    const QStringList toks = text.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    for (const QString &t : toks) {
        bool ok = false; const int v = t.toInt(&ok, 16);
        if (ok) out.append(v);
    }
    return out;
}
}

QString AlgParaManager::ModeDir(int mode)
{
    switch (mode) {
    case SFI:  return "SFI";
    case VIST: return "VIST";
    case EWL:  return "EWL";
    default:   return "WL";
    }
}

QVector<unsigned> AlgParaManager::LoadVistMatrix(int mode, const QString &sensor,
                                                 const QString &res) const
{
    // Vist/V1/<MODE>/<SENSOR>_<res>.txt — 9 значений (реф. LoadVistPara/GetVistValue).
    const QString file = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("Vist/V1/%1/%2_%3.txt").arg(ModeDir(mode), sensor, res));
    QVector<unsigned> m;
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return m;
    for (const int v : ReadHexLines(file))
        m.append(static_cast<unsigned>(v));
    Q_UNUSED(f);
    return m;
}

QVector<unsigned> AlgParaManager::LoadAwbGains(int mode, const QString &sensor,
                                               const QString &res, const QString &scope) const
{
    // Awb/V1/<MODE>/<SENSOR>_<res>[_<SCOPE>].txt — гейны/пороги ББ.
    const QString dir = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("Awb/V1/%1").arg(ModeDir(mode)));
    QString base = QString("%1/%2_%3").arg(dir, sensor, res);
    QString file = base + ".txt";
    if (!scope.isEmpty() && QFileInfo::exists(base + "_" + scope + ".txt"))
        file = base + "_" + scope + ".txt";
    QVector<unsigned> g;
    for (const int v : ReadHexLines(file))
        g.append(static_cast<unsigned>(v));
    return g;
}

AlgParaManager::DenoiseParam AlgParaManager::LoadDenoisePara(const QString &sensor,
                                                             int mode, int level,
                                                             const QString &scope) const
{
    // Denoise/<MODE>/<SENSOR>_Denoise[_<SCOPE>].ini, секция [denoiseLevel_<level>].
    DenoiseParam p;
    const QString dir = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("Denoise/%1").arg(ModeDir(mode)));
    QString base = QString("%1/%2_Denoise").arg(dir, sensor);
    QString file = base + ".ini";
    if (!scope.isEmpty() && QFileInfo::exists(base + "_" + scope + ".ini"))
        file = base + "_" + scope + ".ini";
    if (!QFileInfo::exists(file))
        return p;

    QSettings ini(file, QSettings::IniFormat);
    const QString sec = QString("denoiseLevel_%1").arg(level);
    p.dpcT1 = ini.value(sec + "/dpc_thd_t1").toString().toInt(nullptr, 16);
    p.dpcT2 = ini.value(sec + "/dpc_thd_t2").toString().toInt(nullptr, 16);
    p.kernelG  = ParseHexTokens(ini.value(sec + "/kernelG").toString());
    p.kernelRB = ParseHexTokens(ini.value(sec + "/kernelRB").toString());
    for (int i = 0; i < 16; ++i)
        p.lut += ParseHexTokens(ini.value(QString("%1/Lut_%2").arg(sec).arg(i)).toString());
    p.valid = !p.kernelG.isEmpty();
    return p;
}

AlgParaManager::SensorLut AlgParaManager::LoadSensorLut(const QString &sensor,
                                                        const QString &res) const
{
    // sensor_lut/<SENSOR>_<res>/sensor_lut_{r,g,b}.txt (hex по строке, 1024 значения).
    SensorLut lut;
    const QString dir = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("sensor_lut/%1_%2").arg(sensor, res));
    auto load = [&](const char *ch) {
        QVector<unsigned> out;
        for (const int v : ReadHexLines(QString("%1/sensor_lut_%2.txt").arg(dir, ch)))
            out.append(static_cast<unsigned>(v));
        return out;
    };
    lut.r = load("r");
    lut.g = load("g");
    lut.b = load("b");
    lut.valid = !lut.r.isEmpty() && !lut.g.isEmpty() && !lut.b.isEmpty();
    return lut;
}

AlgParaManager::RbcLut AlgParaManager::LoadRbcLut(const QString &sensor,
                                                  const QString &res) const
{
    // rbc_lut/<SENSOR>_<res>/colrbc_{Hb,Hr,S}.txt (hex по строке).
    RbcLut lut;
    const QString dir = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("rbc_lut/%1_%2").arg(sensor, res));
    auto load = [&](const char *ch) {
        QVector<unsigned> out;
        for (const int v : ReadHexLines(QString("%1/colrbc_%2.txt").arg(dir, ch)))
            out.append(static_cast<unsigned>(v));
        return out;
    };
    lut.hb = load("Hb");
    lut.hr = load("Hr");
    lut.s  = load("S");
    lut.valid = !lut.hb.isEmpty() && !lut.hr.isEmpty() && !lut.s.isEmpty();
    return lut;
}

void AlgParaManager::LoadChbPara(const QString &sensor, const QString &res)
{
    // CHb/<SENSOR>_<res>.txt — 4 hex-значения (реф. LoadCHBPara → LoadFileParaUInt).
    // SetChbStatus использует 4-е (индекс 3).
    const QString file = QDir(KSystem::VideoConfPath())
        .absoluteFilePath(QString("CHb/%1_%2.txt").arg(sensor, res));
    const QVector<int> vals = ReadHexLines(file);
    chbValue_ = (vals.size() >= 4) ? vals[3] : (vals.isEmpty() ? 0 : vals.last());
}

QVector<int> AlgParaManager::LoadKneeLut(const QString &sensor, const QString &scope) const
{
    // Knee/<SENSOR>_KneeLut[_<SCOPE>].txt (hex по строке).
    const QString dir = QDir(KSystem::VideoConfPath()).absoluteFilePath("Knee");
    QString base = QString("%1/%2_KneeLut").arg(dir, sensor);
    QString file = base + ".txt";
    if (!scope.isEmpty() && QFileInfo::exists(base + "_" + scope + ".txt"))
        file = base + "_" + scope + ".txt";
    return ReadHexLines(file);
}

QVector<int> AlgParaManager::LoadIrisTable(const QString &sensor, const QString &res,
                                           const QString &scope) const
{
    // IRIS/<SENSOR>_<res>[_<SCOPE>].txt (значения 1/3/7; hex-парсер = decimal здесь).
    const QString dir = QDir(KSystem::VideoConfPath()).absoluteFilePath("IRIS");
    QString base = QString("%1/%2_%3").arg(dir, sensor, res);
    QString file = base + ".txt";
    if (!scope.isEmpty() && QFileInfo::exists(base + "_" + scope + ".txt"))
        file = base + "_" + scope + ".txt";
    return ReadHexLines(file);
}

void AlgParaManager::LoadBrightEqPara(const QString &sensor)
{
    // videoconf/Bright_EQ/<sensor>/…
    const QString dir = GetSensorConfigFile("Bright_EQ", sensor);
    brightEqGauss_ = ReadHexLines(dir + "/gaussian_filter_hex.txt");   // 36 значений
    static const char *kNames[4] = {"disable", "low", "middle", "high"};
    for (int i = 0; i < 4; ++i)
        brightEqLuma_[i] = ReadHexLines(
            QString("%1/lumaGainLut_%2_hex.txt").arg(dir, kNames[i]));
}

const QVector<int> &AlgParaManager::BrightEqLumaLut(int idx) const
{
    static const QVector<int> empty;
    if (idx < 0 || idx > 3) return empty;
    return brightEqLuma_[idx];
}

QVector<int> AlgParaManager::LoadColEnhLut(const QString &sensor) const
{
    QVector<int> lut;
    QFile f(GetSensorConfigFile("ColEnh", sensor) + "/colenh_para.txt");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return lut;
    while (!f.atEnd()) {
        const QString tok = QString::fromLatin1(f.readLine()).trimmed();
        if (tok.isEmpty()) continue;
        bool ok = false;
        int v = tok.toInt(&ok, 16);
        if (ok) lut.append(v);
    }
    return lut;
}

// --- Обрезка углов (реф. SetCutCornerPara/SetRoundPara/SetOctagonPara) ---
namespace {
constexpr int kN = AlgParaManager::kCutCornerLen;   // 1080

// Реф. ResetCutCornerPara: заполнить буфер значением W (поле +0x10).
void cornerReset(QVector<int> &buf, int w)
{
    buf.fill(w, kN);
}

// Реф. SetRoundPara(buf, b=radius, c): дуга cut = min(W-2c, 2·√(b²−y²)),
// симметрично сверху/снизу; поля вне дуги обнуляются.
void cornerRound(QVector<int> &buf, int W, int H, int b, int c)
{
    const int top = c + (kN - H) / 2;              // граница обнуления
    if (top > 0)
        for (int i = 0; i < top && i < kN; ++i) {
            buf[i] = 0;
            if (kN - 1 - i >= 0) buf[kN - 1 - i] = 0;
        }
    const int rows = H / 2 - c;
    for (int i = 0; i < rows; ++i) {
        const int y = rows - i;                    // реф.: счётчик от rows до 1
        const double arg = double(b) * b - double(y) * y;
        const int s = (arg < 0.0) ? 0 : int(std::sqrt(arg));
        const int val = std::min(W - 2 * c, 2 * s);
        const int fwd = top + i, bwd = kN - 1 - top - i;
        if (fwd >= 0 && fwd < kN) buf[fwd] = val;
        if (bwd >= 0 && bwd < kN) buf[bwd] = val;
    }
}

// Реф. SetOctagonPara(buf, p2=b>>16, p3=b&0xffff, c): диагональный пандус
// cut = (W-2c) − 2·(p2/p3)·y на углах, плоская середина = W-2c.
void cornerOctagon(QVector<int> &buf, int W, int H, int p2, int p3, int c)
{
    const int top = c + (kN - H) / 2;
    if (top > 0)
        for (int i = 0; i < top && i < kN; ++i) {
            buf[i] = 0;
            if (kN - 1 - i >= 0) buf[kN - 1 - i] = 0;
        }
    if (p3 > 0) {
        const double slope2 = 2.0 * (double(p2) / double(p3));
        for (int i = 0; i < p3; ++i) {
            const int y = p3 - i;                  // реф.: счётчик от p3 до 1
            const double d = double(W - 2 * c) - slope2 * double(y);
            const int val = (d < 0.0) ? 0 : int(d);   // реф. fcvtzu (беззнак. усечение)
            const int fwd = top + i, bwd = kN - 1 - top - i;
            if (fwd >= 0 && fwd < kN) buf[fwd] = val;
            if (bwd >= 0 && bwd < kN) buf[bwd] = val;
        }
    }
    const int lo = top + p3, hi = (kN - top) - p3;
    for (int idx = lo; idx < hi; ++idx)
        if (idx >= 0 && idx < kN) buf[idx] = W - 2 * c;
}
}

void AlgParaManager::SetCutCornerPara(int way, int b, int c)
{
    if (way < 0 || way > 1) return;
    QVector<int> &buf = cutLut_[way];
    cornerReset(buf, cutW_);                        // реф. ResetCutCornerPara(=W)
    if (way == 0)
        cornerRound(buf, cutW_, cutH_, b, c);       // круг
    else
        cornerOctagon(buf, cutW_, cutH_, b >> 16, b & 0xffff, c);   // восьмиугольник
}

const QVector<int> &AlgParaManager::CutCornerLut(int way) const
{
    static const QVector<int> empty;
    return (way == 0 || way == 1) ? cutLut_[way] : empty;
}
