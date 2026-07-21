#pragma once

#include <QWidget>
#include <QMap>
#include <QString>
#include <functional>

class QGroupBox;
class QLabel;
class KTableView;
class KPagePushButton;
class KPageLineEdit;
class KExamListSearch;

// Вид списка обследований (реф. KExamListViewUi : QWidget + KObject-mixin — НЕ диалог!,
// ctor @0x801468, Ui_KExamListViewUi::setupUi @0x805258). UI-порт НА ПОЛНОЙ ТОЧНОСТИ —
// собран из РЕАЛЬНЫХ виджетов сессии (встроенный KExamListSearch + KTableView + 4
// KPagePushButton + KPageLineEdit), апгрейд прежней подстановочной версии. Структурный
// близнец KPatientListViewUi/KDicomQueueViewUi. grp_view → vbox → [widget_search (embedded
// KExamListSearch), tableView (KTableView, 20 колонок, col0 чекбокс), пейджер-бар (абс.
// геометрия: label записей/hint + home/pre/edit_page/label_page/next/tail)]. DB — за
// инъектируемым провайдером (реф. OnGetExamListDataFromDB + KObject IPC).
class KExamListViewUi : public QWidget
{
    Q_OBJECT
public:
    explicit KExamListViewUi(QWidget *parent = nullptr);

    void SetPageProvider(std::function<QVector<QMap<QString, QString>>(int page)> fn,
                         int totalPages, int totalCount);

private slots:
    void OnGetExamListDataFromDB(int page);
    void JumpToHeadPage();
    void ClickBtnPre();
    void ClickBtnNext();
    void JumpToTailPage();
    void JumpToCustomPage();

private:
    void setupUi();
    void InitTable();
    void InitPageButton();
    void RefreshPageInfo();
    void gotoPage(int page);

    QGroupBox *m_grpView = nullptr;
    QWidget *m_widgetSearch = nullptr;
    KTableView *m_tableView = nullptr;
    QWidget *m_pagerBar = nullptr;
    QLabel *m_labelDbRecord = nullptr;
    QLabel *m_labelHint = nullptr;
    QLabel *m_labelPage = nullptr;
    KPagePushButton *m_btnHome = nullptr;
    KPagePushButton *m_btnPre = nullptr;
    KPagePushButton *m_btnNext = nullptr;
    KPagePushButton *m_btnTail = nullptr;
    KPageLineEdit *m_editPage = nullptr;
    KExamListSearch *m_search = nullptr;

    int m_currentPage = 1;
    int m_maxPage = 1;
    int m_count = 0;
    std::function<QVector<QMap<QString, QString>>(int)> m_pageProvider;
};
