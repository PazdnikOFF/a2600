#pragma once

#include <memory>
#include <string>

class KConfig;

// Генератор номеров осмотра (реф. KExamNoGenerate, X-2600).
// НЕ синглтон и не объект с состоянием: sizeof==1, ctor пустой, все методы static,
// а состояние — два static-поля в .bss.
//
// Файл счётчика: <data>/protected/ExamListId.ini, персист через KConfig:
//   [ExamId]
//   ExamIdIndex=<int>
//
// Схема номера: "<yyyyMMdd><NNNN>" (+ суффикс 'R', если ViewType != 0 — камерный
// режим; 0 = эндоскоп). Инкремент делает MakeExamId (только в памяти), а на диск
// коммитит SetExamId — отсюда пара «временный номер → вступивший в силу»
// (KExamBussinessHandler::CreateTemporaryExamId / TakeEffectExamId).
class KExamNoGenerate
{
public:
    KExamNoGenerate() = default;   // реф. — пустой

    static void InitConfigFile();     // каталог + пустой файл + KConfig
    static int  GetExamIdIndex();     // ReadInt("ExamId","ExamIdIndex", 0) — дефолт 0, НЕ 1
    static void SetExamId();          // коммит текущего m_iExamIdIndex
    static void SetExamId(int idx);   // idx<0 → 0; WriteData + Save (диск сразу)
    static std::string MakeExamId();  // инкремент + формат; на диск НЕ пишет
    // Реф. — заглушка `return true` (аргумент не читается; правил валидации нет).
    static bool IsValidExamId(const std::string &strExamId);

private:
    static std::shared_ptr<KConfig> m_ptrConfig;   // .bss
    static int m_iExamIdIndex;                     // .bss, zero-init
};
