#pragma once

#include <QHeaderView>

// Заголовок таблицы с чекбоксом «выбрать всё» в одной секции (реф. KCheckBoxHeaderView
// @ctor 0x7b2d00, base QHeaderView). UI-порт РЕАЛЬНОГО кастом-виджета — пара к KTableView
// (чекбокс-колонка 0). Рисует пиксмапами (patient/checkbox/checkbox_{checked,unchecked}
// [_hover].png из ProjectPresetPath), НЕ QStyle. Три-состояние (Unchecked/PartiallyChecked/
// Checked). Клик по секции чекбокса → тоггл + сигнал SigCheckStausChange(int 0/1). Владелец
// (вью) связывает сигнал с выбором строк и вызывает SetCheckState для синхронизации. 100% PORT.
class KCheckBoxHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit KCheckBoxHeaderView(int checkColumn, Qt::Orientation orientation = Qt::Horizontal,
                                 QWidget *parent = nullptr);

    void SetCheckState(Qt::CheckState s);   // реф.: вью→заголовок (all/none/mixed)
    Qt::CheckState GetCheckState() const { return Qt::CheckState(m_checkState); }
    void UpdateCheckboxHeader();            // реф.: сброс hover + repaint

signals:
    void SigCheckStausChange(int checked);   // реф. (орфография как в бинарнике)

protected:
    void paintSection(QPainter *p, const QRect &rect, int logicalIndex) const override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    QString checkboxAsset(const QString &name) const;

    bool m_pressedOnCheckbox = false;   // +0x30
    bool m_isChecked = false;           // +0x31
    bool m_partial = false;             // +0x32
    int m_checkState = 0;               // +0x34 (Qt::CheckState)
    int m_checkColumn = 0;              // +0x38
    bool m_hover = false;               // +0x3c
    int m_hoverSection = -1;            // +0x40
};
