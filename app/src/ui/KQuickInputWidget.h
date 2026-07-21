#pragma once

#include <QWidget>
#include <QStringList>

class QListView;
class QStringListModel;
class QModelIndex;

// Find-попап для KMemComboBox (реф. KQuickInputWidget, безрамочное Popup-окно). Живой
// инкрементальный поиск: QListView + QStringListModel под полем ввода. Владелец
// (KMemComboBox) ставит windowFlags(Popup|Frameless) и позиционирует его. Записи —
// строки «name - id». Наружу — сигнал itemSelect(int) (индекс выбранной строки).
//
// DEVICE-STUB вынесен из этого класса: сама выборка совпадений (реф. пять
// KQuickInput*DbTableHandler::GetMatchDate по зашифрованной SQLite, ключ
// SONOSCOPE_X2000_KEY, таблицы tb_QuickInput*/tb_Report) делается в KMemComboBox через
// инъектируемый провайдер; сюда приходит уже готовый QStringList.
class KQuickInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KQuickInputWidget(QWidget *parent = nullptr);

    void SetItems(const QStringList &items);   // наполнить модель + подогнать высоту
    int CurrentRow() const;
    void MoveCursor(int delta);                // клавиатурная навигация (реф. MovePrev/Next)
    int Count() const;

signals:
    void itemSelect(int index);   // реф. @0x82b668

protected:
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void onClicked(const QModelIndex &idx);

private:
    int GetListViewHeight() const;

    QListView *m_list = nullptr;
    QStringListModel *m_model = nullptr;
};
