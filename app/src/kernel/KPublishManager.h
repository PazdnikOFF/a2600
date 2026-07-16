#pragma once

#include <list>
#include <map>

class KObject;
class KMessage;

// Глобальный брокер подписок (реф. KPublishManager, singleton, X-2600). Реестр —
// map<int msgId, list<KObject*>>. PublishMsg — СИНХРОННАЯ рассылка с защитной копией
// реестра (позволяет подписку/отписку из обработчика без инвалидации итераторов).
class KPublishManager
{
public:
    static KPublishManager &GetInstance();

    void AddSubscribe(int msgId, KObject *obj);      // dedup + append
    void RemoveSubscribe(int msgId, KObject *obj);   // удалить из списка (пустой ключ остаётся)
    void RemoveAllSubscirbes(KObject *obj);          // sic (опечатка реф.) — из всех списков
    void PublishMsg(KMessage &msg);                  // копия реестра → HandleSubscribeMsg каждому

private:
    KPublishManager() = default;
    bool InTheList(const std::list<KObject *> &lst, KObject *obj) const;

    std::map<int, std::list<KObject *>> m_subscribers;
};
