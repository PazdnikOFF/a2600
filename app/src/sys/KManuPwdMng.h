#pragma once

#include <QDate>
#include <QString>

// Управление доступом производителя/сервиса (реф. KManuPwdMng, X-2600).
// НЕ UI, НЕ синглтон-с-состоянием: sizeof==1, полей нет — всё состояние в KSystemSet
// (секция [Manu]). Ленивый глобальный экземпляр (реф. GetKManuPwdMng).
//
// Пароли производителя/админа/сервиса — ПРОИЗВОДНЫЕ ОТ ТЕКУЩЕЙ ДАТЫ (меняются
// помесячно), простая целочисленная арифметика, БЕЗ хеша/шифра. Manu-доступ
// активируется challenge-response лиценз-файлом с USB (по ProcessorSN), срок 59 дней.
class KManuPwdMng
{
public:
    static KManuPwdMng &GetInstance();   // реф. GetKManuPwdMng (ленивый глобал)

    // Пароли текущего месяца (реф.: префикс + getPwd(n) + суффикс).
    QString GetPassWord() const;         // "se" + getPwd(51647) + "mnf"  — производитель
    QString GetAdmPassWord() const;      // "hd" + getPwd(32711) + "adm"  — админ
    QString GetServicePassWord() const;  // "se" + getPwd(6911)  + "srv"  — сервис

    // Лиценз-строка по серийнику + коду (реф. таблица простых T[10]).
    QString GenerateLicense(const QString &sn, int code) const;

    // Countdown срока действия manu-доступа (реф.): отсчёт оставшихся дней от отметки.
    void CheckPermission();
    // Сброс отметки времени при переводе часов (антиобман), только если manu включён.
    void UpdateSystemTime(const QDate &d);

    // Ядро генерации 4 цифр (реф. свободная функция getPwd(n) от текущей даты).
    // Вынесен параметр даты для детерминизма проверки; getPwd(n) = getPwd(n, today).
    static QString getPwd(int n, const QDate &date);
    static QString getPwd(int n);

private:
    KManuPwdMng() = default;
};
