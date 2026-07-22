#include "InputMethod.h"
#include "KPinyinWidget.h"

#include <QKeyEvent>
#include <QWidget>
#include <QApplication>
#include <QInputMethodEvent>
#include <QInputMethodQueryEvent>

InputMethod &InputMethod::GetInstance()
{
    static InputMethod inst;
    return inst;
}

InputMethod::InputMethod()
{
    SetFunctionMap();
}

void InputMethod::SetFunctionMap()
{
    // Реф. @0x761278: bind по языкам (Chinese=0 НЕ регистрируется — дефолт-ветка).
    using namespace std::placeholders;
    m_map[Lang_Latin2]  = std::bind(&InputMethod::InputLatin, this, _1, _2);
    m_map[Lang_Latin3]  = std::bind(&InputMethod::InputLatin, this, _1, _2);
    m_map[Lang_Latin6]  = std::bind(&InputMethod::InputLatin, this, _1, _2);
    m_map[Lang_French]  = std::bind(&InputMethod::InputFrench, this, _1, _2);
    m_map[Lang_Russian] = std::bind(&InputMethod::InputRussian, this, _1, _2);
    m_map[Lang_Polish]  = std::bind(&InputMethod::InputPolish, this, _1, _2);
}

bool InputMethod::InputMutiLang(QKeyEvent *e) const
{
    // Реф. @0x7615f8.
    if (!e)
        return false;
    if (m_activeLang == Lang_English)
        return false;   // native Latin — Qt сам
    QWidget *w = QApplication::focusWidget();
    if (!w)
        return false;
    // Гейт: виджет должен принимать IME и не быть паролем/no-predict.
    QInputMethodQueryEvent q(Qt::ImEnabled);
    QCoreApplication::sendEvent(w, &q);
    if (!q.value(Qt::ImEnabled).toBool())
        return false;
    QInputMethodQueryEvent qh(Qt::ImHints);
    QCoreApplication::sendEvent(w, &qh);
    if (uint(qh.value(Qt::ImHints).toInt()) & 0x30388u)
        return false;
    if (m_activeLang != Lang_Chinese) {
        auto it = m_map.find(m_activeLang);
        if (it == m_map.end())
            return false;
        return it->second(e, w);
    }
    return InputChinese(e, w);
}

bool InputMethod::InputChinese(QKeyEvent *e, QWidget *w) const
{
    // Реф. @0x7608e8: движок готов → keypress → без Ctrl → буква a-z → кормить KPinyinWidget.
    if (!e)
        return false;
    KPinyinWidget &pw = KPinyinWidget::GetInstance();
    if (!pw.CheckGoogle())
        return false;
    if (e->type() != QEvent::KeyPress)
        return false;
    if (e->modifiers() & Qt::ControlModifier)
        return false;
    const QString t = e->text();
    const bool isLetter = t.size() == 1 && t[0].isLetter() && t[0].toLatin1() != 0;
    if (!isLetter)
        return pw.QueryPunctuation(w, t);   // пунктуация
    pw.SetEditWidget(w);
    pw.AppendPinyin(t);
    pw.show();
    return true;
}

bool InputMethod::CommitText(QKeyEvent *e, QWidget *w) const
{
    // Упрощённый per-lang коммит (реф. → KSmallLangInputMethod::HandleKeyEvent(e, lang, w)):
    // управляющие клавиши пропускаем; печатный текст коммитим через QInputMethodEvent.
    const int k = e->key();
    if (k == Qt::Key_Space || k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_Backspace
        || k == Qt::Key_Tab || k == Qt::Key_Left || k == Qt::Key_Right || k == Qt::Key_Up
        || k == Qt::Key_Down)
        return false;
    const QString t = e->text();
    if (t.isEmpty() || !t[0].isPrint())
        return false;
    QInputMethodEvent ime;
    ime.setCommitString(t);
    QCoreApplication::sendEvent(w, &ime);
    return true;
}

bool InputMethod::InputLatin(QKeyEvent *e, QWidget *w) const   { return CommitText(e, w); }
bool InputMethod::InputRussian(QKeyEvent *e, QWidget *w) const { return CommitText(e, w); }
bool InputMethod::InputFrench(QKeyEvent *e, QWidget *w) const  { return CommitText(e, w); }
bool InputMethod::InputPolish(QKeyEvent *e, QWidget *w) const  { return CommitText(e, w); }
