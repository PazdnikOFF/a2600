#include "kernel/KMessage.h"

bool KMessage::IsValid() const
{
    // реф. — true, если хоть одно скалярное поле не сентинел / строка непуста; иначе payload.
    if (m_targetId != -1 || m_msgId != -1 || m_pSender != nullptr || m_sParam != -1
        || m_llParam1 != -1 || m_llParam2 != -1 || !m_strData.empty())
        return true;
    return m_pData != nullptr;
}

void KMessage::Reset()
{
    m_targetId = -1;
    m_msgId = -1;
    m_pSender = nullptr;
    m_sParam = -1;
    m_llParam1 = -1;
    m_llParam2 = -1;
    m_strData.clear();
    m_pData.reset();
    m_bHandled = true;   // sic — реф. Reset() ставит m_bHandled = TRUE
}

bool KMessage::operator==(const KMessage &o) const
{
    // реф. — участвуют все поля; payload сравнивается по идентичности указателя.
    return m_targetId == o.m_targetId && m_pSender == o.m_pSender
        && m_msgId == o.m_msgId && m_sParam == o.m_sParam
        && m_llParam1 == o.m_llParam1 && m_llParam2 == o.m_llParam2
        && m_strData == o.m_strData && m_bHandled == o.m_bHandled
        && m_pData.get() == o.m_pData.get();
}
