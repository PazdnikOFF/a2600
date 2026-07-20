#pragma once

#include <QString>
#include <QRect>
#include <QSize>
#include <QPixmap>
#include <QIcon>
#include <QMap>

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
    // Запись области в layout-ini (QSettings сериализует QRect как @Rect(x y w h)).
    void  SetRect(const QString &section, const QString &key, const QRect &r) const;

    // Именованные области как в оригинале
    QRect getVideoRectForUI(int mode = 0) const;   // [UI]/IMAGE (0) или [VIDEO]/IMAGE
    QRect getVideoRectForImgPro() const;           // [VIDEO]/IMAGE
    QRect getFreezeVideoRect() const;              // [VIDEO]/IMAGE_PIP (freeze/PIP)
    QRect GetKImgListCellRect() const;             // [KImgList]/tablecell
    QRect GetKImgListIconRect() const;             // [KImgList]/tableicon

    // Запись прямоугольников области видео (реф. KDisplayOption::setVideoRectFor*).
    // Калибровка (KVideoCal::SaveDisplayArea) пишет их в layout-ini.
    void  setVideoRectForImgPro(const QRect &r) const;  // [VIDEO]/IMAGE
    void  setVideoRectForUI(const QRect &r) const;      // [UI]/IMAGE

    // Раскладка рабочего стола [KUIDesktop] → карта имя→QRect (реф.
    // GetSoftEndoViewConf/GetHardEndoViewConf). hardEndo=false — набор для
    // софт-эндоскопа (logo/workmode/status/patientinfo/…), true — для аппаратного
    // (osd/time/topmsg/…). Возвращает только присутствующие в ini ключи.
    QMap<QString, QRect> GetDesktopViewConf(bool hardEndo) const;

    // Модель/бренд (стиль). Бренд РуСкейн = PyCkeun (stylelist.ini).
    void    SetProduct(const QString &model, const QString &brand);
    QString Model() const { return model_; }
    QString Brand() const { return brand_; }

    // Пути конфигов эндоскопа (реф. KDisplayOption)
    QString GetSoftEndoDisplayPath() const;        // .../style/<model>/<brand>/scope
    QString GetSoftEndoViewConf() const;           // .../scope/video.ini

    // Пути стиля/тем
    QString GetStyleQssPath() const;               // <root>/qss/black/style.qss
    // Реф. KDisplayOption::GetThemeQssPath(QString) — СТАТИЧЕСКИЙ, возвращает
    // ФАЙЛОВЫЙ путь внутри текущей темы (к результату применяется QFile::exists).
    // У нас тема одна («black») ⇒ <root>/qss/black/<rel>.
    static QString GetThemeQssPath(const QString &rel);

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
