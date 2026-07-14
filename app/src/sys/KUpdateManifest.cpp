#include "sys/KUpdateManifest.h"
#include "sys/KSystem.h"
#include "sys/KVersionConfig.h"
#include "sys/KUpdateConf.h"

#include <QSettings>
#include <QDir>

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
