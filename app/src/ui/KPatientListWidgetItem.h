#pragma once

#include <QFrame>
#include <QMap>
#include <QString>

class QLabel;

// Фильтр событий «показать полный текст в подсказке» (реф. KHoverEventFilter, 40 байт:
// +0x10 виджет, +0x18 полный текст, +0x20 смещение 150). Реф. eventFilter @0x833678:
// на QEvent::ToolTip (0x6e) зовёт QToolTip::showText(widget->mapToGlobal(0,0) +
// QPoint(-10, -2*h + m_offset), fullText). Ветки Enter(10)/Leave(11) активны только при
// m_offset <= 39 — при 150 (наш случай) мёртвые.
class KHoverEventFilter : public QObject
{
    Q_OBJECT
public:
    KHoverEventFilter(QWidget *w, const QString &fullText, int offset = 150);

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    QWidget *m_widget = nullptr;   // +0x10
    QString m_text;                // +0x18
    int m_offset = 150;            // +0x20
};

// Строка лево-навигационного меню управления пациентами (реф. KPatientListWidgetItem :
// QFrame, ctor @0x79e9c0 (QMap<QString,QString>, QString, QWidget*), sizeof 0x60). UI-порт.
//
// Раскладка (все константы из ctor): фрейм 211×80; label_img — ВЕСЬ фрейм, QRect(0,0,211,80),
// AlignCenter; label_text — QRect(23,50,165,28). Иконка НЕ рисуется поверх текста — текст
// лежит в нижней части. Размер фрейма после чтения иконок переопределяется на РАЗМЕР
// пиксмапа normalIcon (setFixedSize).
//
// Четыре состояния меняют ТОЛЬКО пиксмап label_img (никаких stylesheet/флагов):
// Select/UnSelect/Hover/Disable ← ключи карты selectIcon/normalIcon/hoverIcon/disableIcon.
// SetFontSize(px) — единственный stylesheet: «QLabel{font-size: %1px;}» на label_text.
class KPatientListWidgetItem : public QFrame
{
    Q_OBJECT
public:
    // Порядок чтения ключей карты в реф.: selectIcon, normalIcon, hoverIcon, disableIcon.
    KPatientListWidgetItem(const QMap<QString, QString> &icons, const QString &text,
                           QWidget *parent = nullptr);

    QLabel *getLabel() const { return m_labelText; }   // реф. @0x79e890 — именно label_text
    void SetFontSize(int px);                          // реф. @0x79e8a0
    void Select();      // реф. @0x79f850
    void UnSelect();    // реф. @0x79f8b8
    void Hover();       // реф. @0x79f920
    void Disable();     // реф. @0x79f988

private:
    QLabel *m_labelImg = nullptr;    // ui+0x00
    QLabel *m_labelText = nullptr;   // ui+0x08
    QString m_selectIcon;            // +0x38
    QString m_normalIcon;            // +0x40
    QString m_hoverIcon;             // +0x48
    QString m_disableIcon;           // +0x50
};

// Реф. свободная функция setElidedFrame @0x7a3468 (аргумент n НЕ используется — зовут с 6).
// Если текст помещается (width()-10 > ширина текста) — выход. Иначе ставится
// KHoverEventFilter и текст обрезается: Qt::ElideRight по ширине
// (ширина метки − ширина суффикса + 17), после чего суффикс приклеивается обратно.
void setElidedFrame(KPatientListWidgetItem *item, int n, const QString &text,
                    const QString &suffix);
