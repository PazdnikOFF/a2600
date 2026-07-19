#include "sys/KEncStyle.h"
#include "sys/KStyleConfig.h"
#include "sys/KEncSettings.h"
#include "sys/KAccount.h"

#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QMap>

bool KEncStyle::Load(const QString &matchedScopeIniPath)
{
    if (!QFileInfo::exists(matchedScopeIniPath)) {
        iniPath_.clear();
        return false;
    }
    iniPath_ = matchedScopeIniPath;
    return true;
}

bool KEncStyle::LoadForStyle(const QString &series, const QString &brand)
{
    // Путь: <style>/<series>/<brand>/scope/matchedScope.ini (реф. getScopeInfoPath).
    series_ = series;   // тот же стиль питает video.ini-геттеры (getScopeInfoPath)
    brand_  = brand;
    const QString stylePath = KStyleConfig::GetInstance().GetStylePath(series, brand);
    return Load(QDir(stylePath).absoluteFilePath("scope/matchedScope.ini"));
}

QStringList KEncStyle::listFor(const QString &key) const
{
    if (iniPath_.isEmpty())
        return {};
    QSettings ini(iniPath_, QSettings::IniFormat);
    // Реф.: ключ "<Model>/<key>" если присутствует, иначе фолбэк "Default/<key>".
    const QString modelKey = productModel_ + "/" + key;
    const QString useKey = ini.contains(modelKey) ? modelKey : ("Default/" + key);
    // Нативный Qt-INI разбор списка по запятой (реф. QVariant::toStringList).
    return ini.value(useKey).toStringList();
}

QStringList KEncStyle::getSupportedScopeList() const
{
    return listFor("scope");
}

QStringList KEncStyle::GetSupprotedCameraList() const
{
    return listFor("camera");
}

bool KEncStyle::IsScopeValid(const QString &scope) const
{
    // Реф.-нормализация: "…LT…" вместе с "EC" → "…L/T…" (чистая строка).
    // ("*N"-суффикс и кросс-проверка с глобальным enc-списком getScopeList — device/отложено.)
    QString s = scope;
    if (s.contains("LT") && s.contains("EC"))
        s.replace("LT", "L/T");
    return getSupportedScopeList().contains(s, Qt::CaseSensitive);
}

bool KEncStyle::IsCameraValid(const QString &camera) const
{
    // Реф.: членство в списке камер (без нормализации), case-sensitive.
    return GetSupprotedCameraList().contains(camera, Qt::CaseSensitive);
}

// ============================ scope/video.ini ============================
// Таблицы строка→enum (реф. config2*Map, .bss, заполняются в _GLOBAL__sub_I_kencstyle.cpp).
namespace {

int lookupEnum(const QMap<QString, int> &m, const QString &key)
{
    return m.value(key, 0);   // реф.: несовпадение → 0
}

const QMap<QString, int> &sensorTypeMap()
{
    static const QMap<QString, int> m{
        {"OV2740", 0}, {"OH01A", 1}, {"IMX274", 2}, {"OV6946", 3}, {"OCHFA_OAH0428", 4}};
    return m;
}

const QMap<QString, int> &firmwareTypeMap()
{
    // КВИРК РЕФ. (сохранён): значение 6 ОТСУТСТВУЕТ, а 8/7 «перевёрнуты» —
    // OV2740_1024X1024 = 8, OCHFA_OAH0428_720X720 = 7.
    static const QMap<QString, int> m{
        {"OV2740", 0}, {"OH01A_928X768", 1}, {"IMX274", 2}, {"OH01A_768X928", 3},
        {"OV6946", 4}, {"OV2740_1280X960", 5}, {"OV2740_1024X1024", 8},
        {"OCHFA_OAH0428_720X720", 7}};
    return m;
}

const QMap<QString, int> &shapeTypeMap()
{
    static const QMap<QString, int> m{
        {"OCTANGLE_AND_ROUND", 0}, {"OCTANGLE_ONLY", 1}, {"ROUND_ONLY", 2}};
    return m;
}

const QMap<QString, int> &endoTypeMap()
{
    // 17 значений, тождественная нумерация 0..16 в порядке реф.
    static const QStringList names{
        "OV2740_EC_1504X1080", "OV2740_EG_1504X1080", "OV2740_EC_1280X960",
        "OV2740_EG_1280X960", "OV2740_ED_1024X1024", "OV2740_VL_1280X960",
        "OV2740_VM_1504X1080", "OH01A_VS_928X768", "IMX274_1920X1080",
        "OH01A_ENL_768X928", "OH01A_EB_768X928", "OH01A_ECY_928X768",
        "OH01A_ECH_768X928", "OV6946_EUD_400X400", "OCHFA_OAH0428_EUC_720X720",
        "OCHFA_OAH0428_ECH_720X720", "OCHFA_OAH0428_ENL_720X720"};
    static const QMap<QString, int> m = [] {
        QMap<QString, int> t;
        for (int i = 0; i < names.size(); ++i)
            t.insert(names[i], i);
        return t;
    }();
    return m;
}

} // namespace

QString KEncStyle::ConvertSrc2Enc(const QString &src)
{
    // Реф. @0x667870: посимвольно "%02x" от unicode(); символ >= U+0100 кодируется как 0.
    QString out;
    for (const QChar &c : src) {
        const ushort u = c.unicode();
        out += QString::asprintf("%02x", u < 0x100 ? u : 0);
    }
    return out;
}

QString KEncStyle::getScopeInfoPath() const
{
    // Реф.: ProductSeriesStyleConfigPath(GetProductSeries()) + getCurrentStyle() + "/scope/".
    // У нас те же компоненты приходят явно (SetStyle/LoadForStyle) через KStyleConfig.
    return KStyleConfig::GetInstance().GetStylePath(series_, brand_) + "/scope/";
}

QVariant KEncStyle::scopeValue(const QString &scope, const char *key, bool fallback,
                               const QVariant &hardDefault) const
{
    const QString file = getScopeInfoPath() + "video.ini";
    QSettings ini(file, QSettings::IniFormat);
    const QString k = QString::fromLatin1(key);
    // 1-й уровень: [Default]/<key> с НЕвалидным дефолтом (или зашитый дефолт, если фолбэка нет).
    QVariant def = hardDefault;
    if (fallback)
        def = ini.value("Default/" + k, hardDefault);
    // 2-й уровень: <hex>/<key> с результатом первого уровня в качестве дефолта.
    return ini.value(ConvertSrc2Enc(scope) + "/" + k, def);
}

QRect KEncStyle::getScopeSize(const QString &scope) const
{
    return scopeValue(scope, "videoSize", true).toRect();
}

int KEncStyle::GetEndoSensorType(const QString &scope) const
{
    return lookupEnum(sensorTypeMap(), scopeValue(scope, "sensorType", true).toString());
}

int KEncStyle::GetFirmwareType(const QString &scope) const
{
    return lookupEnum(firmwareTypeMap(), scopeValue(scope, "firmwareType", true).toString());
}

int KEncStyle::GetEndoType(const QString &scope) const
{
    return lookupEnum(endoTypeMap(), scopeValue(scope, "endoType", true).toString());
}

int KEncStyle::getScopeRotateType(const QString &scope) const
{
    return scopeValue(scope, "rotateType", true).toInt();
}

int KEncStyle::getScopeDefaultRoundCut(const QString &scope) const
{
    return scopeValue(scope, "defaultRoundCut", true).toInt();
}

int KEncStyle::getScopeDefaultOctangleCut(const QString &scope) const
{
    return scopeValue(scope, "defaultOctangleCut", true).toInt();
}

int KEncStyle::GetEndoShapeType(const QString &scope) const
{
    // Реф.: БЕЗ [Default]; зашитый дефолт — строка "OCTANGLE_AND_ROUND" (→ 0).
    const QString s = scopeValue(scope, "shapeType", false,
                                 QStringLiteral("OCTANGLE_AND_ROUND")).toString();
    return lookupEnum(shapeTypeMap(), s);
}

float KEncStyle::GetScopeZoomRatio(const QString &scope) const
{
    // Реф.: БЕЗ [Default]; зашитый дефолт 1.0 (строится как double, читается toFloat()).
    return scopeValue(scope, "zoomRatio", false, 1.0).toFloat();
}

KEncStyle::_SCOPE_PARA KEncStyle::getScopeParaDefault(const QString &scope) const
{
    // Реф.: один проход, 4 ключа, фолбэк [Default] у каждого.
    _SCOPE_PARA p;
    p.channelDiameter       = scopeValue(scope, "channelDiameter", true).toFloat();
    p.distalEndDiameter     = scopeValue(scope, "distalEndDiameter", true).toFloat();
    p.insertionTubeDiameter = scopeValue(scope, "insertionTubeDiameter", true).toFloat();
    // toUInt() → 16-битное поле: значения > 65535 обрезаются (реф. bfi #32,#16).
    p.workLength = static_cast<quint16>(scopeValue(scope, "workLength", true).toUInt());
    return p;
}

QString KEncStyle::getBiopsyImg(const QString &scope) const
{
    // Реф.: '/' удаляется из модели, затем каталог + hex-имя + ".png". ini НЕ читается.
    QString model = scope;
    model.replace("/", "");
    return getScopeInfoPath() + ConvertSrc2Enc(model) + ".png";
}

QString KEncStyle::GetEndoDisplayModel(const QString &scope) const
{
    // Реф.: гейт — роль <= 1 (RoleNone/RoleAdmin) И бренд "PyCkeun"; иначе модель как есть.
    if (KAccount::GetInstance().CurrentRole() > KAccount::RoleAdmin)
        return scope;
    if (brand_ != QLatin1String("PyCkeun"))
        return scope;
    static const QMap<QString, QString> kDisp{
        {"EG-X20", "G"}, {"EC-X20", "C"}, {"EC-X20L", "C"},
        {"EB-X20", "B"}, {"EB-X20T", "B"}};
    return kDisp.value(scope, scope);   // не в таблице → модель как есть
}

int KEncStyle::getScopeType(const QString &scope) const
{
    // Реф.: порядок проб фиксирован; сравнивается СЫРАЯ модель (не hex).
    static const struct { const char *file; int type; } kProbe[] = {
        {"cenc.ini", 1}, {"genc.ini", 0}, {"benc.ini", 2}, {"nlc.ini", 3},
        {"denc.ini", 4}, {"vetc.ini", 5}, {"cysc.ini", 6}, {"choc.ini", 7},
    };
    const QString dir = getScopeInfoPath();
    for (const auto &p : kProbe) {
        const QString path = dir + QString::fromLatin1(p.file);
        if (!QFileInfo::exists(path))
            continue;
        KEncSettings enc(path);
        if (enc.getStringList().contains(scope, Qt::CaseSensitive))
            return p.type;
    }
    return 0;   // не найдено (НЕОДНОЗНАЧНО с genc=0 — квирк реф.)
}
