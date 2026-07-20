#pragma once

#include <string>

#include "KReportTemplateData.h"

class QTextDocument;
class QTextFrame;
class QTextTableCell;
class QObject;
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
    ~KDocumentGenerator();

    // Владеет m_pContext (сырой указатель) — копирование запрещено во избежание double-free.
    KDocumentGenerator(const KDocumentGenerator &) = delete;
    KDocumentGenerator &operator=(const KDocumentGenerator &) = delete;

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

    // --- под-элементная CRUD (ядро модели, Qt-free; UI-обёртки — документная итерация) ---

    // Реф. AddSubItemData @0x540bf0: вставить копию item в список под parentId по позиции
    // pos (0 → в начало; pos ≥ размера → в конец), слить cfgMap в m_pData->m_mapItemConfigs.
    //   • parentId == "" → корень m_lstItems; иначе FindRefItem(parentId), промах →
    //     лог "not find parent item, add subitem failed" + выход (ничего не вставлено);
    //   • дедуп: если элемент с item.m_strID уже есть → RemoveSubItem(parentId, item.m_strName)
    //     перед вставкой;
    //   • слияние конфигов: upsert по ключу (нет → вставка; есть → перезапись
    //     m_bUserDefine/m_strName/m_mapAttrs);
    //   • хвост: для image-text-map id — SyncRefresnImageItemData (пересборка колонок).
    // Реф. возвращает ulong (нормальный путь -1), но ВСЕ вызывающие возврат игнорируют → void.
    void AddSubItemData(const std::string &parentId, const KReportTemplateItem &item,
                        const std::map<std::string, KReportTemplateItemConfig> &cfgMap,
                        int pos);

    // Реф. DeleteSubItemData @0x53fb10: удалить под-элемент item из списка под id.
    //   • id == "" → удалить из корня ВСЕ узлы с m_strID == item.m_strID; иначе
    //     RemoveSubItem(id, item.m_strName);
    //   • каскад: стереть из m_mapItemConfigs все записи, чей ключ содержит <id-удаляемого>;
    //   • хвост: для image-text-map id — SyncRefresnImageItemData.
    void DeleteSubItemData(const std::string &id, const KReportTemplateItem &item);

    // --- синхронизация колонок image-text-map (реф., восстановлено ДЕКОМПИЛЯТОРОМ) ---
    // Это слой данных под живой раскладкой RT_IMAGE_TEXT_MAP (галерея снимков N×M):
    // колонки MAP1/MAP2/… «зеркалят» эталонную MAP0 через атрибут SynColumnID.
    // Все три метода — чистый STL, Qt не участвует (проверено дизасмом: прямых QText*-
    // вызовов нет). Их оркестратор SyncRefresnImageItemData — итерация 3 (нужен
    // KReportTemplateManager::GetTempletLibName + обвязка синглтонов).

    // Реф. GetAllItemIDs @0x53fdb0: все id колонок, «зеркалящих» колонку id (плюс сам id).
    // Нестатический член, но this в теле реф. НЕ используется (по сути статический).
    //   • Нормализация: id == "/RT_IMAGE_TEXT_MAP/RT_IMAGE_TEXT_MAP0" → cur="/RT_IMAGE_TEXT_MAP";
    //     id == EXT-вариант "/RT_MAIN_CONTENT/.../RT_IMAGE_TEXT_MAP0" → cur="/RT_MAIN_CONTENT/
    //     RT_IMAGE_TEXT_MAP" (relative→relative, prefixed→prefixed).
    //   • cur НЕ содержит "/RT_IMAGE_TEXT_MAP" → результат ровно [cur].
    //   • иначе: конфиги, чей ключ содержит "/RT_IMAGE_TEXT_MAP" и attrs["SynColumnID"]==cur,
    //     дают свои id (в порядке ключа map), затем ОДНИМ последним push — сам cur.
    std::list<std::string> GetAllItemIDs(const std::string &id,
                                         const KReportTemplateDataNew &data) const;

    // Реф. SyncImageItemContent @0x53c490: приводит состав суб-элементов target к proto —
    // из target УДАЛЯЮТСЯ суб-элементы, чьё m_strName ОТСУТСТВУЕТ среди суб-элементов
    // proto. Фильтр по ИМЕНИ (m_strName, +0x20), НЕ по id. this в теле не используется.
    void SyncImageItemContent(const KReportTemplateItem &proto,
                              KReportTemplateItem &target) const;

    // Реф. SyncImageItemParam @0x53eb20: тянет конфиги суб-элементов из libData в
    // m_pData->m_mapItemConfigs. GetSubItemsParam(libData, "RT_IMAGE_TEXT_MAP", tmp) →
    // для каждого (itemId, cfg) с атрибутом SynColumnID: если значение SynColumnID есть
    // ключом в наших конфигах → записать cfg под ключом itemId; иначе — стереть записи
    // по itemId (устаревшая колонка). Конфиги без SynColumnID пропускаются.
    void SyncImageItemParam(const KReportTemplateDataNew &libData);

    // Реф. SyncRefresnImageItemData @0x53efa0 (опечатка Refresn вендора сохранена).
    // Оркестратор: пересобирает поддерево колонок RT_IMAGE_TEXT_MAP в m_pData из
    // библиотечных данных ВЫБРАННОГО шаблона, фильтруя состав по прежней первой колонке.
    // Тянет синглтоны KSysReportTempletControl (выбранный шаблон) + KReportTemplateManager
    // (имя библиотеки + lib-данные). Qt-free, но требует их инициализации. Шаги:
    //   1. templName = выбранный шаблон; 2. libName = GetTempletLibName(templName);
    //   3. libData = GetTemplateLibCfg()->GetTemplateLib(libName) (при промахе — &m_data);
    //   4. refKey = "/RT_IMAGE_TEXT_MAP", но templName=="NP-4x1" → EXT-префикс;
    //   5. top = FindRefItem(m_pData, refKey); null / пустой список → выход;
    //   6. proto = копия top.children[0] (ДО очистки); 7. top.children очищается;
    //   8. libTop = FindConstRefItem(libData, refKey); для каждого его ребёнка —
    //      resolve по id, SyncImageItemContent(proto, копия) для обрезки состава, push в top;
    //      затем SyncImageItemParam(libData).
    // ⚠️ Ключевое (сверено СЫРЫМ asm, декомпилятор путал стек-слоты): с "NP-4x1"
    // сравнивается templName (сырое имя), а в GetTemplateLib идёт libName (префикс
    // "ReportTemplate..."). Логи реф. ("image top item is null" и т.п.) опущены (off-device).
    void SyncRefresnImageItemData();

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

    // --- документная/render-половина (Qt; итерация 5) ---

    // Реф. GetTextDocument @0x53eac0: ТРИВИАЛЬНАЯ фабрика — m_pDoc = new QTextDocument(parent);
    // InitDocument(); return m_pDoc. parent используется ТОЛЬКО как QObject-родитель (в реф.
    // QTextEdit*), поэтому у нас QObject* — зависимость от Widgets снята (как в комментарии .h).
    QTextDocument *GetTextDocument(QObject *parent);

    // Реф. InitDocument @0x53e108: строит содержимое m_pDoc по m_pData. clear() → формат
    // rootFrame (BgColor/margin 20/ширина 100%) → дефолтный шрифт → цикл по m_lstItems с
    // m_pContext->CreateBlock(item.m_strType, &item, rootFrame) → PutFooterOnBottom().
    // МИНИМАЛЬНЫЙ рендер: опущены подстановка номеров страниц и split-линии (помечено).
    void InitDocument();

    // Реф. PutFooterOnBottom @0x53e078: если есть футер-элемент — прижать его к низу
    // страницы через InsertBlockLineAfterItem(id-элемента-перед-футером). Нет футера → no-op.
    void PutFooterOnBottom();

    // Реф. InsertBlockLineAfterItem @0x53dc90: добить документ пустыми абзацами после
    // элемента id, пока высота < editMaxHeight-9 (выравнивание футера по низу). Для min
    // без футера — документированный no-op (тянет FindFrameOrCell + метрики layout).
    void InsertBlockLineAfterItem(const std::string &id);

    // --- примитивы поиска блока в документе (Qt; редакторная половина) ---

    // Реф. FindFrameOrCell @0x53ca28: рекурсивный обход дерева фреймов от frame; ищет элемент
    // с ElementId==id (property UserProperty+1 на frameFormat фрейма ИЛИ на format ячейки
    // таблицы). Совпал фрейм → *outFrame=frame; совпала ячейка → outCell=cell (взаимоисключимо).
    // Таблицы (qobject_cast<QTextTable*>) обходятся по ячейкам + рекурсия во вложенные фреймы
    // ячеек; обычные фреймы — по childFrames. true при находке. Qt-чистый.
    bool FindFrameOrCell(QTextFrame *frame, const std::string &id,
                         QTextFrame **outFrame, QTextTableCell &outCell) const;

    // Реф. GetSelectFrame @0x53d1b0 / GetSelectCell @0x53d160: найти по m_strCurItemId от
    // rootFrame документа. GetSelectFrame → фрейм (nullptr если это ячейка/не найдено);
    // GetSelectCell → ячейка (невалидна если это фрейм/не найдено). Guard: m_pDoc != null.
    QTextFrame    *GetSelectFrame() const;
    QTextTableCell GetSelectCell() const;

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
