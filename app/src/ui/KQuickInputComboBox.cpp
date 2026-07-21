#include "KQuickInputComboBox.h"

KQuickInputComboBox::KQuickInputComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // Реф. ctor @0x5a91e0: голый QComboBox, флаг +0x38 = 0.
    setEditable(true);   // быстрый ввод — редактируемое поле
}

void KQuickInputComboBox::SetLoadProvider(
    std::function<QStringList(const QString &, const QString &)> fn)
{
    m_loadProvider = std::move(fn);
}

void KQuickInputComboBox::Init(const QString &tableName, const QString &field, int flag)
{
    // Реф. @0x5a9278: new KQuickInputModel + LoadData + setModel + AllDataChanged.
    // Порт: DEVICE-STUB — загрузка через провайдер, заполнение нативной модели комбо.
    m_tableName = tableName;
    m_field = field;
    m_flag = (flag != 0);
    clear();
    if (m_loadProvider)
        addItems(m_loadProvider(tableName, field));
    setCurrentIndex(-1);
    setCurrentText(QString());
}

int KQuickInputComboBox::Save()
{
    // Реф. @0x5a9358: MRU-коммит currentText с дедупом (+ таймстамп в реф.-модели).
    const QString t = currentText().trimmed();
    if (t.isEmpty())
        return -1;
    const int existing = findText(t);
    if (existing >= 0)
        return -1;   // дубликат — реф. возвращает -1, ничего не вставляя
    insertItem(0, t);   // most-recent-first (таймстамп — device-часть, опущен)
    return 0;
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
