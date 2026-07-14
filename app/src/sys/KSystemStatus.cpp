#include "sys/KSystemStatus.h"

KSystemStatus &KSystemStatus::GetInstance()
{
    static KSystemStatus inst;   // реф. локальный статик в GetSystemStatus()
    return inst;
}
