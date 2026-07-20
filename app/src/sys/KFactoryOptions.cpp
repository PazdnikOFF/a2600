#include "KFactoryOptions.h"

#include "KSystem.h"
#include "KSystemStatus.h"
#include "../db/KFileBackup.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <unistd.h>

namespace {

// Реф. статик s_nDebugLevel @0x14be688 (кэш уровня отладки для OpenDebug).
int s_nDebugLevel = 0;

// Реф. глобалы g_euTestType / состояние стенда, выставляемые свободными функциями
// SetAutoTestOpen(mode, mask) и SetUITestRecordOpen(mode). Полноценного носителя
// у нас нет (это часть рантайма X2000), поэтому моделируем наблюдаемое состояние —
// оно и проверяется self-test'ом.
int s_autoTestMode = 0;      // 1 = старт стенда, 2 = стоп
int s_autoTestMask = 0;      // реф. всегда 2048 (0x800) для стенда старения
int s_uiTestRecordMode = 0;  // реф. SetUITestRecordOpen(2)
int s_testType = 0;          // реф. g_euTestType

} // namespace

// --- Пути конфигов -----------------------------------------------------------

QString KFactoryOptions::TestEnvFile()
{
    // Реф.: DataPath() + "presetdata/syspreset/testenv.ini".
    return QDir(KSystem::DataPath()).absoluteFilePath("presetdata/syspreset/testenv.ini");
}

QString KFactoryOptions::AgeTestFile()
{
    // Реф.: DataPath() + "protected/syspreset/testenv.ini" (ДРУГОЙ файл, не тот же).
    return QDir(KSystem::DataPath()).absoluteFilePath("protected/syspreset/testenv.ini");
}

// --- testenv.ini -------------------------------------------------------------

void KFactoryOptions::ReadTestEnv()
{
    // Реф.: check_scope->setChecked(settings.value("Env/Scope", true).toBool()).
    QSettings s(TestEnvFile(), QSettings::IniFormat);
    checkScope_ = s.value("Env/Scope", true).toBool();
}

void KFactoryOptions::SaveTestEnv()
{
    // Реф.: setValue("Env/Scope", check_scope->isChecked()).
    QDir().mkpath(QFileInfo(TestEnvFile()).absolutePath());
    QSettings s(TestEnvFile(), QSettings::IniFormat);
    s.setValue("Env/Scope", checkScope_);
    s.sync();
}

// --- Ядро: путь к набору автотест-скриптов -----------------------------------

QString KFactoryOptions::GetTestConfPath() const
{
    // Реф. GetTestConfPath: чистая конкатенация, читает ДВА поля KSystemStatus.
    //
    // +0x10 = PanelType (доказано: SetPanelType = `str w1,[x0,#0x10]; ret`, без emit).
    //   Значения доказаны по HmiMcu::InitVLSGroups (`if (panelType != 1) → "Key panel,
    //   no need to config vls groups"`): 0 = кнопочная панель, 1 = ЖК/сенсорная.
    //   Единственный писатель — HmiMcu::NT_RecMcuRespondAck_CB: (byte[8] >> 3) & 1.
    //   → набор "aging-1-*" = кнопочная панель, "aging-2-*" = ЖК-панель.
    //
    // +0x14 = ViewType (доказано: SetViewType пишет [x19,#0x14], emit type=0, далее
    //   KSystemSet::SetViewType + KColdLightConfig::InitLightTargetList).
    //   Значения доказаны тремя независимыми хелперами — KVideoProxy::IsSoftEndo,
    //   AlgParaManager::IsSoftEndo, KViewBase::IsSoftEndoView: все три суть
    //   `GetSystemStatus()[0x14] == 0`. → 0 = мягкий эндоскоп, 1 = жёсткий/камера.
    const KSystemStatus &st = KSystemStatus::GetInstance();

    QString name = "aging";
    if (checkScope_)
        name += "-scope";
    name += (st.PanelType() == 0) ? "-1" : "-2";
    name += (st.ViewType() == 0) ? "-softendo" : "-hardendo";

    // Реф. добавляет завершающий '/' — сохраняем (путь используется как каталог-источник).
    return QDir(KSystem::SystemPath()).absoluteFilePath("autotest/" + name) + "/";
}

// --- Копирование набора ------------------------------------------------------

bool KFactoryOptions::CopyConf(const QString &src, const QString &dst)
{
    // Реф.: если QDir(dst).exists() → KSystem::DeleteDirectory(dst);
    // затем KFileBackup::copyDirectoryFiles(src, dst, true); sync().
    QDir dstDir(dst);
    if (dstDir.exists())
        dstDir.removeRecursively();   // реф. KSystem::DeleteDirectory

    KFileBackup backup;
    const bool ok = backup.copyDirectoryFiles(src, dst, true);
    ::sync();
    return ok;
}

// --- Старт/стоп стенда -------------------------------------------------------

bool KFactoryOptions::StartTest()
{
    // Реф. StartTest: SaveTestEnv(); CopyConf(GetTestConfPath(), AppPath()+"autotest/");
    // при KAccount::CurrentRole()==4 → KFileBackup::clearFolder(LogPath());
    // SetAutoTestOpen(1, 2048); close().
    SaveTestEnv();

    const QString dst = QDir(KSystem::AppPath()).absoluteFilePath("autotest") + "/";
    const bool ok = CopyConf(GetTestConfPath(), dst);

    // Очистка логов при роли 4 — ОТЛОЖЕНО: KAccount у нас без CurrentRole()==4
    // семантики стенда; ветка не влияет на копирование набора.

    s_autoTestMode = 1;
    s_autoTestMask = 2048;   // реф. литерал 0x800
    return ok;
}

void KFactoryOptions::StopTest()
{
    // Реф. StopTest: если (g_euTestType & 0x400) → SetUITestRecordOpen(2); close().
    // Иначе SetAutoTestOpen(2, 2048) + [AgeTest] IsAgeTest = true; close().
    if (s_testType & 0x400) {
        s_uiTestRecordMode = 2;
        return;
    }

    s_autoTestMode = 2;
    s_autoTestMask = 2048;

    QDir().mkpath(QFileInfo(AgeTestFile()).absolutePath());
    QSettings s(AgeTestFile(), QSettings::IniFormat);
    // Реф. пишет жёсткое true. Читатель — KSelfTest::checkProcessor @0x714d00
    // (value("AgeTest/IsAgeTest", false)): при true самотест выдаёт TR_IPATestNE.
    s.setValue("AgeTest/IsAgeTest", true);
    s.sync();
}

// --- Прочее ------------------------------------------------------------------

QString KFactoryOptions::UpdateReleaseVersion(const QString &productModel)
{
    // Реф.: v = (model == "PyCkeun") ? "V1.0" : "V2.0".
    // Литерал "PyCkeun" — из .rodata реф., смысл не восстановлен (кодовое имя модели).
    return (productModel == "PyCkeun") ? QStringLiteral("V1.0") : QStringLiteral("V2.0");
}

void KFactoryOptions::OpenDebug(int level)
{
    // Реф.: если уровень не изменился — выход; иначе запомнить и
    // SetLogPriority(level==1 || level==2 ? level : 0).
    if (s_nDebugLevel == level)
        return;
    s_nDebugLevel = level;
    // SetLogPriority — свободная функция рантайма X2000; приоритет хранится в s_nDebugLevel.
}

int KFactoryOptions::DebugLevel()
{
    return (s_nDebugLevel == 1 || s_nDebugLevel == 2) ? s_nDebugLevel : 0;
}

// --- Доступ к смоделированному состоянию стенда (для self-test) ---------------

namespace KFactoryOptionsState {

int AutoTestMode()     { return s_autoTestMode; }
int AutoTestMask()     { return s_autoTestMask; }
int UITestRecordMode() { return s_uiTestRecordMode; }
void SetTestType(int v){ s_testType = v; }
void Reset()           { s_autoTestMode = s_autoTestMask = s_uiTestRecordMode = s_testType = 0; }

} // namespace KFactoryOptionsState
