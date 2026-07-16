#pragma once

#include <memory>
#include <string>

class KObject;

// Сообщение in-process шины (реф. KMessage, X-2600, sizeof 0x58). Поля ПУБЛИЧНЫЕ — реф. без
// геттеров, вызыватели читают/пишут напрямую. Значение-тип (копируемо/присваиваемо).
class KMessage
{
public:
    KMessage() = default;   // все сентинелы (дефолты полей ниже)
    // Реф. ctor(int,int,short,ll,ll,shared_ptr): target=a, msg=b, СЕНДЕР НЕ СТАВИТСЯ (0),
    // sParam=c, llParam1=d, llParam2=e, data=f.
    KMessage(int targetId, int msgId, short sParam, long long ll1, long long ll2,
             std::shared_ptr<void> data)
        : m_targetId(targetId), m_msgId(msgId), m_sParam(sParam),
          m_llParam1(ll1), m_llParam2(ll2), m_pData(std::move(data)) {}

    bool IsValid() const;             // true, если хоть одно поле != сентинел
    void Reset();                     // сентинелы, строка "", сброс shared_ptr; m_bHandled=TRUE (sic)
    bool operator==(const KMessage &o) const;

    int                   m_targetId = -1;      // +0x00 — целевой объект (SendMsg)
    int                   m_msgId = -1;         // +0x04 — id сообщения (Subscribe/Publish/Send)
    KObject              *m_pSender = nullptr;  // +0x08 — отправитель (builders ставят this)
    bool                  m_bHandled = false;   // +0x10 — обработано (SendMsg возвращает это)
    short                 m_sParam = -1;        // +0x12
    long long             m_llParam1 = -1;      // +0x18
    long long             m_llParam2 = -1;      // +0x20
    std::string           m_strData;            // +0x28
    std::shared_ptr<void> m_pData;              // +0x48
};
