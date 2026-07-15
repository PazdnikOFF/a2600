#include "sys/KUpdateManifest.h"
#include "sys/KSystem.h"
#include "sys/KVersionConfig.h"
#include "sys/KUpdateConf.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QRegularExpression>

KUpdateManifest &KUpdateManifest::GetInstance()
{
    static KUpdateManifest inst;
    return inst;
}

const QStringList &KUpdateManifest::UpdateItems()
{
    // 1:1 с KUpdateAction::CheckUpdateItems (X2000): app/hmi/pap/papp00..80/lcd.
    static const QStringList items = {
        "app", "hmi", "pap",
        "papp00", "papp01", "papp02", "papp03", "papp04", "papp06", "papp07", "papp80",
        "lcd"};
    return items;
}

QString KUpdateManifest::GetUpdateRoot() const
{
    if (!updateRoot_.isEmpty())
        return updateRoot_;
    // реф.: RootPath() + "data/update/update/" → DataPath()/update/update.
    return QDir(KSystem::DataPath()).absoluteFilePath("update/update");
}

QString KUpdateManifest::ManifestFile() const
{
    return QDir(GetUpdateRoot()).absoluteFilePath("update.ini");
}

bool KUpdateManifest::GetItemNeedUpdate(const QString &item) const
{
    QSettings ini(ManifestFile(), QSettings::IniFormat);
    // реф.: ключ "%1/IsNeedUpdate" со значениями "TRUE"/"FALSE".
    return ini.value(item + "/IsNeedUpdate").toString().compare("TRUE", Qt::CaseInsensitive) == 0;
}

void KUpdateManifest::SetItemNeedUpdate(const QString &item, bool need) const
{
    QSettings ini(ManifestFile(), QSettings::IniFormat);
    ini.setValue(item + "/IsNeedUpdate", need ? "TRUE" : "FALSE");
    ini.sync();
}

QString KUpdateManifest::GetPackageVersion(const QString &item) const
{
    QSettings ini(ManifestFile(), QSettings::IniFormat);
    return ini.value(item + "/Version").toString();
}

QString KUpdateManifest::CalcFileMd5(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&f))
        return QString();
    return QString::fromLatin1(hash.result().toHex());
}

QString KUpdateManifest::ReadFileMd5(const QString &md5File, const QString &targetName)
{
    QFile f(md5File);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    const QString content = QString::fromUtf8(f.readAll());
    // Формат md5sum: "<32-hex>  <имя>" (два пробела) на строку.
    for (const QString &line : content.split('\n', QString::SkipEmptyParts)) {
        const QString t = line.trimmed();
        const int sp = t.indexOf(QRegularExpression("\\s+"));
        if (sp <= 0)
            continue;
        const QString md5 = t.left(sp);
        const QString name = t.mid(sp).trimmed();
        if (name == targetName || QFileInfo(name).fileName() == targetName)
            return md5.toLower();
    }
    return QString();
}

bool KUpdateManifest::CheckFileMd5(const QString &filePath, const QString &md5File)
{
    const QString calc = CalcFileMd5(filePath);
    if (calc.isEmpty())
        return false;
    const QString expected = ReadFileMd5(md5File, QFileInfo(filePath).fileName());
    return !expected.isEmpty() && calc.compare(expected, Qt::CaseInsensitive) == 0;
}

KUpdateManifest::ItemStatus KUpdateManifest::DecideItem(const QString &installed,
                                                        const QString &pkg,
                                                        const QStringList &matched)
{
    // реф. KUpdateAction::UpdateCheck: нет версии пакета — компонента нет в пакете;
    // версия пакета вне matched-списка — несовместима; совпала с установленной —
    // актуально; иначе — обновляем.
    if (pkg.isEmpty())
        return NoPackage;
    if (!matched.isEmpty() && !matched.contains(pkg))
        return Incompatible;
    if (pkg == installed)
        return UpToDate;
    return NeedUpdate;
}

QMap<QString, KUpdateManifest::ItemStatus> KUpdateManifest::CheckUpdateItems() const
{
    QMap<QString, ItemStatus> out;
    KVersionConfig &ver = KVersionConfig::GetInstance();
    KUpdateConf &conf = KUpdateConf::GetInstance();

    for (const QString &item : UpdateItems()) {
        const QString pkg = GetPackageVersion(item);
        const ItemStatus st = DecideItem(ver.GetVersion(item), pkg, conf.GetMatchedVersion(item));
        out.insert(item, st);
        // Помечаем в манифесте, требуется ли обновление (реф. SetItemNeedUpdate).
        SetItemNeedUpdate(item, st == NeedUpdate);
    }
    return out;
}
