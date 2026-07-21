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
class KPatientListSearch;

// Родительский вью списка пациентов (реф. KPatientListViewUi @ctor 0x7e5650, base QWidget,
// setupUi @0x7e7738). КУЛЬМИНАЦИЯ семьи patient: композит РЕАЛЬНЫХ портированных виджетов —
// встроенный KPatientListSearch + одна KTableView + пейджер-бар (4 KPagePushButton
// home/pre/next/tail + KPageLineEdit + метки count/maxPage). Фикс 1630×1024. Пейджер-бар —
// абсолютная геометрия (не layout). DB — за инъектируемым провайдером (реф. IPC/SQLite seam).
class KPatientListViewUi : public QWidget
{
    Q_OBJECT
public:
    explicit KPatientListViewUi(QWidget *parent = nullptr);

    // DEVICE-STUB инъекция: (page, condition) → строки для страницы. Реф. —
    // KPatientListDBTableHandler::GetPageRecordFromDb + KObject IPC.
    void SetPageProvider(std::function<QVector<QMap<QString, QString>>(int page)> fn, int totalPages, int totalCount);

private slots:
    void OnGetPatientListDataFromDB(int page);   // реф.: подгрузка страницы в модель
    void JumpToHeadPage();
    void ClickPatientListBtnPre();
    void ClickPatientListBtnNext();
    void JumpToTailPage();
    void JumpToCustomPage();                      // реф.: из edit_page по returnPressed
    void OnQuery(const QMap<QString, QString> &conditions);

private:
    void setupUi();
    void InitTable();
    void InitPageButton();
    void InitLineEditPage();
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
    KPatientListSearch *m_search = nullptr;

    int m_currentPage = 1;
    int m_maxPage = 1;
    int m_count = 0;
    std::function<QVector<QMap<QString, QString>>(int)> m_pageProvider;
};
