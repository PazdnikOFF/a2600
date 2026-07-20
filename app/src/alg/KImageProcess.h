#pragma once

#include <QImage>
#include <QSize>
#include <QString>

// Обработка изображений отчёта/снимков (реф. KImageProcess, X-2600).
//
// РЕВЕРС: класс — пустой POD без vtable и без полей, ctor — голый `ret`,
// ВСЕ методы СТАТИЧЕСКИЕ (x0 — первый реальный параметр, `this` не передаётся).
// Реализовано так же: класс статических методов.

// Тип компоновки пары изображений (реф. _KGroupType).
enum _KGroupType {
    K_GROUP_B_LEFT_A_RIGHT = 0,  // B слева, A справа (A — миниатюра)
    K_GROUP_A_LEFT_B_RIGHT = 1,  // зеркально; поведение зависит от flag == 1
    K_GROUP_UNUSED_2       = 2,  // реф.: холст 0x0, ничего не рисуется
    K_GROUP_UNUSED_3       = 3,  // реф.: то же
    K_GROUP_OVERLAY        = 4,  // наложение B на A в точке _KPoint
};

// Тип загрузки/целевой размер (реф. _LoadImgType).
enum _LoadImgType {
    K_LOAD_IMG_150x30  = 0,
    K_LOAD_IMG_850x120 = 1,
    K_LOAD_IMG_160x120 = 2,   // и любое значение >= 2
};

// Реф. _KPoint — два int. Размер структуры сверх этих полей НЕ УСТАНОВЛЕН.
struct _KPoint {
    int x = 0;   // 0x00
    int y = 0;   // 0x04
};

// Реф. _KGroupImgSize — 40 байт, 10 × int32 (размер доказан обнулением 0x28
// в CreateGroupImage).
struct _KGroupImgSize {
    int totalW = 0;      // 0x00
    int totalH = 0;      // 0x04
    int imgA_drawW = 0;  // 0x08
    int imgA_drawH = 0;  // 0x0c
    int imgB_w = 0;      // 0x10
    int imgB_h = 0;      // 0x14
    int A_x = 0;         // 0x18
    int A_y = 0;         // 0x1c
    int B_x = 0;         // 0x20
    int B_y = 0;         // 0x24
};

class KImageProcess
{
public:
    // --- цветовое пространство ---------------------------------------------
    // Коэффициенты — РОВНО Rec.709/BT.709, считаны из fmov-иммедиатов бинарника
    // (не выведены из учебника). Порядок каналов — RGB (индекс 0 = R).
    // Шаг источника 3 байта, приёмника — 12 (3 float). ПОЛЯ ВЫРАВНИВАНИЯ СТРОК
    // (bytesPerLine) НЕ УЧИТЫВАЮТСЯ — буферы должны быть плотными.
    // Смещения +128 на Cb/Cr НЕТ, ограничения диапазона НЕТ.
    static void RGB2Ybcbr(const uchar *pRGB, float *pYCC, int width, int height);
    static void Ybcbr2RGB(const float *pYCC, float *pRGB, int width, int height);

    // Кусочная тональная кривая ТОЛЬКО по Y (Cb/Cr не трогаются).
    // КВИРК: константа «пи» в реф. — ЛИТЕРАЛЬНАЯ 3.1416, а не M_PI.
    // Линейная ветвь считается во float, квадратичная — В DOUBLE (важно для
    // побитового совпадения). Ограничения диапазона НЕТ.
    static void EnhanceBrightnessAndContrast(float *pYCC, int width, int height,
                                             int nBrightness, int nContrast,
                                             int nThreshold);

    // Насыщенность в стиле HSL: float RGB → uchar RGB с насыщающим клампом.
    // Работает и при отрицательном nSat (обесцвечивание) — ветки по знаку нет.
    static void EnhanceSaturability(const float *pRGB, uchar *pOut,
                                    int width, int height, int nSat);

    // Единственный потребитель четырёх методов выше. Реф. тюнинг:
    // яркость 40, контраст 20, порог 50, насыщенность 20.
    // Возврат: 5 — ошибка (пустой путь / не загрузилось), 1 — успех.
    // КВИРК: результат save() ОТБРАСЫВАЕТСЯ ⇒ 1 возвращается и при неудаче записи.
    static int OptimizeReportImage(const QString &srcPath, QString dstPath);

    // --- геометрия ----------------------------------------------------------
    // КВИРК: при a.height() <= 0 || a.width() <= 0 НЕ ПИШЕТ НИЧЕГО (даже imgB_*).
    // `flag` читается ТОЛЬКО в ветке type == 1. Зазоров/полей нет.
    static void GetGroupImageSize(const QImage &a, const QImage &b, _KGroupType type,
                                  _KGroupImgSize &out, int flag);
    static QImage CreateGroupImage(const QImage &a, const QImage &b, const _KPoint &pt,
                                   _KGroupType type, int flag);
    // Реф.: оба режима переданы ЯВНО (не полагается на умолчания Qt).
    static QImage CreateThumbnail(const QImage &img, int w, int h);
    // Реф.: результат упакован в x0 (low32 — ширина). Ничьи/NaN идут в else-ветку.
    static QSize ScaledWithAspectRatio(const QSize &a, const QSize &b);

    // Возврат: 5 — пустой путь, 1 — src==dst (no-op) ИЛИ сохранено,
    //          0 — не загрузилось, 2 — не сохранилось.
    // Вписывание считается ВРУЧНУЮ во float32, не средствами Qt;
    // масштабирование — Qt::FastTransformation (в отличие от CreateThumbnail).
    static int ResizeCopyImage(QString srcPath, QString dstPath, _LoadImgType type);

    // --- пути и наложение иконок видео --------------------------------------
    // Реф.: absolutePath() + "/" + baseName() + ".jpg"
    static QString GetVideoImg(const QString &path);

    // Наложение 190x70 (Format_RGB888) в точке из конфига рабочего стола;
    // СОХРАНЯЕТ ПОВЕРХ ТОГО ЖЕ ФАЙЛА.
    static void FormatScreenShotImg(const char *path);

    // Иконка center.png по центру → <dir>/<base>_s.jpg. Путь — только вход.
    static bool MergeVideoSmallImage(const QString &path);
    // Иконка play.png в (100, h - (ih + 100)) → <dir>/res/<base>_v.jpg.
    // КВИРК: ширина НЕ проверяется (в отличие от малой иконки).
    static bool MergeVideoBigImage(const QString &path);
};
