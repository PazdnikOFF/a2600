#pragma once

#include <QString>
#include <QStringList>

#include "ui/KDialog.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QToolButton;

// Заводские опции / стенд старения (реф. KFactoryOptions, X-2600).
//
// В референсе это KDialog (sizeof 0x88, +0x50 Ui_KFactoryOptions*), т.е. диалог
// сервисного меню, доступный только ролям != 2 (реф. OpenFactoryOptions проверяет
// KAccount::CurrentRole()). Здесь реализован ТОЛЬКО off-device логический слой:
// конфиг testenv.ini, сборка пути к набору автотест-скриптов, копирование набора
// в рабочий каталог и коды возврата диалога. Виджеты не моделируются — состояние
// чекбоксов/комбобоксов вынесено в поля-сеттеры (см. SetCheckScope и др.).
//
// ОТЛОЖЕНО (device, реф.-адреса сохранены в docs/PROGRESS.md §10): весь блок
// восстановления — RecoverConfigure/startSystemRecovery/TimeTaskRecovery/
// RecScopeRestoreRsp/RecEndoInfoRec(int,int)/RecCameraInfoRec/ClickBtnClearEndoInfo
// (зовут GetEndoScope()/GetCamera()/GetKHalClass()), а также InitWidget и ctor.
//
// Конфиги (реф. QSettings::IniFormat; файлы создаются в рантайме, в поставке их НЕТ):
//   • DataPath() + "presetdata/syspreset/testenv.ini" — [Env] Scope (bool, дефолт true).
//     Читает ReadTestEnv, пишет SaveTestEnv.
//   • DataPath() + "protected/syspreset/testenv.ini"  — [AgeTest] IsAgeTest = true.
//     Пишется здесь (StopTest), а ЧИТАЕТСЯ KSelfTest::checkProcessor @0x714d00
//     (дефолт false): при true самотестирование добавляет в отчёт предупреждение
//     TR_IPATestNE. Т.е. это межмодульный флаг «прибор побывал на стенде старения»,
//     а не мёртвая запись.
// ⚠️ ОБНОВЛЕНО 2026-07-23 (ROADMAP C5 снят): коллизия имени разрешена СОВМЕЩЕНИЕМ —
// класс стал реф.-ДИАЛОГОМ, сохранив всю прежнюю off-device-логику без изменений.
// Разметка сверена дизасмом `Ui_KFactoryOptions::setupUi` @0x62bfa8 + `retranslateUi`
// @0x62b1c8 + ctor @0x629de8 + `InitWidget` @0x628f78:
//   KDialog, SetKStyle(2), resize 460×751, minimumSize (0,22), заголовок TR_FOptions
//   (ctor переопределяет заглушку uic «TR_Dlg»); главный gridLayout_5 margins(20,40,-,30);
//   4 QGroupBox: group_auto (TR_AgTest), group_recovery (TR_Rcvry), group_other (TR_Otr),
//   group_tools (TR_Tls) + вертикальный спейсер.
// Коды возврата ставятся НЕ через setResult/done, а прямой записью в поле +0x58 с
// последующим close() — в порте так же (SetResult + close).
// DEVICE-STUB: списки cmb_product*/cmb_version/cmb_style/cmd_prototypemode грузятся в реф.
// из конфигов и БД (LoadProductSeriesList/LoadProductList/…); здесь — инъекция сеттерами.
// Блок восстановления и всё, что дёргает GetEndoScope()/GetCamera()/GetKHalClass(), —
// заглушки (как и было задокументировано выше).
class KFactoryOptions : public KDialog
{
    Q_OBJECT
public:
    explicit KFactoryOptions(QWidget *parent = nullptr);

    // DEVICE-STUB инъекция рантайм-списков комбобоксов (в реф. — из конфигов/БД).
    void SetProductSeriesList(const QStringList &v);
    void SetProductModelList(const QStringList &v);
    void SetVersionList(const QStringList &v);
    void SetStyleList(const QStringList &v);
    void SetPrototypeModeList(const QStringList &v);

private slots:
    // Реф. слоты кнопок «инструментов»: записывают код в +0x58 и зовут close().
    void ClickBtnVideoCal();        // 0x11
    void ClickBtnSelfTest();        // 0x12
    void ClickBtnFunTest();         // 0x13
    void ClickBtnAlgParamAjust();   // 0x14
    void ClickBtnErrorRateTest();   // 0x15
    void ClickBtnTemperature();     // 0x16
    // Прочие реф. слоты (device-части — заглушки).
    void ClickBtnSaveOther();
    void ClickBtnAuthMachine();
    void ClickBtnClearEndoInfo();
    void ClickManuLock();
    void RecoverConfigure();
    void AndStyle();                // btn_loadstyle (реф. имя как есть)

private:
    void setupUi();      // реф. Ui_KFactoryOptions::setupUi @0x62bfa8
    void InitWidget();   // реф. @0x628f78 (device-условия — заглушки)

    QCheckBox   *check_pro = nullptr;
    QCheckBox   *check_scope = nullptr;       // ← зеркало поля checkScope_
    QCheckBox   *check_rec_pro = nullptr;
    QCheckBox   *check_rec_scope = nullptr;
    QToolButton *btn_start = nullptr;
    QToolButton *btn_stop = nullptr;
    QToolButton *btn_recovery = nullptr;
    QComboBox   *cmb_productseries = nullptr;
    QComboBox   *cmb_productmodel = nullptr;
    QComboBox   *cmd_prototypemode = nullptr;   // реф. опечатка («cmd_», не «cmb_»)
    QComboBox   *cmb_version = nullptr;
    QComboBox   *cmb_style = nullptr;
    QComboBox   *btn_debug = nullptr;           // реф. имя «btn_», но это QComboBox
    QLineEdit   *edt_sn = nullptr;
    QPushButton *btn_loadstyle = nullptr;
    QToolButton *btn_save_other = nullptr;
    QToolButton *btn_calibration = nullptr;
    QToolButton *btn_selftest = nullptr;
    QToolButton *btn_manu = nullptr;
    QToolButton *btn_errorRateTest = nullptr;
    QToolButton *btn_fun_test = nullptr;
    QToolButton *btn_algParamAjust = nullptr;
    QPushButton *btn_temperature = nullptr;
    QPushButton *btn_authmachine = nullptr;
    QPushButton *btn_clearEndo = nullptr;

public:
    // Коды возврата exec() (реф. поле +0x58). Реф. OpenFactoryOptions разбирает их
    // switch'ем и открывает соответствующий подчинённый диалог.
    enum ResultCode {
        RESULT_NONE           = 0,
        RESULT_VIDEO_CAL      = 0x11,   // OpenVideoCal
        RESULT_SELF_TEST      = 0x12,   // OpenKSelfTestDlg
        RESULT_FUN_TEST       = 0x13,   // OpenFunTest
        RESULT_ALG_PARAM      = 0x14,   // OpenKAlgParamAjustDlg
        RESULT_ERROR_RATE     = 0x15,   // OpenKErrorRateDlg
        RESULT_TEMPERATURE    = 0x16,   // OpenSystemTempratureDlg
    };

    // --- Состояние UI ---
    // Теперь это НАСТОЯЩИЙ ui->check_scope; поле checkScope_ оставлено единственным
    // источником истины (вся off-device-логика ниже не менялась), а чекбокс — его зеркало.
    void SetCheckScope(bool v);
    bool CheckScope() const { return checkScope_; }

    // --- Конфиг testenv.ini ---
    // Реф. ReadTestEnv: check_scope->setChecked(value("Env/Scope", true).toBool()).
    void ReadTestEnv();
    // Реф. SaveTestEnv: setValue("Env/Scope", check_scope->isChecked()).
    void SaveTestEnv();

    // Путь к файлу [Env] (DataPath()+"presetdata/syspreset/testenv.ini").
    static QString TestEnvFile();
    // Путь к файлу [AgeTest] (DataPath()+"protected/syspreset/testenv.ini").
    static QString AgeTestFile();

    // --- Ядро: набор автотест-скриптов ---
    // Реф. GetTestConfPath — конкатенация:
    //   SystemPath() + "autotest/" + "aging"
    //     + (check_scope ? "-scope" : "")
    //     + (lowLight == 0 ? "-1" : "-2")
    //     + (endoHardType == 0 ? "-softendo" : "-hardendo") + "/"
    // Сверено с поставкой: из 8 комбинаций реально существуют 6 (нет *-1-hardendo).
    QString GetTestConfPath() const;

    // Реф. CopyConf: если dst существует — KSystem::DeleteDirectory(dst),
    // затем KFileBackup::copyDirectoryFiles(src, dst, true) и sync().
    static bool CopyConf(const QString &src, const QString &dst);

    // Реф. StartTest: SaveTestEnv(); CopyConf(GetTestConfPath(), AppPath()+"autotest/");
    // при роли 4 — clearFolder(LogPath()); SetAutoTestOpen(1, 2048); close().
    bool StartTest();

    // Реф. StopTest: если g_euTestType & 0x400 → SetUITestRecordOpen(2), close().
    // Иначе SetAutoTestOpen(2, 2048) + запись [AgeTest] IsAgeTest=true, close().
    void StopTest();

    // --- Прочая off-device-логика ---
    // Реф. UpdateReleaseVersion: модель "PyCkeun" → "V1.0", иначе "V2.0".
    static QString UpdateReleaseVersion(const QString &productModel);

    // Реф. OpenDebug: если уровень изменился — запомнить и SetLogPriority(
    // n==1||n==2 ? n : 0). Статик s_nDebugLevel @0x14be688.
    static void OpenDebug(int level);
    static int  DebugLevel();

    // Реф. exec(): QDialog::exec(), возврат поля +0x58.
    int  Result() const { return result_; }
    void SetResult(int v) { result_ = v; }

private:
    bool checkScope_ = true;   // ui->check_scope (+ дефолт ключа Env/Scope)
    int  result_ = RESULT_NONE;   // +0x58
};

// Наблюдаемое состояние стенда. В референсе это ГЛОБАЛЫ рантайма X2000
// (g_euTestType) и свободные функции SetAutoTestOpen(mode, mask) /
// SetUITestRecordOpen(mode); носителя у нас нет, поэтому состояние
// смоделировано — нужно, чтобы StartTest/StopTest были проверяемы.
namespace KFactoryOptionsState {
int  AutoTestMode();       // 1 = старт, 2 = стоп
int  AutoTestMask();       // реф. всегда 2048 (0x800)
int  UITestRecordMode();   // реф. SetUITestRecordOpen(2)
void SetTestType(int v);   // реф. g_euTestType (бит 0x400 → ветка UI-записи)
void Reset();
} // namespace KFactoryOptionsState
