#pragma once

#include <string>

#include "KReportTemplateData.h"

class QTextDocument;
class KRTCreatorContext;

// Генератор документа отчёта (реф. KDocumentGenerator @0x53bf38…0x541dac, X-2600).
// ЖИВОЙ класс: 12 внешних вызывающих (KNewTempletEditor, KReportTempletEditDlg).
// Не путать с мёртвым KTemplateEditDocument (0 вызывающих, IS-A QTextDocument) —
// см. docs/PROGRESS.md §10.
//
// Раскладка реф. (sizeof 0x40, БЕЗ базовых классов, НЕ QObject, не полиморфный —
// хранится в shared_ptr, аллокация 0x50 в KReportTempletEditDlg::InitTemplate):
//   +0x00 QTextDocument*          m_pDoc         (владение через parent QObject)
//   +0x08 KRTCreatorContext*      m_pContext     (new в ctor)
//   +0x10 KReportTemplateDataNew* m_pData        (НЕ владеет)
//   +0x18 std::string             m_strCurItemId (инициализируется STR_INVALID_ITEM_ID)
//   +0x38 bool                    m_bCanMoveFront
//   +0x39 bool                    m_bCanMoveBack
//
// ГРАНИЦЫ Qt5::Widgets У КЛАССА НЕТ: реф. GetTextDocument(QTextEdit* p) использует p
// ТОЛЬКО как QObject*-родителя для `new QTextDocument(p)`. Поэтому в нашей версии
// параметр объявлен как QObject* — поведение то же, зависимость от Widgets снята.
//
// СТАТУС РЕАЛИЗАЦИИ (итерация 1): реализовано ТОЛЬКО write-pure подмножество —
// чистый STL, без Qt вообще, проверяется self-test'ом `docgen`. Документная половина
// (InitDocument/GetTextDocument/Find*/Move*/Change*Selected и всё, что зовёт
// KRTCreatorContext::CreateBlock) НЕ реализована: нужен KRTCreatorContext +
// KRTAbsItemCreator + KRTTextItemCreator. m_pContext поэтому всегда nullptr.
class KDocumentGenerator
{
public:
    // Реф. ctor: m_pDoc=0; m_pData=p; m_strCurItemId=STR_INVALID_ITEM_ID; флаги=0;
    // m_pContext = new KRTCreatorContext(p). У нас контекст не создаётся (не реализован).
    explicit KDocumentGenerator(KReportTemplateDataNew *pData);

    // --- write-pure (реализовано) ---

    // Реф. Save @0x541628: копия doc-атрибутов (map assign) + глубокая копия списка
    // элементов. Qt не участвует вообще.
    void Save(KReportTemplateDataNew &out) const;

    // Реф. ChangeCalcApps @0x53d340: docAttrs["CalcApp"] = value.
    void ChangeCalcApps(const std::string &value);

    // Реф. SetLayoutParam @0x53c5f0: в конфиг элемента пишется атрибут "RefColumn".
    // Имя атрибута сверено с реальными шаблонами прошивки (RefColumn встречается 14 раз
    // в system/presetdata/.../report/template/**.xml).
    bool SetLayoutParam(const std::string &id, const std::string &value);

    // Реф. HasFooterTemplateItem @0x53c680: есть ли элемент с атрибутом Section=="Footer".
    // Значения Section в поставке ровно три: Body / Footer / Header.
    bool HasFooterTemplateItem() const;

    // Реф. FindItmeIdofPreFooter @0x53d210 (опечатка Itme — реф., сохраняем 1:1):
    // id последнего элемента ПЕРЕД первым, у которого Section=="Footer".
    // Пусто, если футера нет либо он первый.
    std::string FindItmeIdofPreFooter() const;

    // Реф. UpdateMovableFlag @0x53c7b0 (восстановлено ДЕКОМПИЛЯТОРОМ Ghidra, не asm).
    // Выставляет m_bCanMoveFront/Back для элемента id по его позиции среди СОСЕДЕЙ:
    //   • оба флага сбрасываются;
    //   • id == STR_INVALID_ITEM_ID → выход;
    //   • сам элемент cell-закреплён (его конфиг имеет атрибут "CellAt") → выход;
    //   • front = есть предыдущий сосед И он НЕ cell-закреплён;
    //   • back  = есть следующий сосед И он НЕ cell-закреплён.
    // Список соседей = дети родителя (id без последнего "/"-сегмента), либо корневой
    // список, если родитель не найден. Cell-закрепление = наличие атрибута "CellAt"
    // (в поставляемых шаблонах не встречается — на реальных данных соседи всегда движимы).
    void UpdateMovableFlag(const std::string &id);

    // --- состояние ---
    const std::string &CurItemId() const { return m_strCurItemId; }
    bool CanMoveFront() const { return m_bCanMoveFront; }
    bool CanMoveBack() const  { return m_bCanMoveBack; }

    // Реф. STR_INVALID_ITEM_ID = "Invalid ID" (восстановлено декомпилятором из
    // _GLOBAL__sub_I_KDocumentGenerator.cpp: string(&STR_INVALID_ITEM_ID,"Invalid ID")).
    // Этой строкой реф. ctor инициализирует m_strCurItemId (наш ctor — теперь тоже).
    static const std::string &InvalidItemId();

private:
    // Значение атрибута конфига элемента по id; пусто, если конфига/атрибута нет.
    std::string itemAttr(const std::string &id, const std::string &attr) const;
    // Есть ли у конфига элемента атрибут "CellAt" (cell-закрепление).
    bool isCellPinned(const std::string &id) const;
    // Список соседей элемента id (дети родителя либо корневой список).
    const std::list<KReportTemplateItem> *siblingsOf(const std::string &id) const;

    QTextDocument          *m_pDoc = nullptr;      // +0x00
    KRTCreatorContext      *m_pContext = nullptr;  // +0x08 — НЕ реализован (итерация 2)
    KReportTemplateDataNew *m_pData = nullptr;     // +0x10
    std::string             m_strCurItemId;        // +0x18
    bool                    m_bCanMoveFront = false;  // +0x38
    bool                    m_bCanMoveBack = false;   // +0x39
};
