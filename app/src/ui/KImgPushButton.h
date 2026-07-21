#pragma once

#include <QPushButton>
#include <QPixmap>
#include <QSize>
#include <QString>

// Картиночная кнопка с 4 состояниями (реф. KImgPushButton @ctor 0x5a8ca8, base QPushButton).
// UI-порт РЕАЛЬНОГО кастом-виджета. Родня KPagePushButton, но ОТЛИЧИЯ важны:
//   • конфиг НЕ QMap, а InitButtons(normal,hover,checked,disable, QSize, bool) — явные пути;
//   • 4 состояния (normal/hover/CHECKED/disable) вместо 3; есть toggle-состояние, pressed НЕТ;
//   • состояние берётся из ШТАТНЫХ флагов Qt (WA_Disabled / isChecked / WA_UnderMouse) —
//     собственных pressed/hover-булей и mouse/enter/leave-оверрайдов НЕТ;
//   • ctor НЕ ставит WA_MouseTracking (hover едет на WA_UnderMouse);
//   • нет RefreshBtnOfLeave; текстовой метки нет (чистая картинка);
//   • пиксмап МАСШТАБИРУЕТСЯ в QRectF(0,0,drawSize) (не native size).
// Приоритет отрисовки: disabled > checked > hover > normal. checkable ctor НЕ включает —
// вызывающий сам setCheckable(true), чтобы дойти до checked-пиксмапа.
// 100% PORT: чистый Qt, только 4 пути картинок.
//
// ВЕНДОР-БАГ (реф.): на успешном пути InitButtons m_drawSize остаётся (-1,-1) из ctor →
// QRectF(0,0,-1,-1) → вырожденная отрисовка (в реф. рисует только fail-путь). В порте ставим
// m_drawSize = size и на успехе (faithful-but-working) — помечено в .cpp.
class KImgPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit KImgPushButton(QWidget *parent = nullptr);

    // Реф. @0x5a8b78: setFixedSize(sz) + load 4 пиксмапа (normal/hover/checked/disable).
    // Все загрузились → m_loaded=1, return 0. Иначе m_drawSize по bool (true→sz, false→
    // normal.size()), update, return -1.
    int InitButtons(const QString &normal, const QString &hover, const QString &checked,
                    const QString &disable, const QSize &size, bool useGivenSizeOnFail);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QPixmap m_normal;    // +0x30
    QPixmap m_hover;     // +0x50
    QPixmap m_checked;   // +0x70
    QPixmap m_disable;   // +0x90
    QSize m_drawSize = QSize(-1, -1);   // +0xb0
    bool m_loaded = false;              // +0xb8
};
