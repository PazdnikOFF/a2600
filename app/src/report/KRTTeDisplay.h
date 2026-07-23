#pragma once

#include <memory>

#include "report/KRTSimpleDisplay.h"   // KRTAbsDisplay

class KRTTeCreatorContext;
class QPrinter;
class QTextDocument;

// «Te»-движок отображения отчёта (реф. KRTTeDisplay : KRTAbsDisplay, ctor @0x5121e8,
// sizeof 0xF0). Рендерит в QTextDocument; вторая перегрузка Display — на печать.
//
// Layout реф. — тот же приём, что у KRTSimpleDisplay: база (vptr + KRTAbsDataSource*
// +0x08), ВСТРОЕННЫЙ ПО ЗНАЧЕНИЮ `KReportDisplayParam` +0x10 (0xD0 байт),
// `QTextDocument*` +0xE0, `KRTTeCreatorContext*` +0xE8.
//
// ⭐ ЗАМЫКАНИЕ СВЯЗКИ ДВУХ ДВИЖКОВ (найдено дизасмом call site
// `KReportPreviewCenterDlg::OnReportPreview(QPrinter*)` @0x4fdce8):
//   1) simpleDisp.m_displayParam.Reset();
//   2) simpleDisp.Display(dataNew)  — вычисляет НАБОР ВАЛИДНЫХ в своём параметре (+0x70);
//   3) teDisp.Reset();
//   4) SetRefValidItems(&teDisp.m_displayParam, simpleDisp.m_displayParam.m_setValidItems)
//      — набор из Simple кладётся в Te как РЕФЕРЕНСНОЕ множество (+0xa0).
// После этого `KRTTeAbsItemCreator::CheckCreate` фильтрует элементы через
// `IsItemValid`, который читает именно ref-множество. Это и объясняет «асимметрию»,
// зафиксированную при разборе KReportDisplayParam (C8): множества РАЗНЫЕ намеренно —
// одно наполняет Simple, другое читает Te.
// (Именованная обёртка `KRTTeDisplay::SetRefValidItems` @0x512238 в бинарнике никем
// не вызывается — OnReportPreview зовёт базовый метод напрямую.)
class KRTTeDisplay : public KRTAbsDisplay
{
public:
    explicit KRTTeDisplay(KRTAbsDataSource *pDataSource);
    ~KRTTeDisplay() override;

    void InitMembers();          // реф. @0x512198 — создаёт KRTTeCreatorContext
    bool Reset() override;       // реф. @0x512150 — параметр + контекст + удалить документ

    // Реф. @0x512238 — тонкий форвард в KReportDisplayParam::SetRefValidItems на +0x10.
    void SetRefValidItems(const std::set<std::string> &items);

    // Реф. @0x512240.
    bool Display(const KReportTemplateDataNew &data) override;
    // Реф. @0x512ae8: `printer == nullptr` → НЕМЕДЛЕННЫЙ false (самая первая инструкция).
    bool Display(const KReportTemplateDataNew &data, QPrinter *printer);

    QTextDocument *Document() const { return m_pDocument; }
    KRTTeCreatorContext *Context() const { return m_pContext.get(); }
    KReportDisplayParam &DisplayParam() { return m_displayParam; }

private:
    bool DisplayImpl(const KReportTemplateDataNew &data, QPrinter *printer);

    KReportDisplayParam m_displayParam;                  // +0x10 (по значению)
    QTextDocument      *m_pDocument = nullptr;           // +0xE0 (владеет)
    std::unique_ptr<KRTTeCreatorContext> m_pContext;     // +0xE8
};
