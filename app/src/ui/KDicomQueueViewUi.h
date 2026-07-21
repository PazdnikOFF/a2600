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
class KDicomQueueSearch;

// Вид очереди отправки DICOM (реф. KDicomQueueViewUi : QWidget + KObject-mixin, ctor
// @0x8102f8, setupUi @0x812728). UI-порт НА ПОЛНОЙ ТОЧНОСТИ — собран из РЕАЛЬНЫХ виджетов
// сессии (embedded KDicomQueueSearch + KTableView + 4 KPagePushButton + KPageLineEdit),
// апгрейд прежней подстановочной версии. Структурный близнец KExamListViewUi/
// KPatientListViewUi. Таблица: 11 колонок (col0 чекбокс, «id» скрыт). DB — за инъектируемым
// провайдером (реф. OnGetDicomQueueDataFromDB + KObject IPC + OnResendSelectedData).
class KDicomQueueViewUi : public QWidget
{
    Q_OBJECT
public:
    explicit KDicomQueueViewUi(QWidget *parent = nullptr);

    void SetPageProvider(std::function<QVector<QMap<QString, QString>>(int page)> fn,
                         int totalPages, int totalCount);

private slots:
    void OnGetDicomQueueDataFromDB(int page);
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
    KDicomQueueSearch *m_search = nullptr;

    int m_currentPage = 1;
    int m_maxPage = 1;
    int m_count = 0;
    std::function<QVector<QMap<QString, QString>>(int)> m_pageProvider;
};
