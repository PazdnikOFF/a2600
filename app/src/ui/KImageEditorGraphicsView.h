#pragma once

#include <QGraphicsView>
#include <QList>
#include <QRectF>
#include <QSize>
#include <QPixmap>

class QGraphicsScene;
class QGraphicsPixmapItem;

// Холст-аннотатор изображения (реф. KImageEditorGraphicsView : QGraphicsView, size ~0xe8).
// UI-порт. НЕ рисование пером, а РАССТАНОВКА фиксированных курсор-пиксмапов (стрелки/точка,
// exam_detail::E_CURSOR_TYPE) кликом по фону-картинке. Клик в m_drawableRect при type!=Lock →
// AddCursorPixmapItem + сигнал DrawEventOccurs. Undo/RemoveAll. GetDrawHistory отдаёт копию.
// Реф. курсор-иконки из GetReadOnlyBaseDir (device-путь) — в порте рисуем глиф-стрелки програмно.
// KScreenMng::GetRatioTo1K масштаб — в порте 1.0. 100% PORT (чистый Qt QGraphics).
class KImageEditorGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    // Реф. exam_detail::E_CURSOR_TYPE.
    enum CursorType {
        Lock = 0,           // навигация: клик НИЧЕГО не ставит
        ArrowRightDown = 1,
        ArrowRightUp = 2,
        ArrowLeftUp = 3,
        ArrowLeftDown = 4,
        Point = 5
    };

    // Одна расставленная метка (реф. image_editor::KCursorItem).
    struct CursorItem {
        QGraphicsPixmapItem *item = nullptr;
        int centerX = 0, centerY = 0;   // сцен-координаты клика
        int type = Lock;
    };

    explicit KImageEditorGraphicsView(QWidget *parent = nullptr);

    void InitSceneAndBackground();                              // реф.: сцена + фон-item z=-1
    void SetBackgroundPixmap(const QString &path, int w, int h); // реф.: фон + sceneRect + drawableRect
    void SetBackgroundPixmap(const QPixmap &pm, int w, int h);   // порт-удобство (без файла)
    void SetCursorType(int type);                              // реф. @SetCursorType: +0xa0 + QCursor
    int  GetCursorType() const { return m_cursorType; }
    void AddCursorPixmapItem(int x, int y, int type);          // реф.: make item + push
    void UndoLastItem();                                       // реф.: убрать последнюю
    void RemoveAllCursorPixmapItem();                          // реф.: очистить
    QList<CursorItem> GetDrawHistory() const { return m_history; }   // реф.: копия
    bool HasDrawn() const { return !m_history.isEmpty(); }
    int  GetBackgroundImgWidth() const { return m_bgSize.width(); }
    int  GetBackgroundImgHeight() const { return m_bgSize.height(); }

    static QPixmap MakeCursorPixmap(int type);   // глиф-стрелка/точка (замена device-иконок)

signals:
    void DrawEventOccurs();   // реф. сигнал (после каждой расстановки)

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    QGraphicsScene *m_scene = nullptr;        // +0xd0
    QGraphicsPixmapItem *m_bgItem = nullptr;  // +0xd8
    QList<CursorItem> m_history;              // +0x30 (реф. deque)
    int m_cursorType = Lock;                  // +0xa0
    QRectF m_drawableRect;                    // +0xa8
    QSize m_bgSize;                           // +0xc8/0xcc
};
