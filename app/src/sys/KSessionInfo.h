#pragma once

#include <QString>

class QNetworkAccessManager;

// Состояние облачной login-сессии (реф. KSessionInfo, X-2600). Синглтон-держатель
// в ПАМЯТИ (персиста нет — обнуляется при рестарте). НЕ UI/НЕ device/НЕ БД.
//
// Два независимых канала удалённого логина к облачному API SonoScape + общий
// QNetworkAccessManager для HTTP:
//   Manu    — platform user/производитель (KPUserLoginDlg::Login);
//   Service — сервисный инженер (KScopeInfoEdit/KCameraInfoEdit::ServiceUserLogin).
// Токены — bearer от auth-сервера; используются для аплоада авторизации эндоскопа/
// камеры/аппарата. Сам класс — пассивный state-holder; логин/таймаут — в диалогах.
class KSessionInfo
{
public:
    static KSessionInfo &GetInstance();   // реф. Meyers-синглтон
    ~KSessionInfo();

    // Manu-канал.
    void SetManuUid(const QString &v);           QString GetManuUid() const;
    void SetManuUserName(const QString &v);       QString GetManuUserName() const;
    void SetManuAccessToken(const QString &v);    QString GetManuAccessToken() const;
    void SetManuLoginFlag(int v);                 int GetManuLoginFlag() const;

    // Service-канал.
    void SetServiceUid(const QString &v);         QString GetServiceUid() const;
    void SetServiceUserName(const QString &v);    QString GetServiceUserName() const;
    void SetServiceAccessToken(const QString &v); QString GetServiceAccessToken() const;
    void SetServiceLoginFlag(int v);              int GetServiceLoginFlag() const;

    // Единый NAM для всех запросов сессии (реф. встраивает по значению; у нас — лениво).
    QNetworkAccessManager *getManager();

private:
    KSessionInfo() = default;

    QString m_manuUid;              // +0x00
    QString m_manuUserName;         // +0x20
    QString m_manuAccessToken;      // +0x40
    int     m_manuLoginFlag = 0;    // +0x60
    QString m_serviceUid;          // +0x68
    QString m_serviceUserName;     // +0x88
    QString m_serviceAccessToken;  // +0xa8
    int     m_serviceLoginFlag = 0;// +0xc8
    QNetworkAccessManager *m_pManager = nullptr;   // +0xd0 (в реф. — по значению)
};
