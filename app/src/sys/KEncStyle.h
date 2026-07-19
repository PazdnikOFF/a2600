#pragma once

#include <QRect>
#include <QString>
#include <QStringList>
#include <QVariant>

// Слой совместимости эндоскопов/камер (реф. KEncStyle, X-2600 — matched-scope часть).
// Читает <style>/<series>/<brand>/scope/matchedScope.ini: для каждой модели продукта
// (секции [Default]/[X-2600]/[X-2600S]/[X-2600A]/[X-2600B]) списки поддержанных
// эндоскопов (`scope=A,B,C…`) и камер (`camera=X,Y…`). Секция выбирается по модели
// продукта (реф. KSystemSet::GetProductModel), фолбэк — [Default].
//
// Off-device-ядро: парсинг ini + сопоставление строк (case-sensitive). Отложено
// (device — живой эндоскоп): getIsDefaultMatch (video.ini + ConvertSrc2Enc(GetEndoModel)),
// нормализация суффикса "*N" (KEndoScope::IsEndoModelHaveSuffix), кросс-проверка
// IsScopeValid с глобальным enc-списком getScopeList (genc/cenc/…enc.ini).
class KEncStyle
{
public:
    // Загрузка matchedScope.ini напрямую по пути.
    bool Load(const QString &matchedScopeIniPath);
    // Загрузка по (серия, бренд) — путь строится через KStyleConfig::GetStylePath.
    bool LoadForStyle(const QString &series, const QString &brand);

    // Модель продукта = имя секции (реф. GetProductModel). По умолчанию X-2600.
    void    SetProductModel(const QString &model) { productModel_ = model; }
    QString ProductModel() const { return productModel_; }

    // Списки из секции [Model] с фолбэком на [Default] (реф. 1:1, имена+опечатка).
    QStringList getSupportedScopeList() const;    // [Model]/scope | [Default]/scope
    QStringList GetSupprotedCameraList() const;   // [Model]/camera | [Default]/camera

    // Валидация членством в поддержанном списке (case-sensitive, реф. Qt::CaseSensitive).
    // IsScopeValid нормализует "…LT…" c "EC" → "…L/T…" (чистая строка); кросс-проверка
    // с глобальным enc-списком и "*N"-суффикс (device) — отложены.
    bool IsScopeValid(const QString &scope) const;
    bool IsCameraValid(const QString &camera) const;

    bool IsLoaded() const { return !iniPath_.isEmpty(); }

    // ======================= scope/video.ini (per-эндоскоп) =======================
    // Реф.-геттеры полей video.ini по имени модели скопа. Секция = ConvertSrc2Enc(модель)
    // (hex), фолбэк — [Default] (ДВУХУРОВНЕВЫЙ: сперва читается Default/<key> с НЕвалидным
    // QVariant, результат идёт дефолтом для <hex>/<key>; нет Default → toInt()==0 и т.п.).
    // В реф. эти методы при ПУСТОМ аргументе подставляют GetEndoModel() (device) — у нас
    // аргумент обязателен, поэтому вся группа off-device.

    // Реф. ConvertSrc2Enc @0x667870: посимвольно sprintf("%02x", unicode()<0x100 ? unicode() : 0)
    // — нижний регистр; символ >= U+0100 даёт "00" (НЕ '?', как дал бы toLatin1()).
    static QString ConvertSrc2Enc(const QString &src);

    // Стиль для video.ini (реф. берёт из KSystemSet::GetProductSeries + getCurrentStyle).
    void SetStyle(const QString &series, const QString &brand) { series_ = series; brand_ = brand; }
    // Реф. getScopeInfoPath — КАТАЛОГ scope/ (не файл!); вызыватели сами дописывают "video.ini".
    QString getScopeInfoPath() const;

    // Таблицы строка→enum (реф. config2*Map в .bss; несовпадение → 0).
    enum _SENSOR_TYPE { SENSOR_OV2740 = 0, SENSOR_OH01A, SENSOR_IMX274, SENSOR_OV6946,
                        SENSOR_OCHFA_OAH0428 };
    enum _SHAPE_TYPE  { SHAPE_OCTANGLE_AND_ROUND = 0, SHAPE_OCTANGLE_ONLY, SHAPE_ROUND_ONLY };
    // Тип «прибора» по enc-спискам (реф. getScopeType): genc=0, cenc=1, benc=2, nlc=3,
    // denc=4, vetc=5, cysc=6, choc=7; не найдено → 0 (0 НЕОДНОЗНАЧЕН: гастроскоп и «не найдено»).
    enum _SCOPE_TYPE  { SCOPE_G = 0, SCOPE_C, SCOPE_B, SCOPE_NL, SCOPE_D, SCOPE_VET,
                        SCOPE_CYS, SCOPE_CHO };

    // Геометрия/идентификация (фолбэк [Default] есть).
    QRect getScopeSize(const QString &scope) const;              // videoSize
    int   GetEndoSensorType(const QString &scope) const;         // sensorType  → _SENSOR_TYPE
    int   GetFirmwareType(const QString &scope) const;           // firmwareType (см. квирк 8/7)
    int   GetEndoType(const QString &scope) const;               // endoType    (17 значений, 0..16)
    int   getScopeRotateType(const QString &scope) const;        // rotateType
    int   getScopeDefaultRoundCut(const QString &scope) const;   // defaultRoundCut
    int   getScopeDefaultOctangleCut(const QString &scope) const;// defaultOctangleCut

    // БЕЗ фолбэка [Default] — жёстко зашитые дефолты (реф.).
    int   GetEndoShapeType(const QString &scope) const;          // дефолт "OCTANGLE_AND_ROUND"(=0)
    float GetScopeZoomRatio(const QString &scope) const;         // дефолт 1.0

    // Реф. getScopeParaDefault — ОДИН проход, 4 ключа, упакованный POD (16 байт).
    // ВНИМАНИЕ: workLength читается toUInt() и хранится 16-битным (>65535 обрезается).
    struct _SCOPE_PARA {
        float   channelDiameter = 0.0f;
        float   distalEndDiameter = 0.0f;
        float   insertionTubeDiameter = 0.0f;
        quint16 workLength = 0;
    };
    _SCOPE_PARA getScopeParaDefault(const QString &scope) const;

    // Реф. getBiopsyImg — СБОРКА ПУТИ, ini не читает: '/' из модели удаляется,
    // затем getScopeInfoPath() + ConvertSrc2Enc(модель) + ".png".
    QString getBiopsyImg(const QString &scope) const;

    // Реф. GetEndoDisplayModel — ini НЕ читает: возвращает модель КАК ЕСТЬ, и только при
    // KAccount::CurrentRole() <= 1 (RoleNone/RoleAdmin) И бренде "PyCkeun" подменяет её
    // по таблице из 5 записей (EG-X20→G, EC-X20→C, EC-X20L→C, EB-X20→B, EB-X20T→B).
    // Реф. берёт стиль глобально (getCurrentStyle → system.ini Product/ProductStyle);
    // у нас — brand_ этого экземпляра (тот же смысл, задаётся SetStyle/LoadForStyle).
    QString GetEndoDisplayModel(const QString &scope) const;

    // Реф. getScopeType — video.ini НЕ читает: сканирует 8 enc-файлов в scope/ в фикс.
    // порядке через KEncSettings::getStringList(), сравнивая СЫРУЮ (не hex) модель.
    int getScopeType(const QString &scope) const;

private:
    QStringList listFor(const QString &key) const;   // <Model>/<key> | Default/<key>
    // Двухуровневое чтение video.ini (см. комментарий выше). fallback=false → без [Default].
    QVariant scopeValue(const QString &scope, const char *key, bool fallback,
                        const QVariant &hardDefault = QVariant()) const;

    QString iniPath_;
    QString productModel_ = "X-2600";
    QString series_;
    QString brand_;
};
