#pragma once

#include <QString>

// Конфиг сервера выгрузки информации об эндоскопе (реф. чтение endoinfoserver.ini,
// X-2600). Используется KCameraInfoEdit/KScopeInfoEdit для отправки логов
// использования эндоскопа в облако SonoScape (DetectEndoInfoUploadApi/UploadEndoInfoLog).
//
// Файл: system/presetdata/endoinfoserver/endoinfoserver.ini, секция [endoinfoserver]:
//   dns1/dns2 — DNS-серверы; proxy — прокси; loginurl — URL авторизации;
//   endoinfoposturl — URL загрузки лога. Рядом public_key.pem (подпись выгрузки).
class KEndoInfoServerConfig
{
public:
    static KEndoInfoServerConfig &GetInstance();

    void SetConfigFile(const QString &path) { cfgFile_ = path; }
    QString ConfigFile() const;                    // …/endoinfoserver/endoinfoserver.ini
    QString PublicKeyFile() const;                 // …/endoinfoserver/public_key.pem

    // Ключи [endoinfoserver] (реф.).
    QString Dns1() const;                          // dns1
    QString Dns2() const;                          // dns2
    QString Proxy() const;                         // proxy
    QString LoginUrl() const;                      // loginurl
    QString EndoInfoPostUrl() const;               // endoinfoposturl

    bool IsValid() const;                          // есть хотя бы post-url

private:
    KEndoInfoServerConfig() = default;
    QString cfgDir() const;
    QString value(const QString &key) const;
    QString cfgFile_;
};
