#include "KMemComboBox.h"
#include "KQuickInputWidget.h"

#include <QLineEdit>
#include <QRegExpValidator>

KMemComboBox::KMemComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // Реф. ctor @0x690a48.
    setEditable(true);

    // Кастомный find-попап вместо нативного дропдауна.
    m_popup = new KQuickInputWidget(this);
    m_popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_popup->setFocusPolicy(Qt::NoFocus);
    m_popup->setFocusProxy(this);
    m_popup->hide();

    // Валидатор-фильтр имени (реф. @0x883068: негативный класс, запрет пунктуации).
    // АППРОКСИМАЦИЯ: ASCII-набор из декода реверса; точный литерал с unicode-fullwidth
    // вариантами не транскрибирован дословно.
    QRegExp re(QStringLiteral("[^_!@#$%&*(){}\\[\\]:;'\\\\|?/><,.`~=+\\-]+"));
    if (lineEdit())
        lineEdit()->setValidator(new QRegExpValidator(re, this));

    connect(m_popup, &KQuickInputWidget::itemSelect, this, &KMemComboBox::ClickedSlot);
    connect(this, &QComboBox::editTextChanged, this, &KMemComboBox::OnRadarChange);
}

void KMemComboBox::SetTableName(const QString &table, bool displayId)
{
    // Реф. @0x691628: имя таблицы уходит в попап (+0x460) — по нему он роутит GetMatchDate.
    m_tableName = table;
    if (m_popup)
        m_popup->SetTableName(table);
    m_displayId = displayId;   // реф.: 2-й арг → +0x34
}

void KMemComboBox::SetDisplayId(bool displayId)
{
    m_displayId = displayId;   // реф. @0x691370 → +0x34
}

void KMemComboBox::SetMatchProvider(KQuickInputWidget::MatchProvider fn)
{
    // Реф.-эквивалент: провайдер живёт в попапе (там же, где SearchMatchItem и _ListBuff).
    if (m_popup)
        m_popup->SetMatchProvider(std::move(fn));
}

void KMemComboBox::setMaxLength(int len)
{
    // Реф. @0x690cd8: ограничение длины на внутреннем lineEdit.
    if (lineEdit())
        lineEdit()->setMaxLength(len);
}

QString KMemComboBox::currentText() const
{
    return QComboBox::currentText().trimmed();
}

QString KMemComboBox::text() const
{
    return currentText();
}

void KMemComboBox::setText(const QString &t)
{
    // Реф. @0x690ec0: программная установка НЕ триггерит find-попап.
    disconnect(this, &QComboBox::editTextChanged, this, &KMemComboBox::OnRadarChange);
    setEditText(t);
    connect(this, &QComboBox::editTextChanged, this, &KMemComboBox::OnRadarChange);
}

// Реф. @0x690d38/0x690d10/0x690d60/0x690d68/0x690d70 — `ldr x0,[x0,#56]` + хвостовой вызов
// одноимённого метода попапа. Никакого разбора строки: поля берутся из `_ListBuff`.
QString KMemComboBox::GetName(int idx) const
{
    return m_popup ? m_popup->GetName(idx) : QString();
}

QString KMemComboBox::GetId(int idx) const
{
    return m_popup ? m_popup->GetId(idx) : QString();
}

int KMemComboBox::GetGender(int idx) const
{
    return m_popup ? m_popup->GetGender(idx) : 2;
}

QDate KMemComboBox::GetDoB(int idx) const
{
    return m_popup ? m_popup->GetDoB(idx) : QDate();
}

int KMemComboBox::GetAge(int idx) const
{
    return m_popup ? m_popup->GetAge(idx) : 0;
}

void KMemComboBox::OnRadarChange(const QString &text)
{
    // Реф. @0x690fe0: временно отключить себя (антирекурсия) → показать окно поиска.
    disconnect(this, &QComboBox::editTextChanged, this, &KMemComboBox::OnRadarChange);
    ShowFindWnd(text);
    connect(this, &QComboBox::editTextChanged, this, &KMemComboBox::OnRadarChange);
}

void KMemComboBox::ShowFindWnd(const QString &prefix)
{
    // Реф. @0x690f68 → KQuickInputWidget::SearchMatchItem (там же _ListBuff и DB-роутинг).
    if (prefix.trimmed().isEmpty() || !m_popup) {
        HideFindWnd();
        return;
    }
    m_records = m_popup->SearchMatchItem(prefix);
    if (m_records.isEmpty()) {
        HideFindWnd();
        return;
    }
    m_popup->SetItems(m_records);
    AdjustFindWndPos();
    m_popup->show();
}

void KMemComboBox::HideFindWnd()
{
    if (m_popup)
        m_popup->hide();
}

void KMemComboBox::AdjustFindWndPos()
{
    // Позиция под полем, ширина = ширине комбо (реф. AdjustFindWndPos).
    const QPoint below = mapToGlobal(rect().bottomLeft());
    m_popup->resize(width(), m_popup->height());
    m_popup->move(below);
}

void KMemComboBox::ClickedSlot(int index)
{
    SelectFindWndItem(index);
    HideFindWnd();
}

void KMemComboBox::SelectFindWndItem(int index)
{
    // Реф. @0x691390: флаг +0x34==0 → в поле id, иначе name; затем emit itemSelect.
    if (index < 0 || index >= m_records.size())
        return;
    setText(m_displayId ? GetName(index) : GetId(index));
    emit itemSelect(index);
}

void KMemComboBox::focusOutEvent(QFocusEvent *e)
{
    QComboBox::focusOutEvent(e);
    emit FocusOut();
}
