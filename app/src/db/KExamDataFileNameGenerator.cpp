#include "db/KExamDataFileNameGenerator.h"

#include "db/KExamData.h"

#include <cstdio>
#include <cstring>

std::atomic<int> KExamDataFileNameGenerator::m_atomFileNum{0};

KExamDataFileNameGenerator::KExamDataFileNameGenerator()
{
    ResetData();
}

void KExamDataFileNameGenerator::ResetData()
{
    m_atomFileNum.store(0, std::memory_order_seq_cst);
    m_strExamId.clear();
}

std::string KExamDataFileNameGenerator::GetFileSerialNum()
{
    const int n = m_atomFileNum.fetch_add(1, std::memory_order_seq_cst) + 1;  // ++
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    if (n <= 999) {
        snprintf(buf, sizeof(buf), "%03d", n);
    } else {
        int r = n % 999;
        if (r == 0)
            r = 999;
        // Первый аргумент — константа 999 (квирк реф., НЕ частное).
        snprintf(buf, sizeof(buf), "%d^%03d", 999, r);
    }
    return buf;
}

bool KExamDataFileNameGenerator::IsMaxNumFiles() const
{
    if (m_atomFileNum.load(std::memory_order_acquire) > 1997)
        return true;
    // Реф. @0x48dbec: tail-call KExamData::IsDoNotGenerateFileName(m_strExamId).
    // (в реф. `this` == &m_strExamId — строка лежит первым членом.)
    return KExamData::IsDoNotGenerateFileName(m_strExamId);
}

std::string KExamDataFileNameGenerator::GenerateFileName(const std::string &a,
                                                         std::string b,
                                                         std::string ext)
{
    m_strExamId = a;   // побочный эффект реф.
    return a + b + "_" + GetFileSerialNum() + "." + ext;
}

KExamDataFileNameGenerator &Generator()
{
    static KExamDataFileNameGenerator inst;
    return inst;
}
