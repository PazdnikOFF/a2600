#pragma once

#include <QTextEdit>
#include <QPoint>

// QTextEdit с тач-перетаскиванием вертикального скроллбара (реф. KTextEdit @ctor 0x815c60,
// base QTextEdit). УСТАНОВЛЕНО реверсом: НЕ инпут-политика (в отличие от KCounterTextEdit/
// KPasswordLineEdit) — только кастом-скролл для тачскрина. ctor: WA_MouseTracking +
// ScrollBarAsNeeded. eventFilter @0x815d20: press на скроллбаре → page-step или начало drag
// (на ползунке), move → пересчёт value по дельте, release → конец drag. Члены: m_dragging
// (+0x30), m_lastPos (+0x34/0x38). Сигналов/инпут-политики НЕТ. 100% PORT (на десктопе
// нативный скроллбар уже это умеет — фильтр можно оставить как есть).
class KTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit KTextEdit(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    bool IsOnVerticalScrollBar(const QPoint &pt) const;   // реф. @0x815cf8

    bool m_dragging = false;   // +0x30
    QPoint m_lastPos;          // +0x34/0x38
};
