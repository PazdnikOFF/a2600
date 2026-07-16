#include "sys/KSessionInfo.h"

#include <QNetworkAccessManager>

KSessionInfo &KSessionInfo::GetInstance()
{
    static KSessionInfo inst;
    return inst;
}

KSessionInfo::~KSessionInfo()
{
    delete m_pManager;
}

void KSessionInfo::SetManuUid(const QString &v)          { m_manuUid = v; }
QString KSessionInfo::GetManuUid() const                 { return m_manuUid; }
void KSessionInfo::SetManuUserName(const QString &v)     { m_manuUserName = v; }
QString KSessionInfo::GetManuUserName() const            { return m_manuUserName; }
void KSessionInfo::SetManuAccessToken(const QString &v)  { m_manuAccessToken = v; }
QString KSessionInfo::GetManuAccessToken() const         { return m_manuAccessToken; }
void KSessionInfo::SetManuLoginFlag(int v)               { m_manuLoginFlag = v; }
int KSessionInfo::GetManuLoginFlag() const               { return m_manuLoginFlag; }

void KSessionInfo::SetServiceUid(const QString &v)          { m_serviceUid = v; }
QString KSessionInfo::GetServiceUid() const                 { return m_serviceUid; }
void KSessionInfo::SetServiceUserName(const QString &v)     { m_serviceUserName = v; }
QString KSessionInfo::GetServiceUserName() const            { return m_serviceUserName; }
void KSessionInfo::SetServiceAccessToken(const QString &v)  { m_serviceAccessToken = v; }
QString KSessionInfo::GetServiceAccessToken() const         { return m_serviceAccessToken; }
void KSessionInfo::SetServiceLoginFlag(int v)               { m_serviceLoginFlag = v; }
int KSessionInfo::GetServiceLoginFlag() const               { return m_serviceLoginFlag; }

QNetworkAccessManager *KSessionInfo::getManager()
{
    if (!m_pManager)
        m_pManager = new QNetworkAccessManager(nullptr);   // реф. ctor(nullptr)
    return m_pManager;
}
