#pragma once

#include <QFrame>
#include <QString>
#include <QStringList>
#include <QList>

class QVBoxLayout;
class QHBoxLayout;
class KPinyinLineEdit;
class QLabel;
class QPushButton;

// Плавающий попап кандидатов пиньинь (реф. KPinyinWidget : QFrame, ctor @0x764828, size 0xd8,
// singleton GetInstance). UI-порт. Безрамочный always-on-top translucent-попап со скруглением:
// строка-спеллинг (реальный KPinyinLineEdit) над рядом [список-кандидатов IconMode | spacer |
// «<» | «>»]. Клавиши цели перехватываются eventFilter: a-z → AppendPinyin, 1-7 → выбор
// кандидата, Space → выбор подсвеченного, -/=/стрелки → страницы/курсор, Enter → сырой спеллинг.
// Выбор коммитится в целевой edit через QInputMethodEvent::setCommitString (НЕ insert/setText).
// DEVICE/ENGINE seam: KGooglePinyin::search/get_candidate/choose — в порте ЗАГЛУШКА (демо-словарь
// пиньинь→ханьцзы). 7 кандидатов на страницу. 100% PORT UI (движок стаб).
class KPinyinWidget : public QFrame
{
    Q_OBJECT
public:
    static KPinyinWidget &GetInstance();

    void SetEditWidget(QWidget *w);       // реф.: installEventFilter + позиция + show
    void AppendPinyin(const QString &s);  // реф.: буфер + UpdateAllCand
    bool HandleKeyEvent(class QKeyEvent *e);   // реф.: маршрутизатор клавиш
    bool CheckGoogle() const { return true; }  // реф.: движок готов (стаб — всегда да)
    bool QueryPunctuation(QWidget *w, const QString &t);   // реф.: fullwidth-пунктуация (стаб)

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void PagePrev();
    void PageNext();

private:
    explicit KPinyinWidget(QWidget *parent = nullptr);
    void InitUI();
    void UpdateAllCand();               // реф.: search → get_candidate×n
    void SetCandPage(int page);         // реф.: страница = 7, курсор 0
    void UpdateCandUI();                // реф.: "1.你" в ряд-меток
    void HighlightIndex(int idx);       // подсветка текущего кандидата
    void ReturnGoogleSelected(int pageIdx);   // реф.: выбор кандидата
    void ReturnText(const QString &s);        // реф.: QInputMethodEvent commit + teardown
    void ReturnNULL();                        // реф.: отмена + teardown
    QStringList StubSearch(const QString &pinyin) const;   // ЗАГЛУШКА движка

    QVBoxLayout *m_vbox = nullptr;   // +0x30
    QHBoxLayout *m_hbox = nullptr;   // +0x38
    KPinyinLineEdit *m_pinyinEdit = nullptr;  // +0x40 (реальный класс, не QLineEdit)
    QHBoxLayout *m_candRow = nullptr;   // +0x48 (реф. QListWidget; в порте ряд QLabel)
    QList<QLabel *> m_candLabels;
    QPushButton *m_prev = nullptr;   // +0x50
    QPushButton *m_next = nullptr;   // +0x58
    QWidget *m_edit = nullptr;       // +0x60 цель
    QString m_pinyin;                // +0x88
    QStringList m_selected;          // +0x90
    QStringList m_allCand;           // +0x98
    QStringList m_pageCand;          // +0xb0
    int m_page = 0;                  // +0xc8
    int m_index = 0;                 // +0xcc
};
