#include "sys/KUserOsdSet.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KUserOsdSet &KUserOsdSet::GetInstance()
{
    static KUserOsdSet inst;
    return inst;
}

QString KUserOsdSet::cfgFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    // реф. GetUserOsdConfigFile: UserPresetPath()/osd.ini.
    return QDir(KSystem::UserPresetPath()).absoluteFilePath("osd.ini");
}

QString KUserOsdSet::GetUserOsdConfigFile() const { return cfgFile(); }

QStringList KUserOsdSet::GetFunctionNameList()
{
    // 1:1 с KUserOsdSet::GetFunctionNameList (порядок = ID функции).
    return {"TR_Frz", "TR_Zm1", "TR_IRIS1", "TR_AGC1", "TR_IEnh", "TR_Snp",
            "TR_Brtnss+", "TR_Brtnss-", "TR_Ctrst", "TR_WBalance", "TR_LMode", "TR_Rcd"};
}

int KUserOsdSet::FunctionIdToIndex(int funcId)
{
    if (funcId < 0)
        return 0;               // отступление: реф. читал бы за границей
    if (funcId > 11)
        return 0;               // реф.: > 11 -> 0
    const QStringList all = GetFunctionNameList();
    const QString name = GetFunctionName(funcId);
    const int idx = all.indexOf(name);
    return idx < 0 ? 0 : idx;   // промах -> 0
}

QString KUserOsdSet::GetFunctionName(int funcId)
{
    const QStringList l = GetFunctionNameList();
    return (funcId >= 0 && funcId < l.size()) ? l.at(funcId) : QString();
}

QString KUserOsdSet::buttonSection(Button btn)
{
    switch (btn) {
    case BTN_A: return "ButtomA";
    case BTN_B: return "ButtomB";
    case BTN_M:
    default:    return "ButtomM";
    }
}

QString KUserOsdSet::pressKey(Press press)
{
    return press == PRESS_LONG ? "LongPress" : "ShortPress";
}

int KUserOsdSet::GetButtonFunctionId(Button btn, Press press) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value(buttonSection(btn) + "/" + pressKey(press), -1).toInt();
}

int KUserOsdSet::GetButtonFunctionId(int packedIndex) const
{
    // index = button*2 + press (A=0,B=1,M=2 / Long=0,Short=1).
    if (packedIndex < 0)
        return -1;            // отступление: реф. читал бы за границей
    if (packedIndex >= 7)
        packedIndex = 0;      // реф.: зажим верхней границы в 0
    const Button btn = Button(packedIndex / 2);
    const Press press = Press(packedIndex % 2);
    return GetButtonFunctionId(btn, press);
}

void KUserOsdSet::SaveButtonId(Button btn, Press press, int funcId) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue(buttonSection(btn) + "/" + pressKey(press), funcId);
    ini.sync();
}

int KUserOsdSet::GetFootSwitchFunctionId(int idx) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value(QString("FootSwitch/Switch%1").arg(idx), -1).toInt();
}

void KUserOsdSet::SaveFootSwitch(int idx, int funcId) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue(QString("FootSwitch/Switch%1").arg(idx), funcId);
    ini.sync();
}

int KUserOsdSet::GetOperationMode() const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("Operation/Mode", 0).toInt();
}

void KUserOsdSet::SaveOperationMode(int mode) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue("Operation/Mode", mode);
    ini.sync();
}

int KUserOsdSet::GetIrisMode() const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("Iris/Mode", 0).toInt();
}

void KUserOsdSet::SaveIrisMode(int mode) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue("Iris/Mode", mode);
    ini.sync();
}
