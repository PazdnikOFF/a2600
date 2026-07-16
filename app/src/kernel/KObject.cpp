#include "kernel/KObject.h"
#include "kernel/KPublishManager.h"

#include <cassert>

std::map<int, KObject *> KObject::m_objects;

KObject::KObject()
{
    InitObject(-1);
}

KObject::KObject(int id, KObject *parent)
    : m_nObjectId(id), m_pParent(parent)
{
    InitObject(id);
}

KObject::KObject(KObject *parent, int id)
    : m_nObjectId(id), m_pParent(parent)
{
    InitObject(id);
}

KObject::~KObject()
{
    if (m_nObjectId != -1)
        m_objects.erase(m_nObjectId);
    UnSubscribeAllMsg();
}

void KObject::InitObject(int id)
{
    if (id == -1)
        return;
    assert(id <= 4999);
    assert(m_objects.find(id) == m_objects.end());   // id уникален
    m_objects[id] = this;
}

KObject *KObject::GetKObject(int id) const
{
    const auto it = m_objects.find(id);
    return it != m_objects.end() ? it->second : nullptr;
}

void KObject::SubscribeMsg(int msgId)
{
    assert(msgId > 9999 && msgId <= 13999);
    KPublishManager::GetInstance().AddSubscribe(msgId, this);
}

void KObject::UnSubscribeMsg(int msgId)
{
    assert(msgId > 9999 && msgId <= 13999);
    KPublishManager::GetInstance().RemoveSubscribe(msgId, this);
}

void KObject::UnSubscribeAllMsg()
{
    KPublishManager::GetInstance().RemoveAllSubscirbes(this);
}

void KObject::PublishMsg(KMessage &msg) const
{
    assert(msg.m_msgId > 9999 && msg.m_msgId <= 13999);
    KPublishManager::GetInstance().PublishMsg(msg);
}

void KObject::PublishMsg(int msgId, short sParam, long long ll, const std::string &data) const
{
    KMessage msg;
    msg.m_msgId = msgId;
    msg.m_pSender = const_cast<KObject *>(this);   // реф. — sender = this
    msg.m_targetId = 0;
    msg.m_sParam = sParam;
    msg.m_llParam1 = ll;
    msg.m_strData = data;
    PublishMsg(msg);
}

bool KObject::SendMsg(KMessage &msg) const
{
    assert(msg.m_targetId > 0 && msg.m_targetId <= 4999);
    assert(msg.m_msgId > 0 && msg.m_msgId <= 3999);
    KObject *tgt = GetKObject(msg.m_targetId);
    if (!tgt) {
        msg.m_bHandled = false;
        return false;
    }
    tgt->HandleMsg(msg);   // база пустая → вызов безвреден (реф. гардит по override)
    return msg.m_bHandled;
}

bool KObject::SendMsg(int target, int msgId, short sParam, long long ll1, long long ll2,
                      const std::string &data) const
{
    KMessage msg;
    msg.m_targetId = target;
    msg.m_msgId = msgId;
    msg.m_pSender = const_cast<KObject *>(this);
    msg.m_sParam = sParam;
    msg.m_llParam1 = ll1;
    msg.m_llParam2 = ll2;
    msg.m_strData = data;
    return SendMsg(msg);
}

void KObject::RequestToParent(KMessage &msg) const
{
    assert(msg.m_msgId > 19999 && msg.m_msgId <= 23999);
    if (m_pParent)
        m_pParent->HandleChildRequest(msg);   // база пустая → безвреден
}

void KObject::PostMsg(KMessage) const
{
    // реф. @0x3e1c10 — 4-байтный ret: async-путь скомпилирован в no-op, сообщение отбрасывается.
}

void KObject::PostMsg(int, short, long long, const std::string &) const
{
    // реф. — строит KMessage (sender=this) и вызывает no-op PostMsg → отбрасывается.
}
