#pragma once

#include <QWidget>
#include <QString>
#include <QDate>

class QVBoxLayout;
class QLabel;
class QLineEdit;
class QDateEdit;

// Редактор «зарезервированного поля» отчёта (реф. KReservedWidget @ctor 0x4df450, base
// QWidget). UI-порт. QVBoxLayout(margins 9): QLabel сверху + вход снизу, тип входа выбирает
// E_EDITOR_TYPE. Реализованы ДВА типа (реф.): E_LINEEDIT(0)→QLineEdit, E_DATEEDIT(1)→QDateEdit
// (прочие → только метка). Метка задаётся позже SetLabel. Сигналов НЕТ (потребитель берёт
// внутренний виджет через GetLineEdit/GetDateEdit). m_isUsed — внешний флаг «поле активно».
// installEventFilter переопределён — форвардит фильтр на активный вход. 100% PORT.
class KReservedWidget : public QWidget
{
    Q_OBJECT
public:
    enum E_EDITOR_TYPE { E_LINEEDIT = 0, E_DATEEDIT = 1 };

    explicit KReservedWidget(QWidget *parent = nullptr, E_EDITOR_TYPE type = E_LINEEDIT);

    void SetLabel(const QString &text);
    void SetLineEditText(const QString &text);
    QString GetLineEditText() const;
    void SetMaxInputLen(int n);
    void SetDate(const QDate &date);           // sentinel/min → specialValueText(" ")
    QDate GetDate() const;
    QLineEdit *GetLineEdit() const { return m_lineEdit; }
    QDateEdit *GetDateEdit() const { return m_dateEdit; }
    void SetIsUsed(bool v) { m_isUsed = v; }
    bool GetIsUsed() const { return m_isUsed; }

    void installEventFilter(QObject *filter);   // реф.: форвард на активный вход

private:
    QVBoxLayout *m_layout = nullptr;   // +0x30
    QLabel *m_label = nullptr;         // +0x38
    QLineEdit *m_lineEdit = nullptr;   // +0x40
    QDateEdit *m_dateEdit = nullptr;   // +0x48
    bool m_isUsed = false;             // +0x50
    int m_editorType = E_LINEEDIT;     // +0x54
};
