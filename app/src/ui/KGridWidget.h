#pragma once

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QString>

class QGridLayout;

// Контроллер переукладки сетки (реф. KGridWidget @ctor 0x5a8350, base QWidget). UI-порт
// РЕАЛЬНОГО кастом-виджета — ранее подставлялся голым QWidget+QGridLayout. КЛЮЧЕВОЕ: НЕ
// авто-флоу и НЕ владеет layout'ом — обёртка над ВНЕШНЕ заполненным QGridLayout: caller
// строит сетку виджетов, отдаёт layout, KGridWidget снимает снапшот и ПЛОТНО переукладывает
// видимые ячейки, убирая дырки от скрытых. Сигналов/выделения/paint НЕТ.
class KGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KGridWidget(QWidget *parent = nullptr);

    void InitWidget(QGridLayout *layout, int columns);   // реф. @0x5a8888
    void SetGridLayout(QGridLayout *layout);             // реф. @0x5a84a8
    void SetColumn(int columns);                         // реф. @0x5a8778
    void LoadAllWidgets();                               // реф. @0x5a8780: снапшот ячеек
    void MarkWidgetVisible(const QString &objectName, bool visible);  // реф. @0x5a83a0
    void Relayout();                                     // реф. @0x5a86b8: плотная переукладка
    void RemoveAllWidgets();                             // реф. @0x5a8610
    void ShowAllRows();                                  // реф. @0x5a8570
    void HideSpecificRows(int fromRow);                  // реф. @0x5a84b0

protected:
    void resizeEvent(QResizeEvent *) override;           // реф. @0x5a89b8: фон под размер

private:
    QGridLayout *m_layout = nullptr;                     // +0x30 (НЕ владеет)
    QWidget *m_background = nullptr;                      // +0x38 «KGridWidgetBackground»
    QVector<QPair<QWidget *, bool>> m_cells;             // +0x40 снапшот (widget, visible)
    int m_columns = 4;                                   // +0x58
};
