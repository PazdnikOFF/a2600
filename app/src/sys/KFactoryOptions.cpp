#include "KFactoryOptions.h"

#include "KSystem.h"
#include "KSystemStatus.h"
#include "../db/KFileBackup.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QSettings>
#include <QSpacerItem>
#include <QToolButton>

#include <unistd.h>

// ─────────────────────── Диалог (реф. Ui_KFactoryOptions) ───────────────────────

namespace {
QToolButton *MakeTool(QWidget *p, const char *name, const QString &text, bool wide = false)
{
    QToolButton *b = new QToolButton(p);
    b->setObjectName(QString::fromLatin1(name));
    b->setText(text);
    if (wide)
        b->setMinimumWidth(120);   // реф. min/max ширина 120 у кнопок стенда
    return b;
}

QLabel *Lbl(QWidget *p, const char *name, const QString &text)
{
    QLabel *l = new QLabel(text, p);
    l->setObjectName(QString::fromLatin1(name));
    return l;
}

QCheckBox *MakeCheck(QWidget *p, const char *name, const QString &text, bool checked = false)
{
    QCheckBox *c = new QCheckBox(text, p);
    c->setObjectName(QString::fromLatin1(name));
    c->setChecked(checked);
    return c;
}

QComboBox *MakeCombo(QWidget *p, const char *name)
{
    QComboBox *c = new QComboBox(p);
    c->setObjectName(QString::fromLatin1(name));
    return c;
}
}   // namespace

KFactoryOptions::KFactoryOptions(QWidget *parent)
    : KDialog(parent, false)
{
    // Реф. ctor @0x629de8: ui = new Ui_KFactoryOptions; setupUi; +0x58 = 0; SetKStyle(2);
    // затем setWindowTitle(tr("TR_FOptions")) — переопределяет заглушку uic «TR_Dlg».
    setObjectName(QStringLiteral("KFactoryOptions"));
    SetKStyle(KDLG_W460);
    SetTitle(QStringLiteral("TR_FOptions"));
    setupUi();
    InitWidget();
    result_ = RESULT_NONE;
}

void KFactoryOptions::setupUi()
{
    QWidget *root = ContentArea();
    QGridLayout *gridLayout_5 = new QGridLayout(root);
    gridLayout_5->setObjectName(QStringLiteral("gridLayout_5"));
    gridLayout_5->setContentsMargins(20, 40, 20, 30);   // реф. margins(20,40,-,30)

    // ── group_auto (TR_AgTest) ──
    QGroupBox *group_auto = new QGroupBox(QStringLiteral("TR_AgTest"), root);
    group_auto->setObjectName(QStringLiteral("group_auto"));
    QGridLayout *gridLayout = new QGridLayout(group_auto);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    gridLayout->setContentsMargins(12, 40, 12, 30);
    check_pro   = MakeCheck(group_auto, "check_pro", QStringLiteral("TR_IProcessor"), true);
    check_scope = MakeCheck(group_auto, "check_scope", QStringLiteral("TR_Escpe"), true);
    btn_start = MakeTool(group_auto, "btn_start", QStringLiteral("TR_Strt"), true);
    btn_stop  = MakeTool(group_auto, "btn_stop", QStringLiteral("TR_Stp"), true);
    gridLayout->addWidget(Lbl(group_auto, "label_env", QStringLiteral("TR_Envmnt:")), 0, 0);
    gridLayout->addWidget(check_pro, 0, 1, 1, 2);
    gridLayout->addWidget(check_scope, 1, 1, 1, 2);
    QHBoxLayout *hLayout_tool0 = new QHBoxLayout();
    hLayout_tool0->setObjectName(QStringLiteral("hLayout_tool0"));
    hLayout_tool0->addWidget(btn_start);
    hLayout_tool0->addWidget(btn_stop);
    gridLayout->addLayout(hLayout_tool0, 2, 0, 1, 3);
    gridLayout_5->addWidget(group_auto, 0, 0);

    // ── group_recovery (TR_Rcvry) ──
    QGroupBox *group_recovery = new QGroupBox(QStringLiteral("TR_Rcvry"), root);
    group_recovery->setObjectName(QStringLiteral("group_recovery"));
    QGridLayout *gridLayout_4 = new QGridLayout(group_recovery);
    gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
    gridLayout_4->setContentsMargins(20, 40, 20, 20);
    check_rec_pro   = MakeCheck(group_recovery, "check_rec_pro", QStringLiteral("TR_IProcessor"));
    check_rec_scope = MakeCheck(group_recovery, "check_rec_scope", QStringLiteral("TR_Escpe"));
    btn_recovery = MakeTool(group_recovery, "btn_recovery", QStringLiteral("TR_Strt"), true);
    // label_recovery — пустой в setupUi, реф. заполняет в рантайме (статус восстановления).
    gridLayout_4->addWidget(Lbl(group_recovery, "label_recovery", QString()), 0, 0, 1, 3);
    gridLayout_4->addWidget(check_rec_pro, 1, 0);
    gridLayout_4->addWidget(check_rec_scope, 1, 1);
    gridLayout_4->addWidget(btn_recovery, 1, 2);
    gridLayout_5->addWidget(group_recovery, 1, 0);

    // ── group_other (TR_Otr) ──
    QGroupBox *group_other = new QGroupBox(QStringLiteral("TR_Otr"), root);
    group_other->setObjectName(QStringLiteral("group_other"));
    QGridLayout *gridLayout_3 = new QGridLayout(group_other);
    gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
    gridLayout_3->setContentsMargins(10, 0, 0, 0);
    cmb_productseries = MakeCombo(group_other, "cmb_productseries");
    cmb_productmodel  = MakeCombo(group_other, "cmb_productmodel");
    cmd_prototypemode = MakeCombo(group_other, "cmd_prototypemode");   // реф. опечатка
    cmb_version       = MakeCombo(group_other, "cmb_version");
    cmb_style         = MakeCombo(group_other, "cmb_style");
    edt_sn = new QLineEdit(group_other);
    edt_sn->setObjectName(QStringLiteral("edt_sn"));
    btn_loadstyle = new QPushButton(QStringLiteral("TR_Impt"), group_other);
    btn_loadstyle->setObjectName(QStringLiteral("btn_loadstyle"));
    btn_save_other = MakeTool(group_other, "btn_save_other", QStringLiteral("TR_Sve"));
    gridLayout_3->addWidget(Lbl(group_other, "label_productseries", QStringLiteral("TR_PSSeries:")), 0, 0);
    gridLayout_3->addWidget(cmb_productseries, 0, 1);
    gridLayout_3->addWidget(Lbl(group_other, "label_productmodel", QStringLiteral("TR_PModel:")), 1, 0);
    gridLayout_3->addWidget(cmb_productmodel, 1, 1);
    gridLayout_3->addWidget(Lbl(group_other, "label_sn", QStringLiteral("TR_IPSN:")), 2, 0);
    gridLayout_3->addWidget(edt_sn, 2, 1);
    gridLayout_3->addWidget(Lbl(group_other, "label_2", QStringLiteral("TR_PMode2:")), 3, 0);
    gridLayout_3->addWidget(cmd_prototypemode, 3, 1);
    gridLayout_3->addWidget(Lbl(group_other, "label_version", QStringLiteral("TR_Rlse:")), 4, 0);
    gridLayout_3->addWidget(cmb_version, 4, 1);
    gridLayout_3->addWidget(Lbl(group_other, "label", QStringLiteral("TR_Stle:")), 5, 0);
    gridLayout_3->addWidget(cmb_style, 5, 1);
    gridLayout_3->addWidget(btn_loadstyle, 5, 2);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    horizontalLayout->addStretch();
    horizontalLayout->addWidget(btn_save_other);
    horizontalLayout->addStretch();
    gridLayout_3->addLayout(horizontalLayout, 6, 0, 1, 3);
    gridLayout_5->addWidget(group_other, 2, 0);

    // ── group_tools (TR_Tls). Разрежённый грид: строки 2,3,6 в реф. пустые. ──
    QGroupBox *group_tools = new QGroupBox(QStringLiteral("TR_Tls"), root);
    group_tools->setObjectName(QStringLiteral("group_tools"));
    QGridLayout *gridLayout_2 = new QGridLayout(group_tools);
    gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
    btn_calibration   = MakeTool(group_tools, "btn_calibration", QStringLiteral("TR_VCal"));
    btn_selftest      = MakeTool(group_tools, "btn_selftest", QStringLiteral("TR_STest"));
    btn_manu          = MakeTool(group_tools, "btn_manu", QStringLiteral("TR_ALock"));
    btn_errorRateTest = MakeTool(group_tools, "btn_errorRateTest", QStringLiteral("TR_BERatio"));
    btn_fun_test      = MakeTool(group_tools, "btn_fun_test", QStringLiteral("TR_FTest"));
    // Две подписи — ХАРДКОД-китайские, БЕЗ TR_-ключа (реф.).
    btn_algParamAjust = MakeTool(group_tools, "btn_algParamAjust", QString::fromUtf8("算法参数调节"));
    btn_temperature = new QPushButton(QString::fromUtf8("温度监测"), group_tools);
    btn_temperature->setObjectName(QStringLiteral("btn_temperature"));
    btn_authmachine = new QPushButton(QStringLiteral("TR_TMAuthorization"), group_tools);
    btn_authmachine->setObjectName(QStringLiteral("btn_authmachine"));
    btn_clearEndo = new QPushButton(QStringLiteral("TR_CEInfo"), group_tools);
    btn_clearEndo->setObjectName(QStringLiteral("btn_clearEndo"));
    gridLayout_2->addWidget(btn_calibration, 0, 0);
    gridLayout_2->addWidget(btn_selftest, 0, 1);
    gridLayout_2->addWidget(btn_manu, 1, 0);
    gridLayout_2->addWidget(btn_errorRateTest, 1, 1);
    gridLayout_2->addWidget(btn_fun_test, 4, 0);
    gridLayout_2->addWidget(btn_algParamAjust, 4, 1);
    gridLayout_2->addWidget(btn_temperature, 5, 0);
    gridLayout_2->addWidget(btn_authmachine, 5, 1);
    gridLayout_2->addWidget(btn_clearEndo, 7, 0, 1, 2);
    gridLayout_5->addWidget(group_tools, 3, 0);

    gridLayout_5->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 0);

    resize(460, 751);          // реф. resize(460, 751)
    setMinimumSize(0, 22);     // реф. setMinimumSize(0, 22)

    // ── connect'ы (реф. 23 в ctor) ──
    connect(btn_calibration, &QToolButton::clicked, this, &KFactoryOptions::ClickBtnVideoCal);
    connect(btn_selftest, &QToolButton::clicked, this, &KFactoryOptions::ClickBtnSelfTest);
    connect(btn_fun_test, &QToolButton::clicked, this, &KFactoryOptions::ClickBtnFunTest);
    connect(btn_algParamAjust, &QToolButton::clicked, this, &KFactoryOptions::ClickBtnAlgParamAjust);
    connect(btn_errorRateTest, &QToolButton::clicked, this, &KFactoryOptions::ClickBtnErrorRateTest);
    connect(btn_temperature, &QPushButton::clicked, this, &KFactoryOptions::ClickBtnTemperature);
    connect(btn_authmachine, &QPushButton::clicked, this, &KFactoryOptions::ClickBtnAuthMachine);
    connect(btn_clearEndo, &QPushButton::clicked, this, &KFactoryOptions::ClickBtnClearEndoInfo);
    connect(btn_manu, &QToolButton::clicked, this, &KFactoryOptions::ClickManuLock);
    connect(btn_save_other, &QToolButton::clicked, this, &KFactoryOptions::ClickBtnSaveOther);
    connect(btn_recovery, &QToolButton::clicked, this, &KFactoryOptions::RecoverConfigure);
    connect(btn_loadstyle, &QPushButton::clicked, this, &KFactoryOptions::AndStyle);
    connect(btn_start, &QToolButton::clicked, this, [this] { StartTest(); });
    connect(btn_stop, &QToolButton::clicked, this, [this] { StopTest(); });
    // check_scope — зеркало поля checkScope_ (единственный источник истины для логики).
    connect(check_scope, &QCheckBox::toggled, this, [this](bool v) { checkScope_ = v; });
    check_scope->setChecked(checkScope_);
    // Реф. также цепляет btn_debug(combo)→OpenDebug, cmb_productseries→LoadProductList,
    // cmb_style→UpdateReleaseVersion и сигналы GetEndoScope()/GetCamera() — device.
}

void KFactoryOptions::InitWidget()
{
    // Реф. @0x628f78. Воспроизводим то, что не требует устройства:
    // валидатор SN «[0-9]{0,10}».
    edt_sn->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("[0-9]{0,10}")), this));
    // Реф. далее: btn_debug (QComboBox, несмотря на имя) создаётся ТОЛЬКО при
    // GetProductAuthFlag()!=1 && !GetManuEnable() и наполняется «debug off/on/all on»;
    // по роли (<=3) прячутся btn_debug/btn_loadstyle/btn_fun_test/btn_algParamAjust/
    // btn_temperature/btn_clearEndo (через setVisible(false), НЕ setEnabled);
    // по GetSystemStatus() и GetEndoScope() гасятся btn_errorRateTest/btn_calibration/
    // check_pro. Всё это device-состояние — в порте не моделируется (виджеты видимы).
}

void KFactoryOptions::SetCheckScope(bool v)
{
    checkScope_ = v;
    if (check_scope)
        check_scope->setChecked(v);
}

void KFactoryOptions::SetProductSeriesList(const QStringList &v)
{
    cmb_productseries->clear();
    cmb_productseries->addItems(v);
}

void KFactoryOptions::SetProductModelList(const QStringList &v)
{
    cmb_productmodel->clear();
    cmb_productmodel->addItems(v);
}

void KFactoryOptions::SetVersionList(const QStringList &v)
{
    cmb_version->clear();
    cmb_version->addItems(v);
}

void KFactoryOptions::SetStyleList(const QStringList &v)
{
    cmb_style->clear();
    cmb_style->addItems(v);
}

void KFactoryOptions::SetPrototypeModeList(const QStringList &v)
{
    cmd_prototypemode->clear();
    cmd_prototypemode->addItems(v);
}

// Реф.: код возврата пишется ПРЯМО в поле +0x58, затем close() — не setResult/done/accept.
void KFactoryOptions::ClickBtnVideoCal()      { SetResult(RESULT_VIDEO_CAL);   close(); }
void KFactoryOptions::ClickBtnSelfTest()      { SetResult(RESULT_SELF_TEST);   close(); }
void KFactoryOptions::ClickBtnFunTest()       { SetResult(RESULT_FUN_TEST);    close(); }
void KFactoryOptions::ClickBtnAlgParamAjust() { SetResult(RESULT_ALG_PARAM);   close(); }
void KFactoryOptions::ClickBtnErrorRateTest() { SetResult(RESULT_ERROR_RATE);  close(); }
void KFactoryOptions::ClickBtnTemperature()   { SetResult(RESULT_TEMPERATURE); close(); }

// Device-заглушки (реф. зовут GetEndoScope()/GetCamera()/GetKHalClass()/KManuPwdMng).
void KFactoryOptions::ClickBtnSaveOther() {}
void KFactoryOptions::ClickBtnAuthMachine() {}
void KFactoryOptions::ClickBtnClearEndoInfo() {}
void KFactoryOptions::ClickManuLock() {}
void KFactoryOptions::RecoverConfigure() {}
void KFactoryOptions::AndStyle() {}

// ──────────────────── Off-device ядро (без изменений) ────────────────────

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
