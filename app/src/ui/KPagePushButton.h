#pragma once

#include <QPushButton>
#include <QPixmap>
#include <QString>
#include <QMap>

// Иконочная кнопка пейджер-бара (реф. KPagePushButton @ctor 0x7b72e8, base QPushButton).
// UI-порт РЕАЛЬНОГО кастом-виджета — ранее подставлялся QPushButton с глифом в пейджер-барах
// (KDicomQueueViewUi/KExamListViewUi/…). Собственных сигналов/слотов НЕТ — наружу торчит
// штатный QPushButton::clicked(). Идентичность кнопки (head/front/next/tail) НЕ зашита в класс:
// три пути картинок передаются в InitButton через QMap; литералы путей живут в call-site'ах.
//
// Состояние рисуется вручную в paintEvent (без stylesheet/QIcon): один из 3 пиксмапов —
//   disabled → m_disable; иначе hover-флаг? m_hover : m_normal (отдельного pressed-пиксмапа нет).
// ctor: setMouseTracking(true) (реф. WA_MouseTracking) — чтобы hover ловился без зажатия.
// 100% PORT: чистый Qt, никаких device-зависимостей (нужны лишь сами файлы картинок).
class KPagePushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit KPagePushButton(QWidget *parent = nullptr);

    // Ключи карты: "normalIcon" / "hoverIcon" / "disableIcon". Грузит 3 пиксмапа и
    // фиксирует размер кнопки под normal-картинку (реф. setFixedSize(normal.size())).
    void InitButton(const QMap<QString, QString> &icons);

    // Сброс кнопки при уходе курсора с бара (реф. @0x7b72b0): снять pressed, снять hover
    // если нет фокуса, перерисовать.
    void RefreshBtnOfLeave();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;

private:
    QPixmap m_normal;    // +0x30
    QPixmap m_disable;   // +0x50
    QPixmap m_hover;     // +0x70
    QString m_normalPath;   // +0x90
    QString m_hoverPath;    // +0x98
    QString m_disablePath;  // +0xa0
    bool m_pressed = false; // +0xa8
    bool m_hoverState = false; // +0xa9
};
