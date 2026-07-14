#include "ui/Theme.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>

namespace theme {

QString root()
{
    const QString env = QProcessEnvironment::systemEnvironment().value("ENDO_ROOT");
    if (!env.isEmpty())
        return env;
    return "/home/root"; // путь на устройстве
}

QString asset(const QString &relToQss)
{
    return QDir(root()).absoluteFilePath("qss/" + relToQss);
}

QString brand(const QString &file)
{
    return QDir(root()).absoluteFilePath("system/style/X-2600/PyCkeun/qss/" + file);
}

QString loadStyleSheet()
{
    QFile f(root() + "/qss/black/style.qss");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QString css = QString::fromUtf8(f.readAll());
    // url(qss/black/...) → абсолютный путь к файлам прошивки
    css.replace("qss/black/", (root() + "/qss/black/").toUtf8());
    return css;
}

} // namespace theme
