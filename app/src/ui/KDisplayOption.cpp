#include "ui/KDisplayOption.h"
#include "ui/Theme.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>
#include <QFileInfo>

KDisplayOption &KDisplayOption::Instance()
{
    static KDisplayOption inst;
    return inst;
}

void KDisplayOption::SelectLayout(const QSize &uiRes, const QSize &imgSize)
{
    const QString dir = KSystem::DisplayConfigPath();
    // Имя как в оригинале: IMG<imgW><imgH>-UI<uiW><uiH>.ini
    const QString uiTag = QString("UI%1%2").arg(uiRes.width()).arg(uiRes.height());

    if (imgSize.isValid()) {
        const QString f = QDir(dir).absoluteFilePath(
            QString("IMG%1%2-%3.ini").arg(imgSize.width()).arg(imgSize.height()).arg(uiTag));
        if (QFileInfo::exists(f)) { layoutFile_ = f; return; }
    }
    // Эндоскоп не подключён / размер неизвестен: берём любой IMG под это разрешение UI
    // (секция [KUIDesktop] одинакова во всех IMG-вариантах данного UI).
    auto matches = QDir(dir).entryList(
        {QString("IMG*-%1.ini").arg(uiTag)}, QDir::Files, QDir::Name);
    // Фолбэк на 1920×1080, если под текущее разрешение раскладок нет.
    if (matches.isEmpty() && uiTag != "UI19201080")
        matches = QDir(dir).entryList({"IMG*-UI19201080.ini"}, QDir::Files, QDir::Name);
    layoutFile_ = matches.isEmpty() ? QString()
                                    : QDir(dir).absoluteFilePath(matches.first());
}

QRect KDisplayOption::GetRect(const QString &section, const QString &key) const
{
    if (layoutFile_.isEmpty())
        return QRect();
    QSettings ini(layoutFile_, QSettings::IniFormat);
    return ini.value(section + "/" + key).toRect();  // @Rect(x y w h) → QRect
}

void KDisplayOption::SetRect(const QString &section, const QString &key, const QRect &r) const
{
    if (layoutFile_.isEmpty())
        return;
    QSettings ini(layoutFile_, QSettings::IniFormat);
    ini.setValue(section + "/" + key, r);  // QRect → @Rect(x y w h)
    ini.sync();
}

QRect KDisplayOption::getVideoRectForUI(int mode) const
{
    return GetRect(mode == 0 ? "UI" : "VIDEO", "IMAGE");
}

QRect KDisplayOption::getVideoRectForImgPro() const { return GetRect("VIDEO", "IMAGE"); }
QRect KDisplayOption::getFreezeVideoRect()   const { return GetRect("VIDEO", "IMAGE_PIP"); }

void KDisplayOption::setVideoRectForImgPro(const QRect &r) const { SetRect("VIDEO", "IMAGE", r); }
void KDisplayOption::setVideoRectForUI(const QRect &r)     const { SetRect("UI", "IMAGE", r); }

QRect KDisplayOption::GetKImgListCellRect() const { return GetRect("KImgList", "tablecell"); }
QRect KDisplayOption::GetKImgListIconRect() const { return GetRect("KImgList", "tableicon"); }

QMap<QString, QRect> KDisplayOption::GetDesktopViewConf(bool hardEndo) const
{
    // Ключи как в реф. GetSoftEndoViewConf/GetHardEndoViewConf ([KUIDesktop]).
    static const QStringList softKeys = {
        "logo", "workmode", "status", "time", "imglist", "lefttime", "topmsg",
        "bottommsg", "connect", "patientinfo", "hospitalinfo", "endobtnguide"};
    static const QStringList hardKeys = {
        "osd", "time", "topmsg", "bottommsg", "connect", "lefttime", "endobtnguide"};

    QMap<QString, QRect> out;
    if (layoutFile_.isEmpty())
        return out;
    QSettings ini(layoutFile_, QSettings::IniFormat);
    for (const QString &k : (hardEndo ? hardKeys : softKeys)) {
        const QString path = "KUIDesktop/" + k;
        if (ini.contains(path))
            out.insert(k, ini.value(path).toRect());
    }
    return out;
}

void KDisplayOption::SetProduct(const QString &model, const QString &brand)
{
    model_ = model;
    brand_ = brand;
}

QString KDisplayOption::GetSoftEndoDisplayPath() const
{
    // .../system/style/<model>/<brand>/scope
    return QDir(KSystem::SystemPath())
        .absoluteFilePath(QString("style/%1/%2/scope").arg(model_, brand_));
}

QString KDisplayOption::GetSoftEndoViewConf() const
{
    return QDir(GetSoftEndoDisplayPath()).absoluteFilePath("video.ini");
}

QString KDisplayOption::GetStyleQssPath() const
{
    return QDir(theme::root()).absoluteFilePath("qss/black/style.qss");
}

QIcon KDisplayOption::GetIcon(const QString &relToBlack) const
{
    return QIcon(theme::asset("black/" + relToBlack));
}

QPixmap KDisplayOption::GetIconPixmap(const QString &relToBlack) const
{
    return QPixmap(theme::asset("black/" + relToBlack));
}

QPixmap KDisplayOption::GetOsdIconPixmap(const QString &name) const
{
    return QPixmap(theme::asset("black/osd/" + name));
}
