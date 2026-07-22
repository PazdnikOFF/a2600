#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <QMutex>

// Одна сохранённая метка (реф. exam_detail::KCursorInfo, 20 байт, порядок сериализации).
struct KCursorInfo
{
    int a = 0;        // центр X в пикселях ОРИГИНАЛА
    int b = 0;        // центр Y в пикселях оригинала
    float x = 0.0f;   // сырые сцен-координаты (реф. пишет как double)
    float y = 0.0f;
    int type = 0;     // E_CURSOR_TYPE
};

// Правки одной картинки (реф. KImgEditInfo, size 0x48): 4 int (гео, деф. -1) + 2 вектора меток
// (imgMarks = стрелки на КАРТИНКЕ, bodyMarks = метки на СХЕМЕ тела) + имя (runtime, НЕ сериал.).
struct KImgEditInfo
{
    int f0 = -1, f1 = -1, f2 = -1, f3 = -1;
    QVector<KCursorInfo> imgMarks;
    QVector<KCursorInfo> bodyMarks;
    QString name;
};

// Сериализатор правок изображений (реф. KImageEditorSerialization, singleton, size 0x58).
// UI-порт. НЕЙТРАЛЕН к графической вью (мост вью↔данные — в хосте KImageEditor::SaveCursorInfo).
// Реф. формат — Boost text_oarchive карты filename→KImgEditInfo; в порте — QDataStream (тот же
// порядок полей: 4 int, вектор imgMarks, вектор bodyMarks; KCursorInfo = 2int+2double+1int),
// самодостаточно без Boost. Один файл на директорию осмотра. Mutex-защита (реф. g_ThumbConfigMutex).
class KImageEditorSerialization
{
public:
    using KThumbConfig = QHash<QString, KImgEditInfo>;

    static KImageEditorSerialization &GetInstance();

    // Реф. @GetEditInfoByFileName: lookup по basename; если нет — вставить дефолт; копия в out.
    int GetEditInfoByFileName(const QString &filename, KImgEditInfo &out);

    bool SaveThumbConfigFile(const QString &path);   // реф.: сериализация всей карты
    bool LoadThumbConfigFile(const QString &path);   // реф.: десериализация
    bool Save();                                     // по сохранённому пути
    bool Load();

    void SetEditInfo(const QString &filename, const KImgEditInfo &info) { m_editInfos[filename] = info; }
    const KThumbConfig &EditInfos() const { return m_editInfos; }

private:
    KImageEditorSerialization() = default;

    KThumbConfig m_editInfos;      // +0x00
    QString m_configFilePath;      // +0x38
    QMutex m_mutex;                // реф. g_ThumbConfigMutex
};
