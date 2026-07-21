#include "KDicomQueueViewUi.h"
#include "KTableView.h"
#include "KPagePushButton.h"
#include "KPageLineEdit.h"
#include "KDicomQueueSearch.h"
#include "sys/KEnvConfig.h"

#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDir>

KDicomQueueViewUi::KDicomQueueViewUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x8102f8: setupUi → new KDicomQueueSearch → InitTable → InitPageButton →
    // InitLineEditPage → SubscribeMsg (device) → Initconnect.
    setupUi();

    m_search = new KDicomQueueSearch(m_widgetSearch);
    QVBoxLayout *sv = new QVBoxLayout(m_widgetSearch);
    sv->setContentsMargins(0, 0, 0, 0);
    sv->addWidget(m_search);

    InitTable();
    InitPageButton();
    RefreshPageInfo();

    connect(m_btnHome, &KPagePushButton::clicked, this, &KDicomQueueViewUi::JumpToHeadPage);
    connect(m_btnPre, &KPagePushButton::clicked, this, &KDicomQueueViewUi::ClickBtnPre);
    connect(m_btnNext, &KPagePushButton::clicked, this, &KDicomQueueViewUi::ClickBtnNext);
    connect(m_btnTail, &KPagePushButton::clicked, this, &KDicomQueueViewUi::JumpToTailPage);
    connect(m_editPage, &QLineEdit::returnPressed, this, &KDicomQueueViewUi::JumpToCustomPage);
}

void KDicomQueueViewUi::setupUi()
{
    setObjectName(QStringLiteral("KDicomQueueViewUi"));
    resize(1630, 1034);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setObjectName(QStringLiteral("verticalLayout_2"));
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_grpView = new QGroupBox(this);
    m_grpView->setObjectName(QStringLiteral("grp_view"));
    m_grpView->setTitle(QString());
    root->addWidget(m_grpView);

    QVBoxLayout *v = new QVBoxLayout(m_grpView);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setContentsMargins(18, 4, 18, 4);

    m_widgetSearch = new QWidget(m_grpView);
    m_widgetSearch->setObjectName(QStringLiteral("widget_search"));
    m_widgetSearch->setMinimumWidth(1591);
    v->addWidget(m_widgetSearch);

    m_tableView = new KTableView(m_grpView);
    m_tableView->setObjectName(QStringLiteral("tableView"));
    v->addWidget(m_tableView, 1);

    m_pagerBar = new QWidget(m_grpView);
    m_pagerBar->setObjectName(QStringLiteral("widget"));
    m_pagerBar->setMinimumHeight(70);
    v->addWidget(m_pagerBar);

    m_labelDbRecord = new QLabel(m_pagerBar);
    m_labelDbRecord->setObjectName(QStringLiteral("label_db_record"));
    m_labelDbRecord->setGeometry(20, 1, 177, 65);

    m_labelHint = new QLabel(m_pagerBar);
    m_labelHint->setObjectName(QStringLiteral("label_hint"));
    m_labelHint->setGeometry(182, 16, 585, 50);
    m_labelHint->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_labelHint->setText(tr("TR_DCOPETExam") + QStringLiteral("\r\n\r\n")
                         + QStringLiteral("F4/F5: ") + tr("TR_SCAPage"));

    m_btnHome = new KPagePushButton(m_pagerBar);
    m_btnHome->setObjectName(QStringLiteral("btn_home"));
    m_btnHome->setGeometry(767, 10, 52, 34);
    m_btnPre = new KPagePushButton(m_pagerBar);
    m_btnPre->setObjectName(QStringLiteral("btn_pre"));
    m_btnPre->setGeometry(835, 10, 52, 34);

    m_editPage = new KPageLineEdit(m_pagerBar);
    m_editPage->setObjectName(QStringLiteral("edit_page"));
    m_editPage->setGeometry(911, 10, 52, 32);
    m_editPage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_labelPage = new QLabel(m_pagerBar);
    m_labelPage->setObjectName(QStringLiteral("label_page"));
    m_labelPage->setGeometry(980, 14, 41, 26);

    m_btnNext = new KPagePushButton(m_pagerBar);
    m_btnNext->setObjectName(QStringLiteral("btn_next"));
    m_btnNext->setGeometry(1021, 10, 52, 34);
    m_btnTail = new KPagePushButton(m_pagerBar);
    m_btnTail->setObjectName(QStringLiteral("btn_tail"));
    m_btnTail->setGeometry(1090, 10, 52, 34);
}

void KDicomQueueViewUi::InitTable()
{
    // Реф. 11 колонок; col0 чекбокс, «id» (col10) скрыт.
    QVector<KHeaderProperty> h = {
        {QString(), QStringLiteral("sel"), 40, true},
        {tr("TR_Tpe2"), QStringLiteral("Type"), 120, true},
        {tr("TR_Svce"), QStringLiteral("Service"), 140, true},
        {tr("TR_PID"), QStringLiteral("PID"), 120, true},
        {tr("TR_Nme"), QStringLiteral("Name"), 160, true},
        {tr("TR_STime"), QStringLiteral("SendTime"), 160, true},
        {tr("TR_Fname"), QStringLiteral("Filename"), 180, true},
        {tr("TR_Sze"), QStringLiteral("Size"), 100, true},
        {tr("TR_Stts"), QStringLiteral("Status"), 120, true},
        {tr("TR_Dtls1"), QStringLiteral("Details"), 200, true},
        {QStringLiteral("id"), QStringLiteral("id"), 0, false}};   // DicomCommandUID скрыт
    m_tableView->InitTableView(h, QStringLiteral("id"), 15, 0);
    connect(m_tableView, &KTableView::SigGetDataFromDB, this, &KDicomQueueViewUi::OnGetDicomQueueDataFromDB);
}

void KDicomQueueViewUi::InitPageButton()
{
    const QString base = QDir(QString::fromStdString(KEnvConfig::GetInstance().GetReadOnlyBaseDir()))
                             .absoluteFilePath(QStringLiteral("patient/pageturning/"));
    auto icons = [&](const QString &type) {
        QMap<QString, QString> m;
        m.insert(QStringLiteral("normalIcon"), base + "page_" + type + "_normal.png");
        m.insert(QStringLiteral("hoverIcon"), base + "page_" + type + "_hover.png");
        m.insert(QStringLiteral("disableIcon"), base + "page_" + type + "_disable.png");
        return m;
    };
    m_btnHome->InitButton(icons(QStringLiteral("head")));
    m_btnPre->InitButton(icons(QStringLiteral("front")));
    m_btnNext->InitButton(icons(QStringLiteral("next")));
    m_btnTail->InitButton(icons(QStringLiteral("tail")));
    m_editPage->SetMaximumValue(m_maxPage);
    m_editPage->setText(QString::number(m_currentPage));
}

void KDicomQueueViewUi::RefreshPageInfo()
{
    m_labelDbRecord->setText(tr("TR_Rcds") + QStringLiteral(": ") + QString::number(m_count));
    m_labelPage->setText(QString::number(m_maxPage));
    m_editPage->SetMaximumValue(m_maxPage);
    m_editPage->setText(QString::number(m_currentPage));

    const bool single = (m_maxPage <= 1);
    m_btnHome->setEnabled(!single && m_currentPage > 1);
    m_btnPre->setEnabled(!single && m_currentPage > 1);
    m_btnNext->setEnabled(!single && m_currentPage < m_maxPage);
    m_btnTail->setEnabled(!single && m_currentPage < m_maxPage);
}

void KDicomQueueViewUi::gotoPage(int page)
{
    if (page < 1) page = 1;
    if (page > m_maxPage) page = m_maxPage;
    m_currentPage = page;
    OnGetDicomQueueDataFromDB(page - 1);
    RefreshPageInfo();
}

void KDicomQueueViewUi::OnGetDicomQueueDataFromDB(int page)
{
    if (m_pageProvider && m_tableView->GetModel())
        m_tableView->GetModel()->SetModelData(page, m_pageProvider(page));
}

void KDicomQueueViewUi::JumpToHeadPage() { gotoPage(1); }
void KDicomQueueViewUi::ClickBtnPre() { gotoPage(m_currentPage - 1); }
void KDicomQueueViewUi::ClickBtnNext() { gotoPage(m_currentPage + 1); }
void KDicomQueueViewUi::JumpToTailPage() { gotoPage(m_maxPage); }

void KDicomQueueViewUi::JumpToCustomPage()
{
    bool ok = false;
    const int n = m_editPage->text().trimmed().toInt(&ok);
    if (ok)
        gotoPage(n);
}

void KDicomQueueViewUi::SetPageProvider(
    std::function<QVector<QMap<QString, QString>>(int)> fn, int totalPages, int totalCount)
{
    m_pageProvider = std::move(fn);
    m_maxPage = totalPages > 0 ? totalPages : 1;
    m_count = totalCount;
    gotoPage(1);
}
