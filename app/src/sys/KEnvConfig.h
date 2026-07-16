#pragma once

#include <string>

// Пути окружения для XML-подсистемы (реф. KEnvConfig, X-2600).
// Синглтон (реф. — function-local static). Единственное поле — m_strBaseDir,
// кэшируется в ctor; RO/user-корни берутся у KSystem при каждом вызове (реф.).
//
// Весь класс — ровно эти три геттера: Set*/Init/прочих Get*Dir в оригинале НЕТ.
class KEnvConfig
{
public:
    static KEnvConfig &GetInstance();

    std::string GetBaseDir() const;          // = KSystem::DataPath()            (кэш ctor)
    std::string GetReadOnlyBaseDir() const;  // = KSystem::ProjectPresetPath()     (syspreset)
    std::string GetUsrDir() const;           // = KSystem::ProjectUserPresetPath() (userpreset)

private:
    KEnvConfig();
    std::string m_strBaseDir;   // +0x00 (единственное поле, sizeof=0x20)
};
