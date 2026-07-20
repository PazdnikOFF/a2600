#include "KDocumentGenerator.h"

#include "KReportTemplateCommonDef.h"
#include "KRTCreatorContext.h"        // m_pContext: рендер блоков + GetFontSize
#include "KReportTemplate.h"          // KReportTemplateManager (синглтон, GetTempletLibName)
#include "KSysReportTempletControl.h" // KSysReportTempletControl (выбранный шаблон)
#include "KTemplateLibCfg.h"          // KTemplateLibCfg::GetTemplateLib

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextLength>
#include <QTextTable>
#include <QTextTableCell>

#include <iterator>

namespace {

// Константы реф. живут в namespace report_template как file-static std::string
// (по копии на TU). У нас их в KReportTemplateCommonDef.h нет, поэтому объявлены
// здесь с реф.-именами. ЗНАЧЕНИЯ сверены не с дизасмом, а с РЕАЛЬНЫМИ шаблонами
// прошивки (system/presetdata/syspreset/mainapp/patient/report/template/**.xml) —
// это более сильное доказательство, чем чтение статического инициализатора:
//   Section  — 40 вхождений, значения ровно три: "Body" / "Footer" / "Header"
//              (совпадает с тройкой символов STR_RT_VALUE_BODY/FOOTER/HEADER);
//   RefColumn — 14 вхождений.
const std::string STR_RT_ITEM_ATTR_SECTION   = "Section";
const std::string STR_RT_ITEM_ATTR_REFCOLUMN = "RefColumn";
const std::string STR_RT_VALUE_FOOTER        = "Footer";
// Значение сверено в .rodata (0x865d18: "CellAt\0\0"); в поставляемых шаблонах
// атрибут CellAt не встречается — на реальных данных cell-закрепления нет.
const std::string STR_RT_ITEM_ATTR_CELLAT    = "CellAt";

// Раскладка галереи снимков RT_IMAGE_TEXT_MAP: колонки MAP1/MAP2/… «зеркалят»
// эталонную MAP0 через атрибут SynColumnID. Имена вендорские, сверены с поставкой
// (system/presetdata/.../report/template/SubContent/*.xml) И с дизасмом статических
// инициализаторов (адреса .bss a97938/a97958/a97978/a97998).
//   "SynColumnID" — 92 вхождения (орфография вендора: SYN, НЕ SYNC). Значение — id
//   колонки MAP0, которую данная колонка зеркалит.
// ⚠️ ВНИМАНИЕ: needle-глобал реф. это "/RT_IMAGE_TEXT_MAP" СО СЛЭШЕМ, а голое
// "RT_IMAGE_TEXT_MAP" — ДРУГОЙ глобал (STR_TOP_IMAGE_TEXT_NAME, имя узла). Для реальных
// ключей конфигов (все начинаются с "/RT_IMAGE_TEXT_MAP") подстрочный тест совпадает,
// но литерал храним точный.
const std::string STR_REF_IMAGE_TEXT_MAP      = "/RT_IMAGE_TEXT_MAP";
const std::string STR_REF_IMAGE_TEXT_MAP0     = "/RT_IMAGE_TEXT_MAP/RT_IMAGE_TEXT_MAP0";
const std::string STR_REF_IMAGE_TEXT_MAP_EXT  = "/RT_MAIN_CONTENT/RT_IMAGE_TEXT_MAP";
const std::string STR_REF_IMAGE_TEXT_MAP0_EXT =
    "/RT_MAIN_CONTENT/RT_IMAGE_TEXT_MAP/RT_IMAGE_TEXT_MAP0";
const std::string STR_RT_ITEM_ATTR_SYNCOLUMNID = "SynColumnID";
// Реф. report_preview::NP_4x1 = "NP-4x1". СРАВНИВАЕТСЯ С templName (сырое имя выбранного
// шаблона), НЕ с libName — сверено сырым asm (декомпилятор путал стек-слоты sp+0x108
// templName / sp+0x128 libName). Для этой раскладки поддерево живёт под MAIN_CONTENT.
const std::string STR_NP_4X1 = "NP-4x1";

} // namespace

const std::string &KDocumentGenerator::InvalidItemId()
{
    // Реф. STR_INVALID_ITEM_ID = "Invalid ID" (декомпилятор, см. .h).
    static const std::string kInvalid = "Invalid ID";
    return kInvalid;
}

KDocumentGenerator::KDocumentGenerator(KReportTemplateDataNew *pData)
    : m_pContext(new KRTCreatorContext(pData)), m_pData(pData),
      m_strCurItemId(InvalidItemId())
{
    // Реф.: m_pDoc=0; m_pData=p; m_strCurItemId=STR_INVALID_ITEM_ID ("Invalid ID");
    // оба bool-флага=0; m_pContext = new KRTCreatorContext(p) — теперь СОЗДАЁТСЯ
    // (render-половина реализована, контекст регистрирует творцов блоков).
}

KDocumentGenerator::~KDocumentGenerator()
{
    // Реф. владеет контекстом; удаляем во избежание утечки (m_pDoc принадлежит
    // parent-QObject, его НЕ трогаем).
    delete m_pContext;
}

std::string KDocumentGenerator::itemAttr(const std::string &id,
                                         const std::string &attr) const
{
    if (!m_pData)
        return std::string();
    auto itCfg = m_pData->m_mapItemConfigs.find(id);
    if (itCfg == m_pData->m_mapItemConfigs.end())
        return std::string();
    auto itAttr = itCfg->second.m_mapAttrs.find(attr);
    if (itAttr == itCfg->second.m_mapAttrs.end())
        return std::string();
    return itAttr->second;
}

void KDocumentGenerator::Save(KReportTemplateDataNew &out) const
{
    // Реф. @0x541628: присваивание map doc-атрибутов + глубокая копия списка
    // элементов (в реф. — через 7 string::swap + _List_node_base::swap на узел;
    // у нас эквивалент даёт обычное присваивание std::list).
    // ВАЖНО: реф. копирует ТОЛЬКО m_mapConfigs и m_lstItems. Карта m_mapItemConfigs
    // в Save НЕ участвует — out сохраняет свою (сверено по трём копируемым
    // контейнерам в дизасме мёртвого близнеца и по отсутствию обращения к +0x48).
    if (!m_pData)
        return;
    out.m_mapConfigs = m_pData->m_mapConfigs;
    out.m_lstItems   = m_pData->m_lstItems;
}

void KDocumentGenerator::ChangeCalcApps(const std::string &value)
{
    // Реф. @0x53d340: docAttrs["CalcApp"] = value.
    // Ключ "CalcApp" — литерал реф. (0x865c58, длина 7).
    if (!m_pData)
        return;
    m_pData->m_mapConfigs["CalcApp"] = value;
}

bool KDocumentGenerator::SetLayoutParam(const std::string &id,
                                        const std::string &value)
{
    // Реф. @0x53c5f0: в конфиг элемента пишется атрибут STR_RT_ITEM_ATTR_REFCOLUMN.
    // Реф. ВСЕГДА возвращает 1 (в мёртвом близнеце — то же поведение).
    if (!m_pData)
        return true;
    m_pData->m_mapItemConfigs[id].m_mapAttrs[STR_RT_ITEM_ATTR_REFCOLUMN] = value;
    return true;
}

bool KDocumentGenerator::HasFooterTemplateItem() const
{
    // Реф. @0x53c680: обход m_pData->m_lstItems (список @+0x30), для каждого элемента
    // конфиг по его id, сравнение атрибута Section с "Footer" (в дизасме — memcmp,
    // т.е. обычное сравнение std::string).
    if (!m_pData)
        return false;
    for (const KReportTemplateItem &item : m_pData->m_lstItems) {
        if (itemAttr(item.m_strID, STR_RT_ITEM_ATTR_SECTION) == STR_RT_VALUE_FOOTER)
            return true;
    }
    return false;
}

bool KDocumentGenerator::isCellPinned(const std::string &id) const
{
    // Реф. UpdateMovableFlag: конфиг элемента существует И имеет атрибут "CellAt".
    if (!m_pData)
        return false;
    auto it = m_pData->m_mapItemConfigs.find(id);
    if (it == m_pData->m_mapItemConfigs.end())
        return false;
    return it->second.m_mapAttrs.count(STR_RT_ITEM_ATTR_CELLAT) != 0;
}

const std::list<KReportTemplateItem> *
KDocumentGenerator::siblingsOf(const std::string &id) const
{
    // Реф.: parentPath = id.substr(0, find_last_of("/")); при отсутствии "/"
    // find_last_of даёт npos, реф. клампит к длине → parentPath = весь id.
    // FindRefItem(parentPath): нашёлся → его дети (+0xe0); не нашёлся → корень (+0x30).
    if (!m_pData)
        return nullptr;
    const std::size_t pos = id.find_last_of('/');
    const std::string parentPath = id.substr(0, pos == std::string::npos ? id.size() : pos);
    const KReportTemplateItem *parent = report_template::FindConstRefItem(*m_pData, parentPath);
    return parent ? &parent->m_lstSubItems : &m_pData->m_lstItems;
}

std::list<std::string>
KDocumentGenerator::GetAllItemIDs(const std::string &id,
                                 const KReportTemplateDataNew &data) const
{
    // Реф. @0x53fdb0 (декомпилятор + сверка статических инициализаторов). Порядок 1:1.
    std::list<std::string> result;

    // Нормализация id колонки MAP0 к id её контейнера (реф. присваивает cur один из двух
    // глобалов — relative или MAIN_CONTENT-prefixed; ветка A проверяется первой).
    std::string cur = id;
    if (id == STR_REF_IMAGE_TEXT_MAP0)
        cur = STR_REF_IMAGE_TEXT_MAP;
    else if (id == STR_REF_IMAGE_TEXT_MAP0_EXT)
        cur = STR_REF_IMAGE_TEXT_MAP_EXT;

    // Не image-text-map id → результат ровно [cur], map не обходится (реф. ранний возврат).
    if (cur.find(STR_REF_IMAGE_TEXT_MAP) == std::string::npos) {
        result.push_back(cur);
        return result;
    }

    // Собрать конфиги колонок галереи (ключ содержит "/RT_IMAGE_TEXT_MAP") во временный map
    // (реф. — отдельный проход, отсюда порядок сортировки ключей на выходе).
    std::map<std::string, KReportTemplateItemConfig> tmp;
    for (const auto &kv : data.m_mapItemConfigs) {
        if (kv.first.find(STR_REF_IMAGE_TEXT_MAP) != std::string::npos)
            tmp.insert(kv);
    }
    // Колонки, зеркалящие cur (attrs["SynColumnID"] == cur) — их id, в порядке ключа map.
    for (const auto &kv : tmp) {
        auto itSyn = kv.second.m_mapAttrs.find(STR_RT_ITEM_ATTR_SYNCOLUMNID);
        if (itSyn != kv.second.m_mapAttrs.end() && itSyn->second == cur)
            result.push_back(kv.first);
    }
    // И сам cur — ОДНИМ безусловным push, последним.
    result.push_back(cur);
    return result;
}

void KDocumentGenerator::SyncImageItemContent(const KReportTemplateItem &proto,
                                              KReportTemplateItem &target) const
{
    // Реф. @0x53c490 (декомпилятор): оставить в target.m_lstSubItems только те
    // суб-элементы, чьё m_strName ЕСТЬ среди суб-элементов proto. Фильтр по ИМЕНИ
    // (реф. читает node payload +0x20 = m_strName), НЕ по id — легко перепутать.
    // this в теле реф. не используется (метод по сути статический).
    for (auto it = target.m_lstSubItems.begin(); it != target.m_lstSubItems.end();) {
        bool keep = false;
        for (const KReportTemplateItem &p : proto.m_lstSubItems) {
            if (p.m_strName == it->m_strName) {
                keep = true;
                break;
            }
        }
        if (keep)
            ++it;
        else
            it = target.m_lstSubItems.erase(it);
    }
}

void KDocumentGenerator::SyncImageItemParam(const KReportTemplateDataNew &libData)
{
    // Реф. @0x53eb20 (декомпилятор): синхронизировать конфиги колонок-зеркал из libData
    // в наш m_pData->m_mapItemConfigs. Реф. логирует "[info] … SyncImageItemParam" —
    // лог опущен (off-device).
    if (!m_pData)
        return;

    // Плоский сбор конфигов суб-элементов image-text-map (реф. GetSubItemsParam:
    // ключ конфига СОДЕРЖИТ подстроку "RT_IMAGE_TEXT_MAP").
    std::map<std::string, KReportTemplateItemConfig> tmp;
    report_template::GetSubItemsParam(libData, STR_REF_IMAGE_TEXT_MAP, tmp);

    for (const auto &kv : tmp) {
        const std::string &itemId = kv.first;
        const KReportTemplateItemConfig &cfg = kv.second;

        // Конфиг без SynColumnID — не колонка-зеркало, пропускаем.
        auto itSyn = cfg.m_mapAttrs.find(STR_RT_ITEM_ATTR_SYNCOLUMNID);
        if (itSyn == cfg.m_mapAttrs.end())
            continue;
        const std::string &synVal = itSyn->second;

        if (m_pData->m_mapItemConfigs.find(synVal) == m_pData->m_mapItemConfigs.end()) {
            // Колонка-эталон (значение SynColumnID) в наших данных отсутствует —
            // запись по этому itemId устарела, стираем (реф. equal_range+erase; ключ
            // map уникален → эквивалентно erase(itemId)).
            m_pData->m_mapItemConfigs.erase(itemId);
        } else {
            // Эталон есть — переносим конфиг зеркала под ключом itemId.
            KReportTemplateItemConfig &dst = m_pData->m_mapItemConfigs[itemId];
            dst.m_bUserDefine = cfg.m_bUserDefine;
            dst.m_strName     = cfg.m_strName;
            dst.m_mapAttrs    = cfg.m_mapAttrs;
        }
    }
}

void KDocumentGenerator::AddSubItemData(
    const std::string &parentId, const KReportTemplateItem &item,
    const std::map<std::string, KReportTemplateItemConfig> &cfgMap, int pos)
{
    // Реф. @0x540bf0. Возврат ulong (нормальный путь -1) реф. игнорируется → void.
    if (!m_pData)
        return;

    // 1. Целевой список: корень при пустом parentId, иначе дети найденного родителя.
    std::list<KReportTemplateItem> *target = nullptr;
    if (parentId.empty()) {
        target = &m_pData->m_lstItems;     // реф. здесь assert(list != nullptr)
    } else {
        KReportTemplateItem *parent = report_template::FindRefItem(*m_pData, parentId);
        if (!parent)
            return;   // реф. puts("not find parent item, add subitem failed") — лог опущен
        target = &parent->m_lstSubItems;
    }

    // 2. Дедуп: если элемент с таким id уже есть → удалить перед вставкой (по имени в
    //    parentId). КВИРК: RemoveSubItem не имеет root-фолбэка, поэтому при parentId=="" это
    //    no-op — дубль в корне НЕ удаляется (реф. так же). Список-объект target стабилен:
    //    erase ребёнка не двигает узел-родитель.
    if (report_template::FindConstRefItem(*m_pData, item.m_strID) != nullptr)
        report_template::RemoveSubItem(*m_pData, parentId, item.m_strName);

    // 3. Вставка глубокой копии item по позиции pos (0 → в начало; pos ≥ размера → в конец).
    //    Реф. разбивает на 3 ветки (пусто / pos==0 / advance), но результат = один insert
    //    в позицию min(pos, size) от начала.
    auto it = target->begin();
    for (int k = 0; k < pos && it != target->end(); ++k)
        ++it;
    target->insert(it, item);

    // 4. Слияние конфигов: upsert по ключу (нет → вставка; есть → перезапись трёх полей).
    for (const auto &kv : cfgMap) {
        KReportTemplateItemConfig &dst = m_pData->m_mapItemConfigs[kv.first];
        dst.m_bUserDefine = kv.second.m_bUserDefine;
        dst.m_strName     = kv.second.m_strName;
        dst.m_mapAttrs    = kv.second.m_mapAttrs;
    }

    // 5. Хвост: image-text-map id → пересборка колонок. ОБА find по item.m_strID (сверено
    //    сырым asm): прямое вхождение константы В id ЛИБО обратно — id как подстрока EXT.
    if (item.m_strID.find(STR_REF_IMAGE_TEXT_MAP) != std::string::npos
        || STR_REF_IMAGE_TEXT_MAP_EXT.find(item.m_strID) != std::string::npos)
        SyncRefresnImageItemData();
}

void KDocumentGenerator::DeleteSubItemData(const std::string &id,
                                           const KReportTemplateItem &item)
{
    // Реф. @0x53fb10.
    if (!m_pData)
        return;

    // 1. Удаление узла.
    if (id.empty()) {
        // Корень: убрать ВСЕ узлы с m_strID == item.m_strID (реф. цикл не прерывается —
        // обрабатывает возможные дубли по id).
        for (auto it = m_pData->m_lstItems.begin(); it != m_pData->m_lstItems.end();) {
            if (it->m_strID == item.m_strID)
                it = m_pData->m_lstItems.erase(it);
            else
                ++it;
        }
    } else {
        // Не корень: удалить ребёнка по ИМЕНИ (item.m_strName) из родителя id.
        report_template::RemoveSubItem(*m_pData, id, item.m_strName);
    }

    // 2. Каскад конфигов: стереть все записи, чей ключ СОДЕРЖИТ item.m_strID (сверено сырым
    //    asm — needle именно item.m_strID, НЕ parentId). Так вычищается сам элемент и его
    //    дети по префиксу id; сиблинги не затрагиваются.
    for (auto it = m_pData->m_mapItemConfigs.begin();
         it != m_pData->m_mapItemConfigs.end();) {
        if (it->first.find(item.m_strID) != std::string::npos)
            it = m_pData->m_mapItemConfigs.erase(it);
        else
            ++it;
    }

    // 3. Хвост: image-text-map id → пересборка (реф. Delete-пара проверяет ТОЛЬКО прямое
    //    вхождение константы, без обратной EXT-проверки Add-пары). По item.m_strID —
    //    консистентно с подтверждённым хвостом Add и каскадом выше.
    if (item.m_strID.find(STR_REF_IMAGE_TEXT_MAP) != std::string::npos)
        SyncRefresnImageItemData();
}

void KDocumentGenerator::SyncRefresnImageItemData()
{
    // Реф. @0x53efa0 (декомпилятор + сверка сырого asm по двум сравнениям). Порядок 1:1.
    if (!m_pData)
        return;

    // 1. Имя выбранного шаблона.
    const std::string templName = KSysReportTempletControl::GetInstance()
                                      ->GetSelectedTempletInfo()
                                      .TempletName()
                                      .toStdString();

    // 2. Имя библиотеки, содержащей шаблон. GetTempletLibName при промахе out НЕ трогает —
    //    поэтому libName стартует пустым (реф. так же строит пустую строку перед вызовом).
    std::string libName;
    KReportTemplateManager::GetInstance()->GetTempletLibName(templName, libName);

    // 3. Библиотечные данные раскладки. В GetTemplateLib идёт libName (префикс
    //    "ReportTemplate…", совпадает с ключами-группами); при промахе реф. отдаёт &m_data.
    KReportTemplateDataNew *libData =
        KReportTemplateManager::GetInstance()->GetTemplateLibCfg()->GetTemplateLib(libName);
    if (!libData)
        return;   // защитно: реф. не проверяет (GetTemplateLib никогда не null).

    // 4. Ключ поддерева image-text-map. РОВНО templName == "NP-4x1" → MAIN_CONTENT-префикс.
    std::string refKey = STR_REF_IMAGE_TEXT_MAP;
    if (templName == STR_NP_4X1)
        refKey = STR_REF_IMAGE_TEXT_MAP_EXT;

    // 5. Верхний узел галереи в НАШИХ данных.
    KReportTemplateItem *top = report_template::FindRefItem(*m_pData, refKey);
    if (!top)
        return;                          // реф. лог "...image top item is null" (опущен)
    if (top->m_lstSubItems.empty())
        return;                          // реф. лог "...image top item child is empty"

    // 6. Прототип состава — из ПЕРВОЙ колонки, СНЯТ ДО очистки (по нему обрежется каждая
    //    новая колонка: SyncImageItemContent фильтрует по именам суб-элементов proto).
    KReportTemplateItem proto = top->m_lstSubItems.front();

    // 7. Старые колонки стираются.
    top->m_lstSubItems.clear();

    // 8. Пересобрать колонки из библиотеки.
    const KReportTemplateItem *libTop = report_template::FindConstRefItem(*libData, refKey);
    if (libTop) {
        for (const KReportTemplateItem &child : libTop->m_lstSubItems) {
            // Резолв реального lib-элемента по id (реф. — повторный Find, хотя child уже узел).
            const KReportTemplateItem *resolved =
                report_template::FindConstRefItem(*libData, child.m_strID);
            if (!resolved)
                continue;                // защитно
            KReportTemplateItem newItem = *resolved;   // копия: 7 строк + саб-список
            SyncImageItemContent(proto, newItem);      // обрезать состав по прототипу
            top->m_lstSubItems.push_back(newItem);      // hook в top
        }
        // Синхронизировать конфиги колонок из библиотеки в m_pData->m_mapItemConfigs.
        SyncImageItemParam(*libData);
    }
}

void KDocumentGenerator::UpdateMovableFlag(const std::string &id)
{
    // Реф. @0x53c7b0 (восстановлено декомпилятором Ghidra). Порядок 1:1.
    m_bCanMoveFront = false;
    m_bCanMoveBack = false;

    if (id == InvalidItemId())
        return;
    // Сам элемент cell-закреплён → не двигается вообще.
    if (isCellPinned(id))
        return;

    const std::list<KReportTemplateItem> *sibs = siblingsOf(id);
    if (!sibs)
        return;

    // Ищем элемент среди соседей, запоминая предыдущего и следующего.
    for (auto it = sibs->begin(); it != sibs->end(); ++it) {
        if (it->m_strID != id)
            continue;
        // Предыдущий сосед → можно ли двигать вперёд.
        if (it != sibs->begin()) {
            auto prev = std::prev(it);
            m_bCanMoveFront = !isCellPinned(prev->m_strID);
        }
        // Следующий сосед → можно ли двигать назад.
        auto nxt = std::next(it);
        if (nxt != sibs->end())
            m_bCanMoveBack = !isCellPinned(nxt->m_strID);
        return;
    }
}

std::string KDocumentGenerator::FindItmeIdofPreFooter() const
{
    // Реф. @0x53d210 (опечатка "Itme" — реф., сохранена 1:1): id последнего элемента
    // ПЕРЕД первым с Section=="Footer". Обход того же списка верхнего уровня.
    // Если футера нет или он самый первый — результат пуст.
    if (!m_pData)
        return std::string();
    std::string prev;
    for (const KReportTemplateItem &item : m_pData->m_lstItems) {
        if (itemAttr(item.m_strID, STR_RT_ITEM_ATTR_SECTION) == STR_RT_VALUE_FOOTER)
            return prev;
        prev = item.m_strID;
    }
    return std::string();
}

QTextDocument *KDocumentGenerator::GetTextDocument(QObject *parent)
{
    // Реф. @0x53eac0: тривиальная фабрика. parent — только QObject-родитель нового
    // QTextDocument (в реф. QTextEdit*), поэтому владение документом уходит parent'у.
    m_pDoc = new QTextDocument(parent);
    InitDocument();
    return m_pDoc;
}

void KDocumentGenerator::InitDocument()
{
    // Реф. @0x53e108. МИНИМАЛЬНЫЙ рендер: опущены (помечено) сброс члена-строки @+0x18,
    // подстановка номеров страниц, split-линии и begin/endEditBlock-группировка undo.
    if (!m_pDoc || !m_pContext || !m_pData)
        return;                                    // реф. assert(m_pDoc/m_pContext)

    m_pDoc->clear();

    // Формат корневого фрейма: фон из m_mapConfigs["BgColor"], поле 20, ширина 100%.
    QTextFrame *root = m_pDoc->rootFrame();
    QColor bg;
    bg.setNamedColor(QString::fromStdString(m_pData->m_mapConfigs["BgColor"]));
    QTextFrameFormat fmt = root->frameFormat();
    fmt.setMargin(20.0);
    if (bg.isValid())
        fmt.setBackground(QBrush(bg, Qt::SolidPattern));   // property BackgroundBrush 0x820
    fmt.setWidth(QTextLength(QTextLength::PercentageLength, 100.0));  // FrameWidth 0x4003
    root->setFrameFormat(fmt);   // публичный эквивалент protected QTextObject::setFormat

    // Дефолтный шрифт документа (реф. GetFontSize(nullptr) → база + DPI-скейл).
    m_pDoc->setDefaultFont(m_pContext->GetFontSize(static_cast<KReportTemplateItem *>(nullptr)));

    // Главный цикл: по элементам верхнего уровня → творец блока каждого типа.
    if (!m_pData->m_lstItems.empty()) {
        for (KReportTemplateItem &item : m_pData->m_lstItems)
            m_pContext->CreateBlock(item.m_strType, &item, root);
        // [ОМИТ min] между элементами InsertSplitLine при наличии split-линии.
        PutFooterOnBottom();
    }
}

void KDocumentGenerator::PutFooterOnBottom()
{
    // Реф. @0x53e078: если есть футер — набить пустыми абзацами перед ним (прижать к низу
    // страницы). Само прижатие — в InsertBlockLineAfterItem (см. ниже; для min без футера
    // это no-op).
    if (!HasFooterTemplateItem())
        return;
    const std::string preId = FindItmeIdofPreFooter();   // опечатка "Itme" — реф.
    if (!preId.empty())
        InsertBlockLineAfterItem(preId);
}

bool KDocumentGenerator::FindFrameOrCell(QTextFrame *frame, const std::string &id,
                                         QTextFrame **outFrame,
                                         QTextTableCell &outCell) const
{
    // Реф. @0x53ca28. Рекурсивный обход дерева фреймов; ElementId в property UserProperty+1.
    if (!frame)
        return false;

    // 1. Совпадает ли сам фрейм.
    if (frame->frameFormat().property(QTextFormat::UserProperty + 1).toString().toStdString()
        == id) {
        if (outFrame)
            *outFrame = frame;
        return true;
    }

    if (QTextTable *tbl = qobject_cast<QTextTable *>(frame)) {
        // 2. Таблица — обход ячеек.
        for (int r = 0; r < tbl->rows(); ++r) {
            for (int c = 0; c < tbl->columns(); ++c) {
                QTextTableCell cell = tbl->cellAt(r, c);
                if (!cell.isValid())
                    continue;
                if (cell.format().property(QTextFormat::UserProperty + 1)
                        .toString().toStdString() == id) {
                    outCell = cell;
                    return true;
                }
                // 2b. Рекурсия во вложенные фреймы ячейки.
                for (QTextFrame::iterator it = cell.begin(); !it.atEnd(); ++it) {
                    if (QTextFrame *child = it.currentFrame())
                        if (FindFrameOrCell(child, id, outFrame, outCell))
                            return true;
                }
            }
        }
    } else {
        // 3. Обычный фрейм — рекурсия по дочерним фреймам.
        for (QTextFrame::iterator it = frame->begin(); !it.atEnd(); ++it) {
            if (QTextFrame *child = it.currentFrame())
                if (FindFrameOrCell(child, id, outFrame, outCell))
                    return true;
        }
    }
    return false;
}

QTextFrame *KDocumentGenerator::GetSelectFrame() const
{
    // Реф. @0x53d1b0: поиск по m_strCurItemId от корня; отдаёт фрейм (не ячейку).
    if (!m_pDoc)
        return nullptr;
    QTextFrame *out = nullptr;
    QTextTableCell dummy;
    FindFrameOrCell(m_pDoc->rootFrame(), m_strCurItemId, &out, dummy);
    return out;
}

QTextTableCell KDocumentGenerator::GetSelectCell() const
{
    // Реф. @0x53d160: поиск по m_strCurItemId от корня; отдаёт ячейку (не фрейм).
    QTextTableCell result;
    if (!m_pDoc)
        return result;
    QTextFrame *dummy = nullptr;
    FindFrameOrCell(m_pDoc->rootFrame(), m_strCurItemId, &dummy, result);
    return result;
}

void KDocumentGenerator::InsertBlockLineAfterItem(const std::string & /*id*/)
{
    // Реф. @0x53dc90: ВЫРАВНИВАНИЕ ФУТЕРА ПО НИЗУ СТРАНИЦЫ. Находит фрейм/ячейку элемента id
    // (FindFrameOrCell), берёт lastCursorPosition и добивает документ ПУСТЫМИ абзацами
    // (QTextBlockFormat по умолчанию, без property), пока высота документа < editMaxHeight-9
    // (editMaxHeight из QTextDocumentLayout). Контент НЕ добавляет — только визуальный отступ.
    //
    // ОТСТУПЛЕНИЕ (помечено): реализовано как no-op. Требует ещё не восстановленных
    // FindFrameOrCell @0x53ca28 (Qt-поиск фрейма/ячейки по id) и метрик QTextDocumentLayout.
    // На КОРРЕКТНОСТЬ КОНТЕНТА и его рендер не влияет — только на прижатие футера к нижнему
    // краю. Полностью реализуется в UI-итерации вместе с FindFrameOrCell/Change*Selected.
}
