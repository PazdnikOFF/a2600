#include "KPinyinWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QCoreApplication>
#include <QPainterPath>
#include <QRegion>
#include <QMutex>

KPinyinWidget &KPinyinWidget::GetInstance()
{
    static KPinyinWidget *inst = nullptr;
    static QMutex mtx;
    if (!inst) {
        QMutexLocker lock(&mtx);
        if (!inst)
            inst = new KPinyinWidget();
    }
    return *inst;
}

KPinyinWidget::KPinyinWidget(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor: InitStytle (frameless tool-popup translucent) + InitUI.
    setObjectName(QStringLiteral("KPinyinWidget"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(QStringLiteral(   // реф. KSystem::getIMEThemeQss — стаб
        "QFrame{background:rgb(30,32,38); border:1px solid rgb(83,83,83); border-radius:5px;}"
        "QLineEdit{background:transparent; color:rgb(0,205,209); border:none;}"
        "QListWidget{background:transparent; color:white; border:none;}"
        "QPushButton{background:transparent; color:white; border:none;}"));
    InitUI();
    resize(480, 110);
    setMinimumWidth(200);
}

void KPinyinWidget::InitUI()
{
    // Реф. InitUI: VBox(margins 3) → pinyinEdit + HBox[candList stretch | spacer | «<» | «>»].
    m_vbox = new QVBoxLayout(this);
    m_vbox->setContentsMargins(3, 3, 3, 3);

    m_pinyinEdit = new QLineEdit(this);
    m_pinyinEdit->setFocusPolicy(Qt::NoFocus);
    m_pinyinEdit->setReadOnly(true);
    m_vbox->addWidget(m_pinyinEdit);

    m_hbox = new QHBoxLayout();
    // Реф. QListWidget IconMode; в порте — ряд QLabel (надёжный горизонт. рендер).
    m_candRow = new QHBoxLayout();
    m_candRow->setSpacing(4);
    m_candRow->setContentsMargins(0, 0, 0, 0);
    m_hbox->addLayout(m_candRow);
    m_hbox->addStretch(1);
    m_prev = new QPushButton(QStringLiteral("<"), this);
    m_prev->setFlat(true); m_prev->setAutoDefault(false); m_prev->setFocusPolicy(Qt::NoFocus);
    m_hbox->addWidget(m_prev);
    m_next = new QPushButton(QStringLiteral(">"), this);
    m_next->setFlat(true); m_next->setAutoDefault(false); m_next->setFocusPolicy(Qt::NoFocus);
    m_hbox->addWidget(m_next);
    m_vbox->addLayout(m_hbox);

    connect(m_prev, &QPushButton::clicked, this, &KPinyinWidget::PagePrev);
    connect(m_next, &QPushButton::clicked, this, &KPinyinWidget::PageNext);
}

void KPinyinWidget::resizeEvent(QResizeEvent *e)
{
    // Реф.: скруглённая маска (radius 5).
    QPainterPath path;
    path.addRoundedRect(rect(), 5.0, 5.0);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
    QFrame::resizeEvent(e);
}

void KPinyinWidget::SetEditWidget(QWidget *w)
{
    // Реф.: сохранить + installEventFilter + позиция у поля + show.
    m_edit = w;
    if (w) {
        w->installEventFilter(this);
        move(w->mapToGlobal(w->rect().bottomLeft()));
        show();
    }
}

QStringList KPinyinWidget::StubSearch(const QString &pinyin) const
{
    // ЗАГЛУШКА KGooglePinyin: демо-словарь пиньинь→ханьцзы (реф. движок читает dict-файл).
    auto splitChars = [](const char *utf8) {
        QStringList r;
        for (QChar c : QString::fromUtf8(utf8)) r << QString(c);
        return r;
    };
    if (pinyin == QStringLiteral("ni"))  return splitChars("你尼泥呢拟逆腻");
    if (pinyin == QStringLiteral("hao")) return splitChars("好号豪毫浩耗嚎");
    if (pinyin == QStringLiteral("wo"))  return splitChars("我握卧沃涡");
    return splitChars("•••");   // плейсхолдер
}

bool KPinyinWidget::QueryPunctuation(QWidget *w, const QString &t)
{
    // Реф.: маппинг ASCII-пунктуации → fullwidth CJK + коммит в edit. В порте — стаб (не обрабатываем).
    Q_UNUSED(w); Q_UNUSED(t);
    return false;
}

void KPinyinWidget::AppendPinyin(const QString &s)
{
    // Реф.: буфер (лимит < 0xe) + спеллинг-строка + UpdateAllCand.
    if (s.isEmpty() || m_pinyin.length() >= 14)
        return;
    m_pinyin += s;
    m_pinyinEdit->setText(m_pinyin);
    UpdateAllCand();
}

void KPinyinWidget::UpdateAllCand()
{
    m_allCand = StubSearch(m_pinyin);
    SetCandPage(0);
}

void KPinyinWidget::SetCandPage(int page)
{
    m_page = page;
    m_index = 0;
    m_pageCand.clear();
    for (int i = page * 7; i < qMin(page * 7 + 7, m_allCand.size()); ++i)
        m_pageCand << m_allCand[i];
    UpdateCandUI();
}

void KPinyinWidget::UpdateCandUI()
{
    qDeleteAll(m_candLabels);
    m_candLabels.clear();
    for (int i = 0; i < m_pageCand.size(); ++i) {
        QLabel *lb = new QLabel(QStringLiteral("%1.%2").arg(i + 1).arg(m_pageCand[i]), this);
        lb->setAlignment(Qt::AlignCenter);
        m_candRow->addWidget(lb);
        m_candLabels << lb;
    }
    HighlightIndex(m_index);
}

void KPinyinWidget::HighlightIndex(int idx)
{
    m_index = idx;
    for (int i = 0; i < m_candLabels.size(); ++i)
        m_candLabels[i]->setStyleSheet(i == idx
            ? QStringLiteral("color:black; background:rgb(0,205,209); border-radius:3px; padding:2px 6px;")
            : QStringLiteral("color:white; padding:2px 6px;"));
}

void KPinyinWidget::PageNext()
{
    if ((m_page + 1) * 7 < m_allCand.size())
        SetCandPage(m_page + 1);
}

void KPinyinWidget::PagePrev()
{
    if (m_page > 0)
        SetCandPage(m_page - 1);
}

void KPinyinWidget::ReturnGoogleSelected(int pageIdx)
{
    // Реф.: выбор → в порте (стаб-движок) сразу коммитим выбранный ханьцзы.
    if (pageIdx < 0 || pageIdx >= m_pageCand.size())
        return;
    ReturnText(m_pageCand[pageIdx]);
}

void KPinyinWidget::ReturnText(const QString &s)
{
    // Реф.: снять eventFilter → QInputMethodEvent commit → teardown.
    if (m_edit) {
        m_edit->removeEventFilter(this);
        if (!s.isEmpty()) {
            QInputMethodEvent ime;
            ime.setCommitString(s);
            QCoreApplication::sendEvent(m_edit, &ime);
        }
    }
    m_edit = nullptr;
    m_pinyin.clear();
    m_selected.clear();
    m_allCand.clear();
    m_pageCand.clear();
    if (m_pinyinEdit) m_pinyinEdit->clear();
    qDeleteAll(m_candLabels); m_candLabels.clear();
    hide();
}

void KPinyinWidget::ReturnNULL()
{
    ReturnText(QString());   // отмена: teardown без коммита
}

bool KPinyinWidget::HandleKeyEvent(QKeyEvent *e)
{
    const int k = e->key();
    if (k >= Qt::Key_1 && k <= Qt::Key_7) {   // '1'..'7' → выбор кандидата 0..6
        ReturnGoogleSelected(k - Qt::Key_1);
        return true;
    }
    switch (k) {
    case Qt::Key_Space:
        if (e->modifiers() & Qt::ControlModifier) ReturnNULL();
        else ReturnGoogleSelected(m_index);
        return true;
    case Qt::Key_Minus:
    case Qt::Key_Up:
        PagePrev(); return true;
    case Qt::Key_Equal:
    case Qt::Key_Down:
        PageNext(); return true;
    case Qt::Key_Left:
        if (m_index > 0) { --m_index; HighlightIndex(m_index); } else PagePrev();
        return true;
    case Qt::Key_Right:
        if (m_index < m_pageCand.size() - 1) { ++m_index; HighlightIndex(m_index); } else PageNext();
        return true;
    case Qt::Key_Backspace:
        if (!m_pinyin.isEmpty()) { m_pinyin.chop(1); m_pinyinEdit->setText(m_pinyin);
            if (m_pinyin.isEmpty()) ReturnNULL(); else UpdateAllCand(); }
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        ReturnText(m_pinyinEdit->text());   // сырой спеллинг
        return true;
    default:
        break;
    }
    const QString t = e->text();
    if (!t.isEmpty() && t[0].unicode() >= 0x21 && t[0].unicode() <= 0xff) {
        AppendPinyin(t);
        return true;
    }
    return false;
}

bool KPinyinWidget::eventFilter(QObject *obj, QEvent *ev)
{
    const QEvent::Type t = ev->type();
    if (t == QEvent::MouseButtonPress || t == QEvent::MouseButtonRelease
        || t == QEvent::MouseButtonDblClick)
        return true;   // съесть мышь
    if (t == QEvent::FocusOut) {
        ReturnNULL();
        return false;
    }
    if (t == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        if (ke->matches(QKeySequence::SelectAll) || ke->matches(QKeySequence::Copy))
            return true;
        return HandleKeyEvent(ke);
    }
    return QFrame::eventFilter(obj, ev);
}
