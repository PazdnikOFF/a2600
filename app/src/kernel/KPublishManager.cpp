#include "kernel/KPublishManager.h"
#include "kernel/KObject.h"
#include "kernel/KMessage.h"

KPublishManager &KPublishManager::GetInstance()
{
    // реф. — файловый статический глобал; Meyers-синглтон функционально эквивалентен.
    static KPublishManager s_instance;
    return s_instance;
}

bool KPublishManager::InTheList(const std::list<KObject *> &lst, KObject *obj) const
{
    for (KObject *o : lst)
        if (o == obj)
            return true;
    return false;
}

void KPublishManager::AddSubscribe(int msgId, KObject *obj)
{
    std::list<KObject *> &lst = m_subscribers[msgId];   // авто-создание пустого списка
    if (!InTheList(lst, obj))                            // dedup
        lst.push_back(obj);
}

void KPublishManager::RemoveSubscribe(int msgId, KObject *obj)
{
    const auto it = m_subscribers.find(msgId);
    if (it == m_subscribers.end())
        return;
    it->second.remove(obj);   // ключ остаётся даже опустев (реф.)
}

void KPublishManager::RemoveAllSubscirbes(KObject *obj)
{
    for (auto &kv : m_subscribers)
        kv.second.remove(obj);
}

void KPublishManager::PublishMsg(KMessage &msg)
{
    if (m_subscribers.empty())
        return;
    // Защитная копия всего реестра — реентерабельность (реф. копирует _Rb_tree, не лочит).
    const std::map<int, std::list<KObject *>> copy = m_subscribers;
    const auto it = copy.find(msg.m_msgId);
    if (it == copy.end())
        return;
    for (KObject *sub : it->second)
        if (sub)
            sub->HandleSubscribeMsg(msg);   // СИНХРОННО, in-line
}
