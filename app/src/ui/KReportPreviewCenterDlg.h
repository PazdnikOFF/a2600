#pragma once

#include <QWidget>
#include <QString>

class QGridLayout;
class QPrinter;
class QPrintPreviewWidget;
class QGraphicsView;
class QScrollBar;
class QTextDocument;

// Поверхность превью отчёта (реф. KReportPreviewCenterDlg @ctor 0x4fc948, base QWidget,
// Create @0x4fc0c8). UI-порт. QGridLayout + QPrintPreviewWidget(previewPrinter) на тёмном фоне
// #141519, горизонт. скроллбар скрыт. Реф. контент рендерит OnReportPreview из report-template
// модели (KReportTemplateManager/KRTSimpleDisplay/KRTTeDisplay→QTextDocument) — DEVICE. В порте
// оболочка превью + инъектируемый QTextDocument (SetPreviewHtml). Кнопок нет — вью-контролы
// (fit/zoom) зовёт владелец. Принтеры A4/Portrait/150dpi/PDF.
class KReportPreviewCenterDlg : public QWidget
{
    Q_OBJECT
public:
    explicit KReportPreviewCenterDlg(QWidget *parent = nullptr);
    ~KReportPreviewCenterDlg() override;

    void SetPreviewHtml(const QString &html);   // порт-стаб контента (DEVICE-seam)
    void UpdatePreview();                        // реф. @0x4fbec8 → updatePreview
    void OnBtnFitPage();                         // реф. @0x4fbf00
    void OnBtnFitWidth();                        // реф. @0x4fbef8
    void SetZoomFactor(double z);                // реф. @0x4fbf08
    double GetZoomFactor() const;                // реф. @0x4fbf10

private slots:
    void OnReportPreview(QPrinter *printer);     // реф. @0x4fdce8 (paintRequested): рендер

private:
    void Create();
    void InitPrinter(QPrinter *p);   // реф. @0x4fbf18: A4/Portrait/150dpi/PDF

    QGridLayout *m_layout = nullptr;
    QPrinter *m_previewPrinter = nullptr;   // +0x38
    QPrintPreviewWidget *m_preview = nullptr;   // +0x30
    QGraphicsView *m_view = nullptr;        // +0x58
    QScrollBar *m_vScroll = nullptr;        // +0x60
    QTextDocument *m_doc = nullptr;         // порт: контент-документ (стаб)
};
