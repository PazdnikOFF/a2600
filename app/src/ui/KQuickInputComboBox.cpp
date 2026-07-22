#include "KQuickInputComboBox.h"
#include "KQuickInputModel.h"

KQuickInputComboBox::KQuickInputComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // Реф. ctor @0x5a91e0: голый QComboBox.
    setEditable(true);   // быстрый ввод — редактируемое поле
}

void KQuickInputComboBox::Init(const QString &table, const QString &field, int count)
{
    // Реф. @0x5a9278: new KQuickInputModel(this) → LoadData(транзитом) → setModel →
    // AllDataChanged().
    if (!m_model)
        m_model = new KQuickInputModel(this);
    m_model->LoadData(table.toStdString(), field.toStdString(), count);
    setModel(m_model);
    setCurrentIndex(-1);
    setCurrentText(QString());
    AllDataChanged();
}

void KQuickInputComboBox::AllDataChanged()
{
    if (m_model)
        m_model->AllDataChanged();   // реф. @0x5a9310 — прямой форвард
}

int KQuickInputComboBox::Save()
{
    // Реф. @0x5a9358: KComboBoxItem из текущего текста → KQuickInputModel::SaveData.
    if (!m_model)
        return -1;
    const QString t = currentText().trimmed();
    if (t.isEmpty())
        return -1;
    KComboBoxItem item;
    item.text = t.toStdString();
    return m_model->SaveData(item);
}

void KQuickInputComboBox::showPopup()
{
    // Реф. @0x5a9730: эмит SigIsShowPopup + ленивое подключение currentIndexChanged.
    emit SigIsShowPopup(true);
    if (!m_idxConnected) {
        connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &KQuickInputComboBox::OnCurrentIndexChanged);
        m_idxConnected = true;
    }
    QComboBox::showPopup();
}

void KQuickInputComboBox::hidePopup()
{
    emit SigIsShowPopup(false);   // порт: парный сигнал скрытия (инференс)
    QComboBox::hidePopup();
}

void KQuickInputComboBox::OnCurrentIndexChanged(int)
{
    // Реф. слот — обработчик выбора (device-роутинг в реф.); в порте оставлен как хук.
}
