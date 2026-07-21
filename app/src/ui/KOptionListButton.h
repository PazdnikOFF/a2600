#pragma once

#include <QWidget>
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontMetrics>

// Сегментный переключатель (реф. KOptionListButton, ctor @0x3d9380, base QWidget).
// UI-порт РЕАЛЬНОГО кастом-виджета. ВОПРЕКИ имени/подстановке — это НЕ попап-селектор и
// НЕ QComboBox: все опции рисуются встык равными ячейками, клик по ячейке — выбор.
// Полностью кастомная отрисовка (без stylesheet/ассетов/попапа): DrawBackground (градиент,
// на выбранной ячейке highlight-кисть) → DrawText (эллипсис+центр, выбранная — highlight-
// цвет) → DrawGrid (скруглённая рамка radius + вертикальные разделители). Скруглённые углы
// через setMask. 100% PORT — чистый Qt, ноль device-зависимостей.
class KOptionListButton : public QWidget
{
    Q_OBJECT
public:
    explicit KOptionListButton(QWidget *parent = nullptr);
    explicit KOptionListButton(const QStringList &options, QWidget *parent = nullptr);

    void SetOptions(const QStringList &options);   // реф. @0x3d92c8: авто-выбор index 0
    QStringList Options() const { return m_options; }
    QString Option(int index) const;

    void SetIndex(int index);                       // реф. @0x3d8950: emit IndexChanged
    int Index() const { return m_index; }

    // Непрозрачные данные на опцию (реф. QVariant-список).
    void SetOptionData(int index, const QVariant &data);
    QVariant GetOptionData(int index) const;
    QVariant GetCurOptionData() const { return GetOptionData(m_index); }
    int GetTheFirstOptionIndexWithData(const QVariant &data) const;

    // Тривиальные сеттеры оформления (реф. — записи в члены).
    void SetNormalColor0(const QColor &c) { m_normalColor0 = c; }
    void SetNormalColor1(const QColor &c) { m_normalColor1 = c; }
    void SetHighlightColor0(const QColor &c) { m_highlightColor0 = c; }
    void SetHighlightColor1(const QColor &c) { m_highlightColor1 = c; }
    void SetTextColor(const QColor &c) { m_textColor = c; }
    void SetTextHighlightColor(const QColor &c) { m_textHighlightColor = c; }
    void SetBorderColor(const QColor &c) { m_borderColor = c; }
    void SetFont(const QFont &f) { m_font = f; m_fontMetrics = QFontMetrics(f); }
    void SetRadius(int r) { m_radius = r; }         // реф. @0x3d92b8 → +0x50
    void SetBorderWidth(int w) { m_borderWidth = w; } // реф. @0x3d92c0 → +0x4c

signals:
    void IndexChanged(int index);   // реф. @0x819ec0

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    int IndexAtPos(int x, int y) const;   // реф. @0x3d8a30
    void ResizeGradient();                // реф. @0x3d8b88
    void ResizeMask();                    // реф. @0x3d8d00
    void DrawBackground(QPainter &p);
    void DrawText(QPainter &p);
    void DrawGrid(QPainter &p);

    QStringList m_options;              // +0x30
    mutable QStringList m_displayTexts; // +0x38 (эллипс-кэш → тултип)
    QList<QVariant> m_optionData;       // +0x40
    int m_index = -1;                   // +0x48
    int m_borderWidth = 1;              // +0x4c
    int m_radius = 9;                   // +0x50
    QBrush m_normalBrush;               // +0x58
    QBrush m_highlightBrush;            // +0x60
    QColor m_normalColor0 = QColor(0x66, 0x66, 0x66);   // +0x68
    QColor m_normalColor1 = QColor(0x44, 0x44, 0x44);   // +0x78
    QColor m_borderColor = QColor(0x31, 0x31, 0x31);    // +0x88
    QColor m_highlightColor0 = QColor(0x14, 0x15, 0x19); // +0x98
    QColor m_highlightColor1 = QColor(0x14, 0x15, 0x19); // +0xa8
    QColor m_textColor = QColor(0xFF, 0xFF, 0xFF);       // +0xb8
    QColor m_textHighlightColor = QColor(0xFF, 0xFF, 0xFF); // +0xc8
    QFont m_font = QFont(QStringLiteral("Arial"), 16);  // +0xd8
    QFontMetrics m_fontMetrics = QFontMetrics(m_font);  // +0xe8
};
