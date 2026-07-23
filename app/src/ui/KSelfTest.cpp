#include "ui/KSelfTest.h"

#include "endo/KEndoScope.h"
#include "sys/KSystem.h"
#include "sys/KSystemSet.h"

#include <QDir>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

namespace {
// Единственный device-шов (см. заголовок): на приборе это KMainCtrlThread.
std::function<QPair<int, int>()> g_lampTime = []{ return qMakePair(0, 0); };
}

void KSelfTest::SetLampTimeProvider(std::function<QPair<int, int>()> f)
{
    g_lampTime = f ? f : std::function<QPair<int, int>()>([]{ return qMakePair(0, 0); });
}

KSelfTest::KSelfTest(QWidget *parent)
    : KDialog(parent, false)
{
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("KSelfTest"));
    resize(640, 480);

    QWidget *host = ContentArea() ? ContentArea() : this;
    m_pGridLayout = new QGridLayout(host);
    m_pGridLayout->setObjectName(QStringLiteral("gridLayout"));
    m_pGridLayout->setVerticalSpacing(20);
    // Реф. setContentsMargins(-1, 50, -1, 20): по горизонтали — стиль, сверху 50, снизу 20.
    m_pGridLayout->setContentsMargins(-1, 50, -1, 20);

    m_pBtnOK = new QPushButton(host);
    m_pBtnOK->setObjectName(QStringLiteral("btn_OK"));
    m_pBtnOK->setMinimumSize(75, 0);
    m_pBtnOK->setMaximumSize(100, 16777215);
    m_pGridLayout->addWidget(m_pBtnOK, 1, 1, 1, 1);

    m_pListWidget = new QListWidget(host);
    m_pListWidget->setObjectName(QStringLiteral("listWidget"));
    // Реф.: список занимает всю верхнюю строку — три колонки.
    m_pGridLayout->addWidget(m_pListWidget, 0, 0, 1, 3);

    setWindowTitle(tr("TR_Dlg"));
    m_pBtnOK->setText(tr("TR_OK"));
    QMetaObject::connectSlotsByName(this);

    SetKStyle(KDLG_W460);
    // Реф. отдельно перекрывает титул KDialog::setWindowTitle(tr("TR_STest")) —
    // у нас титул-бар KDialog ставится через SetTitle.
    SetTitle(tr("TR_STest"));

    m_infoList.clear();
    connect(m_pBtnOK, SIGNAL(clicked()), this, SLOT(close()));

    // Реф.: отчёт собирается ПРЯМО В КОНСТРУКТОРЕ, до показа окна.
    startCheck();
    displayInfo();
}

void KSelfTest::startCheck()
{
    m_infoList << tr("TR_INPTSTest:");
    // Порядок в реф. именно такой: процессор → эндоскоп → свет.
    checkProcessor();
    checkEndo();
    checkLight();
    m_infoList << tr("TR_ICompleted");
    m_infoList.removeDuplicates();
}

void KSelfTest::checkProcessor()
{
    if (KSystemSet::GetInstance().GetProcessorSN().isEmpty())
        m_infoList << tr("TR_IPSNNI");   // серийный номер процессора не задан

    // Реф.: DataPath() + "protected/syspreset/testenv.ini" (конкатенация — DataPath
    // оканчивается на "/"), ключ AgeTest/IsAgeTest, дефолт false.
    const QString path = QDir(KSystem::DataPath())
                             .absoluteFilePath(QStringLiteral("protected/syspreset/testenv.ini"));
    QSettings env(path, QSettings::IniFormat);
    if (!env.value("AgeTest/IsAgeTest", false).toBool())
        m_infoList << tr("TR_IPATestNE");   // прогонный (age) тест не пройден/не включён
}

void KSelfTest::checkEndo()
{
    KEndoScope *endo = GetEndoScope();
    const _EndoInfoStruct *info = endo->EndoInfo();      // реф. GetEndoInfo()
    const _EepromInfo *eeprom = endo->EepromData();      // реф. GetEepromData()

    if (info->sModel.isEmpty())
        m_infoList << tr("TR_EModelNC");                 // модель эндоскопа не определена

    if (info->sEndoID.isEmpty() || info->sWarrantyDate.isEmpty())
        m_infoList << tr("TR_EInfomationNI");            // sic: Infomation — опечатка вендора

    // ⚠️ Полярность ОБРАТНАЯ предыдущим: строка добавляется, когда признаки НАЙДЕНЫ —
    // счётчик использований не нулевой ЛИБО заполнены сервис-контракт/комментарий,
    // т.е. эндоскоп уже был в работе.
    if (eeprom->usedCount != 0 || !info->sServerContract.isEmpty() || !info->sComment.isEmpty())
        m_infoList << tr("TR_IROEndoscopeNE");

    // fixFlags: бит 1 — наличие памяти баланса белого, бит 0 — калибровка.
    const unsigned char flags = eeprom->fixFlags;
    if (!(flags & 0x02))
        m_infoList << tr("TR_WBMemoryNE");
    if ((flags & 0x03) == 0x02)
        m_infoList << tr("TR_VACalibrationNE");
}

void KSelfTest::checkLight()
{
    const QPair<int, int> t = g_lampTime();
    // Реф.: `cmp used,#29; ccmp total,#29,#0,hi; b.ls` — строка добавляется, если
    // ХОТЯ БЫ один счётчик <= 29 (порог именно 29, сравнение строгое «больше»).
    if (t.first <= 29 || t.second <= 29)
        m_infoList << tr("TR_CLATestNE");
}

void KSelfTest::displayInfo()
{
    m_pListWidget->clear();
    // ⚠️ Квирк реф.: позиция вставки берётся ПОСЛЕ clear(), т.е. всегда 0.
    m_pListWidget->insertItems(m_pListWidget->count(), m_infoList);
}

void OpenKSelfTestDlg()
{
    KSelfTest *dlg = new KSelfTest(nullptr);
    dlg->exec();
    delete dlg;
}
