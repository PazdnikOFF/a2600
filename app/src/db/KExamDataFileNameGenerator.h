#pragma once

#include <atomic>
#include <string>

// Генератор имён файлов данных осмотра (реф. KExamDataFileNameGenerator, X-2600).
// sizeof == 0x20: единственное поле экземпляра — m_strExamId; счётчик файлов —
// СТАТИЧЕСКИЙ atomic<int> в .bss (общий на класс, не на экземпляр).
//
// Экземпляр — Мейерсов синглтон за СВОБОДНОЙ функцией `Generator()` (реф.
// _Z9Generatorv), а не за GetInstance().
class KExamDataFileNameGenerator
{
public:
    KExamDataFileNameGenerator();   // реф.: ctor зовёт ResetData()

    // Реф.: m_atomFileNum.store(0) И m_strExamId.clear() — всего 4 инструкции.
    // Зовётся из ctor и из KExamBussinessHandler::EndoPowerOffAction ⇒ нумерация
    // файлов начинается заново на каждом сеансе осмотра.
    void ResetData();

    // Реф.: n = ++m_atomFileNum;
    //   n <= 999 → snprintf("%03d", n)                → "001".."999"
    //   n >  999 → r = n % 999; if (!r) r = 999;
    //              snprintf("%d^%03d", 999, r)        → "999^001"
    // ВНИМАНИЕ: первый %d — ЛИТЕРАЛЬНАЯ КОНСТАНТА 999, а НЕ частное от деления.
    std::string GetFileSerialNum();

    // Реф.: > 1997 (0x7cd = 2*999-1) → true; иначе
    // KExamData::IsDoNotGenerateFileName(m_strExamId).
    bool IsMaxNumFiles() const;

    // Реф.: m_strExamId = a;  return a + b + "_" + GetFileSerialNum() + "." + ext;
    // Разделители "_" и "." зашиты. Побочный эффект — запоминание examId.
    std::string GenerateFileName(const std::string &a, std::string b, std::string ext);

private:
    std::string m_strExamId;                    // 0x00
    static std::atomic<int> m_atomFileNum;      // .bss, общий на класс
};

// Реф. свободная функция-синглтон (_Z9Generatorv).
KExamDataFileNameGenerator &Generator();
