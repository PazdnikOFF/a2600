#pragma once

#include <QFrame>

class QPushButton;
class QLabel;

// Переиспользуемая панель кнопок настроек (реф. KParamSetBtn : QFrame, ctor @0x685030,
// Ui_KParamSetBtn::setupUi @0x685c28). UI-порт РЕАЛЬНОГО кастом-виджета (ранее подставлялся
// обычной панелью в KSystemSetDlg/KCustomEdit). 420×114, 3 ряда:
//   • frame_custom: btn_custom1/2/3 (текст задаётся SetBtnCustomFuncName, пусты по умолч.);
//   • frame_page (скрыт, показывается при SetTotalPageNum>1): «<» / label «n/m» / «>»;
//   • frame_btn: Default(TR_Dflt)/Save(TR_Sve)/Exit(TR_Ext).
// Сигналы наружу: ClickBtnCustom(int)/PageChanged(int)/ClickBtnDeafault()(sic)/ClickBtnSave()/
// ClickBtnExit(). API: SetTotalPageNum/SetBtnCustomFuncName(elide+tooltip)/HideCustom3Btn/
// SetBtnCustomVisible. Всё stock Qt, вложенных кастомов нет; скины — внешний QSS по objectName.
//
// DEVICE в порт не тянется: GetSystemStatus (гейтинг enable btn_custom1 по frozen-статусу) —
// заглушка (всегда enabled).
class KParamSetBtn : public QFrame
{
    Q_OBJECT
public:
    explicit KParamSetBtn(QWidget *parent = nullptr);

    void SetTotalPageNum(int n);                                       // >1 → показать page-nav
    void SetBtnCustomFuncName(const QString &a, const QString &b, const QString &c);
    void HideCustom3Btn();
    void SetBtnCustomVisible(bool v);

signals:
    void ClickBtnCustom(int idx);
    void PageChanged(int page);
    void ClickBtnDeafault();   // орфография как в бинарнике (внешний код коннектится точно так)
    void ClickBtnSave();
    void ClickBtnExit();

private:
    void setupUi();
    void updateLabelPageShow();

    QFrame *m_frameCustom = nullptr;
    QFrame *m_framePage = nullptr;
    QPushButton *m_custom1 = nullptr;
    QPushButton *m_custom2 = nullptr;
    QPushButton *m_custom3 = nullptr;
    QPushButton *m_prePage = nullptr;
    QPushButton *m_nextPage = nullptr;
    QLabel *m_labelPage = nullptr;
    int m_currentPage = 1;
    int m_totalPageNum = 1;
};
