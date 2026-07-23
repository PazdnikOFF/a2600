#include "KReportPreviewCenterDlg.h"

#include <QGridLayout>
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QGraphicsView>
#include <QScrollBar>
#include <QTextDocument>

#include "report/KRTSimpleDisplay.h"
#include "report/KRTTeDisplay.h"
#include "report/KRTAbsDataSource.h"
#include "report/KRTTeCreatorContext.h"

KReportPreviewCenterDlg::KReportPreviewCenterDlg(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x4fc948: init + Create.
    Create();
}

KReportPreviewCenterDlg::~KReportPreviewCenterDlg()
{
    delete m_doc;
    delete m_previewPrinter;
}

void KReportPreviewCenterDlg::Create()
{
    // Реф. @0x4fc0c8.
    m_layout = new QGridLayout(this);

    m_previewPrinter = new QPrinter(QPrinter::HighResolution);
    InitPrinter(m_previewPrinter);

    m_preview = new QPrintPreviewWidget(m_previewPrinter, this);
    m_layout->addWidget(m_preview);
    setLayout(m_layout);

    m_view = m_preview->findChild<QGraphicsView *>();
    if (m_view) {
        m_view->horizontalScrollBar()->setVisible(false);   // реф.: горизонт. скроллбар скрыт
        m_vScroll = m_view->verticalScrollBar();
        m_view->setBackgroundBrush(QColor(QStringLiteral("#141519")));   // тёмный фон превью
        m_view->setFocus(Qt::OtherFocusReason);
    }

    connect(m_preview, &QPrintPreviewWidget::paintRequested,
            this, &KReportPreviewCenterDlg::OnReportPreview);
}

void KReportPreviewCenterDlg::InitPrinter(QPrinter *p)
{
    // Реф. @0x4fbf18: A4 / Portrait / 150dpi / PDF.
    p->setOutputFormat(QPrinter::PdfFormat);
    p->setResolution(150);
    p->setPageSize(QPrinter::A4);
    p->setOrientation(QPrinter::Portrait);
    // outputFileName = examDir + "/report.pdf" — device-путь, опущено.
}

void KReportPreviewCenterDlg::SetPreviewHtml(const QString &html)
{
    // Порт-стаб: реф. контент строит report-template модель (KRT*Display→QTextDocument) — DEVICE.
    if (!m_doc)
        m_doc = new QTextDocument();
    m_doc->setHtml(html);
    UpdatePreview();
}

void KReportPreviewCenterDlg::UpdatePreview()
{
    // Реф. @0x4fbec8: updatePreview() → повторный paintRequested → OnReportPreview.
    if (m_preview)
        m_preview->updatePreview();
}

void KReportPreviewCenterDlg::SetReportSource(KRTAbsDataSource *ds,
                                             const KReportTemplateDataNew *data)
{
    // Реф. диалог держит оба движка полями (+0x68 simpleDisp, +0x70 teDisp).
    m_simpleDisp = ds ? std::make_unique<KRTSimpleDisplay>(ds) : nullptr;
    m_teDisp     = ds ? std::make_unique<KRTTeDisplay>(ds) : nullptr;
    m_pData = data;
}

KRTTeCreatorContext *KReportPreviewCenterDlg::Context() const
{
    return m_teDisp ? m_teDisp->Context() : nullptr;
}

bool KReportPreviewCenterDlg::IsPipelineReady() const
{
    return m_simpleDisp && m_teDisp && m_pData;
}

void KReportPreviewCenterDlg::OnReportPreview(QPrinter *printer)
{
    // ⭐ Реф. @0x4fdce8 — ЧЕТЫРЕ ШАГА, сверены дизасмом:
    //   1) simpleDisp.m_displayParam.Reset();
    //   2) simpleDisp.Display(data)      — ВЫЧИСЛЯЕТ набор валидных элементов (+0x70);
    //   3) teDisp.Reset();
    //   4) SetRefValidItems(&teDisp.m_displayParam, simpleDisp.…m_setValidItems)
    //      — набор кладётся в Te как РЕФЕРЕНСНОЕ множество (+0xa0), по которому
    //        KRTTeAbsItemCreator::CheckCreate потом фильтрует элементы.
    // Затем Te рендерит документ и он печатается в принтер превью.
    if (IsPipelineReady()) {
        m_simpleDisp->DisplayParam().Reset();
        m_simpleDisp->Display(*m_pData);
        m_teDisp->Reset();
        m_teDisp->SetRefValidItems(m_simpleDisp->DisplayParam().ValidItemsStd());

        m_teDisp->Display(*m_pData, printer);
        if (QTextDocument *doc = m_teDisp->Document()) {
            doc->setPageSize(printer->pageRect().size());
            doc->print(printer);
            return;
        }
    }
    // Фолбэк: инъектированный html (прежний порт-стаб) — когда конвейер не подключён.
    if (m_doc) {
        m_doc->setPageSize(printer->pageRect().size());
        m_doc->print(printer);
    }
}

void KReportPreviewCenterDlg::OnBtnFitPage()
{
    if (m_preview)
        m_preview->fitInView();
}

void KReportPreviewCenterDlg::OnBtnFitWidth()
{
    if (m_preview)
        m_preview->fitToWidth();
}

void KReportPreviewCenterDlg::SetZoomFactor(double z)
{
    if (m_preview)
        m_preview->setZoomFactor(z);
}

double KReportPreviewCenterDlg::GetZoomFactor() const
{
    return m_preview ? m_preview->zoomFactor() : 1.0;
}
