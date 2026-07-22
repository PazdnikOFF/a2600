#include "KImageEditorSerialization.h"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QMutexLocker>

static const quint32 kMagic = 0x4B544843u;   // 'KTHC' — KThumbConfig
static const quint32 kVersion = 1;

static QDataStream &operator<<(QDataStream &s, const KCursorInfo &c)
{
    s << qint32(c.a) << qint32(c.b) << double(c.x) << double(c.y) << qint32(c.type);
    return s;
}
static QDataStream &operator>>(QDataStream &s, KCursorInfo &c)
{
    qint32 a, b, t; double x, y;
    s >> a >> b >> x >> y >> t;
    c.a = a; c.b = b; c.x = float(x); c.y = float(y); c.type = t;
    return s;
}
static QDataStream &operator<<(QDataStream &s, const KImgEditInfo &e)
{
    s << qint32(e.f0) << qint32(e.f1) << qint32(e.f2) << qint32(e.f3);
    s << quint32(e.imgMarks.size());
    for (const KCursorInfo &c : e.imgMarks) s << c;
    s << quint32(e.bodyMarks.size());
    for (const KCursorInfo &c : e.bodyMarks) s << c;
    return s;
}
static QDataStream &operator>>(QDataStream &s, KImgEditInfo &e)
{
    qint32 f0, f1, f2, f3; quint32 n;
    s >> f0 >> f1 >> f2 >> f3;
    e.f0 = f0; e.f1 = f1; e.f2 = f2; e.f3 = f3;
    s >> n; e.imgMarks.resize(int(n));
    for (int i = 0; i < int(n); ++i) s >> e.imgMarks[i];
    s >> n; e.bodyMarks.resize(int(n));
    for (int i = 0; i < int(n); ++i) s >> e.bodyMarks[i];
    return s;
}

KImageEditorSerialization &KImageEditorSerialization::GetInstance()
{
    static KImageEditorSerialization inst;
    return inst;
}

int KImageEditorSerialization::GetEditInfoByFileName(const QString &filename, KImgEditInfo &out)
{
    // Реф.: lookup по basename; если нет — вставить дефолт (name-заполнен).
    const QString key = QFileInfo(filename).fileName();
    if (!m_editInfos.contains(key)) {
        KImgEditInfo def;
        def.name = key;
        m_editInfos.insert(key, def);
    }
    out = m_editInfos.value(key);
    return 0;
}

bool KImageEditorSerialization::SaveThumbConfigFile(const QString &path)
{
    QMutexLocker lock(&m_mutex);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_5_12);
    s << kMagic << kVersion << quint32(m_editInfos.size());
    for (auto it = m_editInfos.constBegin(); it != m_editInfos.constEnd(); ++it)
        s << it.key() << it.value();
    m_configFilePath = path;
    return true;
}

bool KImageEditorSerialization::LoadThumbConfigFile(const QString &path)
{
    QMutexLocker lock(&m_mutex);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    if (f.atEnd())   // реф. peek()-гвард пустого файла
        return false;
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_5_12);
    quint32 magic, ver, n;
    s >> magic >> ver >> n;
    if (magic != kMagic)
        return false;
    m_editInfos.clear();
    for (quint32 i = 0; i < n; ++i) {
        QString key; KImgEditInfo e;
        s >> key >> e;
        m_editInfos.insert(key, e);
    }
    m_configFilePath = path;
    return true;
}

bool KImageEditorSerialization::Save()
{
    return m_configFilePath.isEmpty() ? false : SaveThumbConfigFile(m_configFilePath);
}

bool KImageEditorSerialization::Load()
{
    return m_configFilePath.isEmpty() ? false : LoadThumbConfigFile(m_configFilePath);
}
