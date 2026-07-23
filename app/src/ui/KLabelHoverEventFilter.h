#pragma once

#include <QObject>
#include <QPoint>
#include <QString>

class QWidget;

// Фильтр «подсказка по наведению на метку» (реф. KLabelHoverEventFilter : QObject,
// sizeof 0x28: +0x10 виджет, +0x18 текст, +0x20 int-смещение). UI-порт.
//
// ⚠️ В ЭТОЙ СБОРКЕ КЛАСС МЁРТВ: ссылок на его vtable нет нигде, кроме собственных
// деструкторов (остался от inline-определения в заголовке). Портирован ради полноты —
// точка установки НЕ ВОССТАНОВЛЕНА. Живой родственник с той же формулой точки —
// KHoverEventFilter (см. KPatientListWidgetItem.h).
//
// eventFilter @0x8381c8: на QEvent::ToolTip (0x6e) показывает подсказку и ПРОВАЛИВАЕТСЯ
// дальше (не возвращает); затем, ТОЛЬКО если смещение <= 39, обрабатывает Enter (0xa) —
// показать, и Leave (0xb) — погасить (showText с пустой строкой), оба с return true.
class KLabelHoverEventFilter : public QObject
{
    Q_OBJECT
public:
    KLabelHoverEventFilter(QWidget *w, const QString &text, int offset);

    // Реф. формула точки (обе ветки одинаковы): mapToGlobal(0,0) + (-10, -2*height + offset).
    QPoint TipPoint() const;

public:
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    QWidget *m_widget = nullptr;   // +0x10
    QString m_text;                // +0x18
    int m_offset = 0;              // +0x20
};
