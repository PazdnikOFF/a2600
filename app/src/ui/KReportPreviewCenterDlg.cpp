#include "KReportPreviewCenterDlg.h"

#include <QGridLayout>
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QGraphicsView>
#include <QScrollBar>
#include <QTextDocument>

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

void KReportPreviewCenterDlg::OnReportPreview(QPrinter *printer)
{
    // Реф. @0x4fdce8: DEVICE — рендер report-doc из шаблон-модели + пагинация (SplitDocument).
    // В порте: печать инъектированного QTextDocument (стаб контента).
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
