#include "db/KExamNoGenerate.h"
#include "kernel/KConfig.h"
#include "sys/KSystem.h"
#include "sys/KSystemStatus.h"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstdio>
#include <cstring>

namespace {
const char *SECTION   = "ExamId";
const char *KEY_INDEX = "ExamIdIndex";
const char *FILE_NAME = "ExamListId.ini";
const char *FMT_ENDO  = "%s%04d";    // ViewType == 0 (эндоскоп)
const char *FMT_CAM   = "%s%04dR";   // ViewType != 0 (камера)
const char *DATE_FMT  = "yyyyMMdd";
} // namespace

std::shared_ptr<KConfig> KExamNoGenerate::m_ptrConfig;
int KExamNoGenerate::m_iExamIdIndex = 0;

void KExamNoGenerate::InitConfigFile()
{
    const QString full = QDir(KSystem::ProtectedPath()).absoluteFilePath(FILE_NAME);

    QDir dir(QFileInfo(full).absolutePath());
    if (!dir.exists())
        QDir().mkpath(dir.absolutePath());   // каталог создаётся (реф.)

    if (!QFile::exists(full)) {
        // Реф. создаёт ПУСТОЙ файл сырым fopen(...,"w") — не через KConfig.
        QFile f(full);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
            qInfo() << "KExamNoGenerate..create file" << FILE_NAME << "success";
        } else {
            qInfo() << "KExamNoGenerate..create file" << FILE_NAME << "fail";
        }
    }
    // Реф. создаёт KConfig ВСЕГДА, даже если файл создать не удалось.
    m_ptrConfig = std::make_shared<KConfig>(full.toStdString());
}

int KExamNoGenerate::GetExamIdIndex()
{
    if (!m_ptrConfig)
        InitConfigFile();
    // Кэша нет — реф. читает KConfig каждый вызов; m_iExamIdIndex не трогает.
    return m_ptrConfig->ReadInt(SECTION, KEY_INDEX, 0);
}

void KExamNoGenerate::SetExamId()
{
    if (!m_ptrConfig)
        InitConfigFile();
    SetExamId(m_iExamIdIndex);
}

void KExamNoGenerate::SetExamId(int idx)
{
    if (idx < 0)
        idx = 0;
    // Реф. здесь null-check НЕ делает (прямой вызов до Init → SIGSEGV).
    m_ptrConfig->WriteData(SECTION, KEY_INDEX, idx);
    m_ptrConfig->Save();   // на диск сразу
}

std::string KExamNoGenerate::MakeExamId()
{
    int idx = GetExamIdIndex() + 1;
    // Реф.: остаток от 9999, а НЕ сброс в 1 (10000%9999==1, но 19998%9999==0).
    if (idx > 9999)
        idx = idx % 9999;
    m_iExamIdIndex = idx;   // на диск не пишем — коммитит SetExamId

    const QByteArray date = QDate::currentDate().toString(DATE_FMT).toUtf8();
    // Суффикс 'R' — при ЛЮБОМ ненулевом ViewType (реф. cbnz, не сравнение с 1).
    const char *fmt = (KSystemStatus::GetInstance().ViewType() == 0) ? FMT_ENDO : FMT_CAM;

    char buf[1024];
    std::memset(buf, 0, sizeof(buf));
    std::snprintf(buf, sizeof(buf), fmt, date.constData(), m_iExamIdIndex);
    return std::string(buf, std::strlen(buf));
}

bool KExamNoGenerate::IsValidExamId(const std::string &strExamId)
{
    (void)strExamId;
    return true;   // реф. — `mov w0,#1; ret` (аргумент не читается)
}
