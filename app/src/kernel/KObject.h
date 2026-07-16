#pragma once

#include "kernel/KMessage.h"

#include <map>
#include <string>

// Участник in-process шины сообщений (реф. KObject, X-2600 — ПОЛИМОРФНЫЙ, sizeof 0x38, без
// базы). Диспетчеризация СИНХРОННАЯ. Диапазоны id проверяются assert'ами (как в реф.):
//   объекты 0..4999; Send msgId (0,3999]; Publish msgId (9999,13999]; Request msgId (19999,23999].
// PostMsg в этой сборке реф. — ЗАГЛУШКА (сообщение отбрасывается; async-путь скомпилирован в no-op).
// Реестр объектов m_objects — СТАТИЧЕСКИЙ (id→объект), общий для всех экземпляров.
class KObject
{
public:
    KObject();
    KObject(int id, KObject *parent);
    KObject(KObject *parent, int id);
    virtual ~KObject();

    virtual std::string GetKObjectName() const { return m_strName; }   // vtable +0x10

    void SubscribeMsg(int msgId);        // регистрация у брокера (msgId∈(9999,13999])
    void UnSubscribeMsg(int msgId);
    void UnSubscribeAllMsg();

    void PublishMsg(KMessage &msg) const;   // broadcast подписчикам → HandleSubscribeMsg
    void PublishMsg(int msgId, short sParam, long long ll, const std::string &data) const;

    bool SendMsg(KMessage &msg) const;      // адресно m_objects[target] → HandleMsg; возврат handled
    bool SendMsg(int target, int msgId, short sParam, long long ll1, long long ll2,
                 const std::string &data) const;

    void RequestToParent(KMessage &msg) const;   // вверх родителю → HandleChildRequest

    void PostMsg(KMessage msg) const;       // реф. — no-op (сообщение отбрасывается)
    void PostMsg(int msgId, short sParam, long long ll, const std::string &data) const;

    KObject *GetKObject(int id) const;      // из статического реестра m_objects
    KObject *GetParentObject() const { return m_pParent; }
    int      ObjectId() const { return m_nObjectId; }

    // Обработчики — по умолчанию ПУСТЫЕ, переопределяются подклассами.
    virtual void HandleMsg(KMessage &msg) { (void)msg; }            // vtable +0x18
    virtual void HandleChildRequest(KMessage &msg) { (void)msg; }   // vtable +0x20
    virtual void HandleSubscribeMsg(KMessage &msg) { (void)msg; }   // vtable +0x28

protected:
    void InitObject(int id);   // регистрация в m_objects (id!=-1, <=4999, уникален)

    int          m_nObjectId = -1;      // +0x08
    std::string  m_strName;             // +0x10
    KObject     *m_pParent = nullptr;   // +0x30

    static std::map<int, KObject *> m_objects;   // реф. static — глобальный реестр id→объект
};
