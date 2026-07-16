#include "sys/KManuPwdMng.h"
#include "sys/KSystemSet.h"

KManuPwdMng &KManuPwdMng::GetInstance()
{
    static KManuPwdMng inst;   // реф. ленивый глобал GetKManuPwdMng
    return inst;
}

QString KManuPwdMng::getPwd(int n, const QDate &date)
{
    // реф.: a = год%100 (или 55, если год кратен 100); S = a * месяц * n;
    // берутся 4 цифры S начиная со 2-й снизу, старшая первой (ведущие нули сохраняются).
    const int y = date.year();
    const int a = (y % 100 != 0) ? (y % 100) : 55;
    const int m = date.month();
    const int s = a * m * n;
    const int d2 = (s / 10) % 10;
    const int d3 = (s / 100) % 10;
    const int d4 = (s / 1000) % 10;
    const int d5 = (s / 10000) % 10;
    return QStringLiteral("%1%2%3%4").arg(d5).arg(d4).arg(d3).arg(d2);
}

QString KManuPwdMng::getPwd(int n)
{
    return getPwd(n, QDate::currentDate());
}

QString KManuPwdMng::GetPassWord() const
{
    return QStringLiteral("se") + getPwd(51647) + QStringLiteral("mnf");
}

QString KManuPwdMng::GetAdmPassWord() const
{
    return QStringLiteral("hd") + getPwd(32711) + QStringLiteral("adm");
}

QString KManuPwdMng::GetServicePassWord() const
{
    return QStringLiteral("se") + getPwd(6911) + QStringLiteral("srv");
}

QString KManuPwdMng::GenerateLicense(const QString &sn, int code) const
{
    // реф.: таблица простых T[10]; пустой sn → дефолт; '/' → '-'.
    static const int T[10] = {2111, 2503, 701, 491, 163, 3691, 4733, 6529, 7993, 5851};
    QString s = sn.isEmpty() ? QStringLiteral("201707182011") : sn;
    s.replace('/', '-');

    QString out = QString::number(code / 10);
    for (const QChar &ch : s) {
        const int c = ch.unicode();
        const int t = T[c % 10];
        out += QString::number(static_cast<long long>(t) * t * c);
    }
    out += QString::number(code % 10);
    return out;
}

void KManuPwdMng::CheckPermission()
{
    KSystemSet &ss = KSystemSet::GetInstance();
    if (!ss.GetManuEnable())
        return;

    const QDate today = QDate::currentDate();
    QDate mark = ss.GetManuMarkTime();
    if (!mark.isValid())
        mark = today;
    const int days = mark.daysTo(today);
    const int left = ss.GetManuLeftTime();

    if (left > days) {
        ss.SetManuLeftTime(left - days);
        ss.SetManuMarkTime(today);
    } else {
        // Истёк: гасим доступ.
        ss.SetManuEnable(false);
        ss.SetManuLeftTime(0);
        ss.SetManuMarkTime(today);
    }
}

void KManuPwdMng::UpdateSystemTime(const QDate &d)
{
    KSystemSet &ss = KSystemSet::GetInstance();
    if (ss.GetManuEnable())
        ss.SetManuMarkTime(d);   // антиобман по переводу часов
}
