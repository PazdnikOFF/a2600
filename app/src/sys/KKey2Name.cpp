#include "sys/KKey2Name.h"

#include "sys/KKeyNameTables.h"

std::string KKey2Name::GetNameOfKey(int eKey)
{
    return keyname::GetNameOfKey(eKey);
}

std::string KKey2Name::GetNameOfQtScancode(int nScancode)
{
    return keyname::GetNameOfQtScancode(nScancode);
}
