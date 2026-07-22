#pragma once

#include <QComboBox>

class KQuickInputModel;

// Комбо быстрого ввода с MRU (реф. KQuickInputComboBox @ctor 0x5a91e0, base QComboBox).
// СИБЛИНГ KMemComboBox, но проще: нативный дропдаун поверх KQuickInputModel (не кастомный
// find-попап). Используется для повторного ввода частых значений (врач, направивший,
// имя/ID пациента).
//
// Реф. связка (сверена дизасмом):
//   Init(a1, a2, count) @0x5a9278 — new KQuickInputModel(this) → LoadData(a1, a2, count)
//                                    → setModel → AllDataChanged();
//   Save()              @0x5a9358 — строит KComboBoxItem из текущего текста и зовёт
//                                    KQuickInputModel::SaveData(item);
//   AllDataChanged(bool)@0x5a9310 — форвард в модель;
//   showPopup()         @0x5a9730 — эмит сигнала + ленивый connect currentIndexChanged.
// Третий аргумент Init — это ЛИМИТ строк (транзитом в LoadData), а не флаг: прежний порт
// трактовал его как флаг, исправлено.
//
// DEVICE-STUB не здесь: модель ходит в KQuickInput*DbTableHandler, а те — в инъектируемый
// KQuickInputStore (см. db/KQuickInputStore.h). Без установленного store список пуст.
class KQuickInputComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit KQuickInputComboBox(QWidget *parent = nullptr);

    // a1 — селектор словаря ("tb_QuickInputPatient"/"tb_QuickInputDoctor"/
    // "tb_QuickInputApplicant"), a2 — поле ("PatientName"/"PatientID"; для врача и
    // направившего игнорируется), count — лимит строк.
    void Init(const QString &table, const QString &field, int count = 10);
    void AllDataChanged();
    KQuickInputModel *Model() const { return m_model; }

    // Реф. @0x5a9358: «отметить использование» текущего текста. 0 — успех, -1 — нет модели,
    // пустой текст или записи с таким текстом в словаре нет (реф. НЕ добавляет новую).
    int Save();

signals:
    void SigIsShowPopup(bool shown);   // реф. showPopup emit

private slots:
    void OnCurrentIndexChanged(int index);   // лениво подключается в showPopup

public:
    void showPopup() override;   // реф. @0x5a9730
    void hidePopup() override;

private:
    KQuickInputModel *m_model = nullptr;
    bool m_idxConnected = false;
};
