#include "ctrl/KColdLightConfig.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QSettings>

KColdLightConfig &KColdLightConfig::GetInstance()
{
    static KColdLightConfig inst;   // реф. g_pColdLightConfig
    return inst;
}

bool KColdLightConfig::LoadVLSConfig(const QString &coldlightIniPath)
{
    vlsConfigs_.clear();
    QSettings ini(coldlightIniPath, QSettings::IniFormat);
    const int num = ini.value("V01/VLSConfigNum", 0).toInt();
    if (num <= 0)
        return false;
    for (int i = 0; i < num; ++i) {
        // VLSConfig<i>=WL,SFI,VIST — QSettings сам разбивает по запятой (toStringList).
        QStringList modes;
        for (const QString &m : ini.value(QString("V01/VLSConfig%1").arg(i)).toStringList()) {
            const QString t = m.trimmed();
            if (!t.isEmpty() && t != QLatin1String("NULL"))
                modes << t;
        }
        vlsConfigs_.append(modes);
    }
    return !vlsConfigs_.isEmpty();
}

void KColdLightConfig::SetUserVLSConfig(int configIdx, int modeIdx)
{
    // Реф. SetUserVLSConfig: зафиксировать выбранную комбинацию и режим в ней.
    if (configIdx < 0 || configIdx >= vlsConfigs_.size())
        return;
    userConfig_ = configIdx;
    const int n = vlsConfigs_[configIdx].size();
    userMode_ = (n > 0) ? qBound(0, modeIdx, n - 1) : 0;
}

QString KColdLightConfig::CurrentMode() const
{
    if (userConfig_ < 0 || userConfig_ >= vlsConfigs_.size())
        return QString();
    const QStringList &combo = vlsConfigs_[userConfig_];
    if (userMode_ < 0 || userMode_ >= combo.size())
        return QString();
    return combo[userMode_];
}

bool KColdLightConfig::LoadCommPara(const QString &commParaIniPath)
{
    commPara_.clear();
    QSettings ini(commParaIniPath, QSettings::IniFormat);
    ini.beginGroup("V01");
    // Ключи "<model>\<mode>" → QSettings трактует '\' как разделитель групп:
    // childGroups() = модели, внутри childKeys() = режимы; значение — список 26 float.
    for (const QString &model : ini.childGroups()) {
        ini.beginGroup(model);
        for (const QString &mode : ini.childKeys()) {
            QVector<float> vals;
            for (const QString &tok : ini.value(mode).toStringList()) {
                bool ok = false; const float v = tok.trimmed().toFloat(&ok);
                if (ok) vals.append(v);
            }
            if (!vals.isEmpty())
                commPara_.insert(model + "/" + mode, vals);
        }
        ini.endGroup();
    }
    ini.endGroup();
    return !commPara_.isEmpty();
}

QVector<float> KColdLightConfig::GetLightParam(const QString &lightModel,
                                               const QString &mode) const
{
    return commPara_.value(lightModel + "/" + mode);
}

// =================== Параметры авто-диммирования (_KAutomaticDimmerParam) ===================
// В бинарнике Get/SetCameraManuDimmerParam и Get/SetAutomaticDimmerParam — ПОБАЙТОВЫЕ КЛОНЫ,
// отличающиеся ровно одной инструкцией (длина строки имени файла). Поэтому здесь общие тела
// параметризованы именем файла — наблюдаемое поведение идентично реф.
namespace {

const char *const kCamera2aFile = "coldlightCamera2aPara.ini";
const char *const kCommParaFile = "coldlightCommPara.ini";

// Реф.-пролог, общий для всех четырёх методов.
QString dimmerKey(QString endoModel, QString lightMode)
{
    if (endoModel.indexOf("/") != -1)                 // '/' ломает путь ключа QSettings
        endoModel.replace("/", "__", Qt::CaseSensitive);
    if (endoModel.isEmpty())
        endoModel = QStringLiteral("DefaultParam");   // подстановка ТОЛЬКО при пустом аргументе
    if (lightMode.isEmpty())
        lightMode = QStringLiteral("default");
    return QStringLiteral("V01/") + endoModel + "/" + lightMode;
}

QString dimmerFile(const char *name)
{
    return QDir(KSystem::ColdlightConfigPath()).absoluteFilePath(QString::fromLatin1(name));
}

void readDimmerParam(const char *file, QString endoModel, QString lightMode,
                     _KAutomaticDimmerParam &p)
{
    QSettings ini(dimmerFile(file), QSettings::IniFormat);
    const QStringList v = ini.value(dimmerKey(endoModel, lightMode), QString()).toStringList();
    if (v.count() < 26)   // реф. гейт: требуется > 25; иначе лог и ВЫХОД без записи в param
        return;
    auto f = [&](int i) { return v.at(i).trimmed().toFloat(); };
    p.f00 = f(0);  p.f04 = f(1);  p.f08 = f(2);  p.f0c = f(3);  p.f10 = f(4);  p.f14 = f(5);
    p.f18 = f(6);  p.f1c = f(7);  p.f20 = f(8);  p.f24 = f(9);  p.f28 = f(10); p.f2c = f(11);
    p.f30 = f(12); p.f34 = f(13); p.f38 = f(14); p.f3c = f(15); p.f40 = f(16); p.f44 = f(17);
    p.i48 = static_cast<qint8>(v.at(18).trimmed().toInt());     // знаковые (реф. toInt)
    p.i49 = static_cast<qint8>(v.at(19).trimmed().toInt());
    p.u4a = static_cast<quint8>(v.at(20).trimmed().toUInt());   // беззнаковый (реф. toUInt)
    p.f4c = f(21);
    p.f50 = f(22);
    // ПЕРЕСТАВЛЕННЫЙ ХВОСТ (квирк реф.): 23→0x58, 24→0x5c, 25→0x54.
    p.f58 = f(23);
    p.f5c = f(24);
    p.f54 = f(25);
}

void writeDimmerParam(const char *file, QString endoModel, QString lightMode,
                      const _KAutomaticDimmerParam &p)
{
    // Реф. форматирование: float → QString("%1").arg(double, 0, 'g', -1, ' ');
    // три целых поля → QString("%1").arg(qlonglong, 0, 10, ' '). Разделитель ", " даёт
    // сам QSettings при сериализации QStringList.
    auto fs = [](float x) { return QString("%1").arg(static_cast<double>(x), 0, 'g', -1, QLatin1Char(' ')); };
    auto is = [](qlonglong x) { return QString("%1").arg(x, 0, 10, QLatin1Char(' ')); };
    QStringList v;
    v << fs(p.f00) << fs(p.f04) << fs(p.f08) << fs(p.f0c) << fs(p.f10) << fs(p.f14)
      << fs(p.f18) << fs(p.f1c) << fs(p.f20) << fs(p.f24) << fs(p.f28) << fs(p.f2c)
      << fs(p.f30) << fs(p.f34) << fs(p.f38) << fs(p.f3c) << fs(p.f40) << fs(p.f44)
      << is(p.i48) << is(p.i49) << is(p.u4a)
      << fs(p.f4c) << fs(p.f50)
      << fs(p.f58) << fs(p.f5c) << fs(p.f54);   // та же перестановка → round-trip сходится
    QSettings ini(dimmerFile(file), QSettings::IniFormat);
    ini.setValue(dimmerKey(endoModel, lightMode), v);
}

} // namespace

void KColdLightConfig::GetCameraManuDimmerParam(QString endoModel, QString lightMode,
                                                _KAutomaticDimmerParam &param) const
{
    readDimmerParam(kCamera2aFile, endoModel, lightMode, param);
}

void KColdLightConfig::SetCameraManuDimmerParam(QString endoModel, QString lightMode,
                                                _KAutomaticDimmerParam param) const
{
    writeDimmerParam(kCamera2aFile, endoModel, lightMode, param);
}

void KColdLightConfig::GetAutomaticDimmerParam(QString endoModel, QString lightMode,
                                               _KAutomaticDimmerParam &param) const
{
    readDimmerParam(kCommParaFile, endoModel, lightMode, param);
}

void KColdLightConfig::SetAutomaticDimmerParam(QString endoModel, QString lightMode,
                                               _KAutomaticDimmerParam param) const
{
    writeDimmerParam(kCommParaFile, endoModel, lightMode, param);
}
