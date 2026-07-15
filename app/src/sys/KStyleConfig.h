#pragma once

#include <QString>
#include <QStringList>
#include <QRect>

// Конфигурация стилей/брендов (реф. KEncStyle::getStyleList/getCurrentStyle* +
// KStyleFactory + stylelist.ini, X-2600). Бренды перечислены в
// system/style/stylelist.ini [Style]StyleList (порядок фиксирован — сохраняется в MCU).
// Ассеты бренда: system/style/<SERIES>/<BRAND>/ (qss/scope/…). Для X-2600/РуСкейн
// бренд = PyCkeun (см. KUIDesktop/Theme). Модель эндоскопа/камеры — device (KEncStyle).
class KStyleConfig
{
public:
    static KStyleConfig &GetInstance();

    void SetStyleRoot(const QString &path) { styleRoot_ = path; }  // .../system/style

    bool Load();                                  // читает stylelist.ini
    QStringList GetStyleList() const { return styleList_; }        // доступные бренды
    bool IsStyleValid(const QString &brand) const { return styleList_.contains(brand); }

    // Путь ассетов бренда: <styleRoot>/<series>/<brand> (реф. getCurrentStylePath).
    QString GetStylePath(const QString &series, const QString &brand) const;

    // --- Scope-info: <stylePath>/scope/video.ini (реф. KEncStyle::getScopeInfoPath +
    // getScopeDefault{Round,Octangle}Cut). Секции — hex-код имени скопа (ConvertSrc2Enc:
    // ASCII→hex, напр. "EC-X20"→"45432d583230"), есть [Default] как фолбэк. ---
    struct ScopeInfo {
        int roundCut = 0;        // defaultRoundCut — радиус круга (b для SetCornerCutWay way0)
        int octangleCut = 0;     // defaultOctangleCut — упаковка (p2<<16)|p3 (way1)
        QString shapeType;       // OCTANGLE_AND_ROUND / ROUND_ONLY / OCTANGLE_ONLY
        QString sensorType;      // OV2740 / OH01A / OCHFA_OAH0428 / OV6946 …
        QString firmwareType;    // OV2740_1280X960 …
        QString endoType;        // OV2740_EC_1504X1080 …
        QRect   videoSize;       // @Rect(x y w h)
        // Доп. поля video.ini (реф. KEncStyle::getScopeRotateType/GetScopeZoomRatio/
        // getScopeParaDefault/getIsDefaultMatch — off-device по имени скопа).
        int     rotateType = 0;              // rotateType — поворот кадра (0/1/2)
        float   zoomRatio = 1.0f;            // zoomRatio (@Variant float)
        float   channelDiameter = 0.0f;      // channelDiameter (@Variant float, мм)
        float   distalEndDiameter = 0.0f;    // distalEndDiameter (@Variant float, мм)
        float   insertionTubeDiameter = 0.0f;// insertionTubeDiameter (@Variant float, мм)
        int     workLength = 0;              // workLength (мм)
        bool    defaultMatch = false;        // defaultMatch — «дефолтная» пара скоп/камера
        bool    valid = false;   // false — секции нет и [Default] пуст
    };
    // Полная запись по имени скопа (с фолбэком на [Default]).
    ScopeInfo GetScopeInfo(const QString &series, const QString &brand,
                           const QString &scope) const;
    // Значение выреза (реф. getScopeDefaultRoundCut/OctangleCut): per-scope ключ с
    // фолбэком на [Default]. way 0 = round, 1 = octangle.
    int GetScopeDefaultCut(const QString &series, const QString &brand,
                           const QString &scope, int way) const;
    // ConvertSrc2Enc: ASCII-строка → hex (нижний регистр, 2 символа на байт).
    static QString EncodeScopeName(const QString &scope);
    // Текущий бренд (по умолчанию — первый из списка; на устройстве — из настроек).
    QString GetCurrentStyle() const { return current_.isEmpty() && !styleList_.isEmpty()
                                              ? styleList_.first() : current_; }
    void SetCurrentStyle(const QString &brand) { current_ = brand; }

private:
    KStyleConfig() = default;
    QString styleRoot() const;
    QString styleRoot_;
    QStringList styleList_;
    QString current_;
};
