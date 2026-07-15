#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

// Манифест пакета обновления + логика решения «что обновлять» (реф. update-сторона
// KUpdateConf / KUpdateAction, X-2600).
//
// В оригинале это Qt-оркестратор (KUpdateMng/KUpdateAction/KUpdatePrepare — диалоги
// с прогресс-баром, распаковкой rar и прошивкой железа). Off-device ядро вынесено сюда:
//   • канонический список обновляемых компонентов (реф. CheckUpdateItems: app/hmi/pap/
//     papp00..80/lcd);
//   • чтение/запись манифеста update.ini (<updateRoot>/update.ini, ключ
//     [<item>]/IsNeedUpdate = TRUE|FALSE — реф. KUpdateConf::SetItemNeedUpdate) и версий
//     пакета ([<item>]/Version);
//   • решение по компоненту (реф. KUpdateAction::UpdateCheck): сравнение версии пакета с
//     установленной (KVersionConfig) и проверка совместимости (KUpdateConf matchedversion).
//
// GetUpdateRoot() = RootPath()+"data/update/update/" (реф.), т.е. DataPath()/update/update.
// Устройство-зависимое (распаковка rar, ExecUpdCmd, прошивка) — Фаза E.
class KUpdateManifest
{
public:
    static KUpdateManifest &GetInstance();

    // Статус компонента (реф. NotifyOneItemUpdating/TheLatest/NotNeedUpdate).
    enum ItemStatus {
        UpToDate,      // версия пакета == установленной (обновление не требуется)
        NeedUpdate,    // версия пакета отличается и совместима — обновляем
        Incompatible,  // версия пакета не входит в matched-список — несовместима
        NoPackage,     // компонента нет в пакете
    };

    // Канонический список обновляемых компонентов (1:1 с CheckUpdateItems).
    static const QStringList &UpdateItems();

    // Корень распакованного пакета и файл манифеста.
    void    SetUpdateRoot(const QString &dir) { updateRoot_ = dir; }
    QString GetUpdateRoot() const;                 // .../data/update/update
    QString ManifestFile() const;                  // <updateRoot>/update.ini

    // Манифест: флаг «нужно обновить» ([<item>]/IsNeedUpdate = TRUE|FALSE).
    bool GetItemNeedUpdate(const QString &item) const;
    void SetItemNeedUpdate(const QString &item, bool need) const;
    // Версия компонента в пакете ([<item>]/Version).
    QString GetPackageVersion(const QString &item) const;

    // --- Проверка целостности файлов пакета (реф. KUpdateConf::*FileMd5Code) ---
    // MD5-хеш файла (hex, нижний регистр). В оригинале считается через HAL
    // (KHalClass::getUpdateFileChecksum) — off-device эквивалент QCryptographicHash::Md5.
    static QString CalcFileMd5(const QString &filePath);
    // Прочитать ожидаемый MD5 для targetName из md5-файла формата "<md5>  <имя>\n"
    // (реф. ReadFileMd5Code). "" если имя не найдено.
    static QString ReadFileMd5(const QString &md5File, const QString &targetName);
    // Совпадает ли посчитанный MD5 файла с записанным в md5File для его basename
    // (реф. CheckUpdateFileMd5Code).
    static bool CheckFileMd5(const QString &filePath, const QString &md5File);

    // Решение по одному компоненту (реф. UpdateCheck). installed — установленная версия,
    // pkg — версия пакета, matched — список совместимых версий ([] = без ограничения).
    static ItemStatus DecideItem(const QString &installed, const QString &pkg,
                                 const QStringList &matched);

    // Полный прогон: для всех UpdateItems() берёт установленную версию (KVersionConfig),
    // версию пакета (манифест), matched-список (KUpdateConf), считает статус и
    // проставляет IsNeedUpdate в манифесте. Возвращает карту item→status.
    QMap<QString, ItemStatus> CheckUpdateItems() const;

private:
    KUpdateManifest() = default;
    QString updateRoot_;
};
