#pragma once

#include <QString>
#include <QRect>
#include <QSize>
#include <QPixmap>
#include <QIcon>

// Провайдер отображения (реф. класс KDisplayOption, X-2600).
// Выбирает layout-файл display/IMG%1%2-UI%3%4.ini по разрешению монитора
// (KSystem::GetSystemResolution) и размеру картинки эндоскопа, читает области
// через QSettings (@Rect → QRect) и резолвит иконки/qss-пути.
//
// Реальные методы: getVideoRectForUI, GetKImgListCellRect/IconRect,
// GetIcon/GetIconPixmap/GetOsdIconPixmap, GetStyleQssPath, GetSoftEndoDisplayPath.
class KDisplayOption
{
public:
    static KDisplayOption &Instance();

    // Выбор layout-файла по разрешению UI (монитор) и размеру изображения.
    // imgSize можно не задавать (десктоп без эндоскопа) — берётся любой IMG под UI.
    void SelectLayout(const QSize &uiRes, const QSize &imgSize = QSize());

    // Текущий размер картинки эндоскопа (из KSoftEndoParam при подключении).
    // По умолчанию 1280×960 — раскладка рабочего стола без эндоскопа.
    void  SetCurrentImageSize(const QSize &s) { currentImageSize_ = s; }
    QSize CurrentImageSize() const { return currentImageSize_; }

    QString LayoutFile() const { return layoutFile_; }

    // Универсальное чтение области: [section]/key = @Rect(x y w h)
    QRect GetRect(const QString &section, const QString &key) const;

    // Именованные области как в оригинале
    QRect getVideoRectForUI(int mode = 0) const;   // [UI]/IMAGE (0) или [VIDEO]/IMAGE
    QRect GetKImgListCellRect() const;             // [KImgList]/tablecell
    QRect GetKImgListIconRect() const;             // [KImgList]/tableicon

    // Модель/бренд (стиль). Бренд РуСкейн = PyCkeun (stylelist.ini).
    void    SetProduct(const QString &model, const QString &brand);
    QString Model() const { return model_; }
    QString Brand() const { return brand_; }

    // Пути конфигов эндоскопа (реф. KDisplayOption)
    QString GetSoftEndoDisplayPath() const;        // .../style/<model>/<brand>/scope
    QString GetSoftEndoViewConf() const;           // .../scope/video.ini

    // Пути стиля/тем
    QString GetStyleQssPath() const;               // <root>/qss/black/style.qss

    // Иконки (по логическому пути внутри qss/black)
    QIcon   GetIcon(const QString &relToBlack) const;
    QPixmap GetIconPixmap(const QString &relToBlack) const;
    QPixmap GetOsdIconPixmap(const QString &name) const;   // qss/black/osd/<name>

private:
    KDisplayOption() = default;
    QString layoutFile_;              // выбранный display/IMG*-UI*.ini
    QString model_ = "X-2600";       // модель продукта
    QString brand_ = "PyCkeun";      // бренд/стиль (РуСкейн)
    QSize   currentImageSize_ = QSize(1280, 960); // размер картинки эндоскопа
};
