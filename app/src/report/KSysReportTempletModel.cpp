#include "report/KSysReportTempletModel.h"
#include "report/KSysReportTempletCfg.h"
#include "report/KReportTemplate.h"          // KReportTemplateManager
#include "report/KTemplateLibCfg.h"
#include "report/KReportTemplateCommonDef.h" // report_template::*

#include <QDate>
#include <QDebug>

#include <cassert>

// --- Сентинелы/константы реф. (значения runtime-init .bss; восстановлены из конфигов) ---
namespace {
// Model-сентинел «все департаменты» (реф. STR_REPORT_ALL_DEPT). ОТЛИЧАЕТСЯ от Control ("KW_ALL").
const std::string STR_REPORT_ALL_DEPT = "REPORT_DEPT_ALL";
// Fallback-дефолт (реф. report_preview::NP_nx3) — короткое имя каталога (TempletInfo.xml).
const std::string NP_nx3 = "NP-nx3";
// «Общий шаблон» (реф. STR_GENERAL_TEMPLATE @0xab4dc0, .rodata "General"), к которому
// цепляются user-define департаменты. В заводском TempletInfo.xml такого шаблона НЕТ —
// AddUserDefineDept вернёт false, пока пользователь не создаст "General" (реф.-поведение).
const std::string STR_GENERAL_TEMPLATE = "General";
} // namespace

KSysReportTempletModel::KSysReportTempletModel()
{
    Init();   // реф. ctor завершается вызовом Init()
}

KSysReportTempletModel::~KSysReportTempletModel() = default;

void KSysReportTempletModel::Init()
{
    ClearCfgCache();
    KSysReportTempletCfg::GetInstance().GetTempletInfos(m_vecTempletInfos);
}

void KSysReportTempletModel::ClearCfgCache()
{
    m_vecTempletInfos.clear();
    m_mapCfgCache.clear();
    m_delList.clear();
}

void KSysReportTempletModel::LoadDefault()
{
    KSysReportTempletCfg::GetInstance().LoadDefault();
    Init();
}

void KSysReportTempletModel::DiscardChange()
{
    KSysReportTempletCfg::GetInstance().LoadUserXML();
    Init();
}

void KSysReportTempletModel::Reload()
{
    KSysReportTempletCfg &cfg = KSysReportTempletCfg::GetInstance();
    cfg.Reload();
    QVector<KTempletBaseInfo> local;
    cfg.GetTempletInfos(local);
    // Сверка: имена, появившиеся в перечитанном каталоге, но отсутствующие в рабочей
    // копии, дописываются (in-memory правки прочих шаблонов сохраняются).
    for (const KTempletBaseInfo &info : local)
        if (!IsExist(info.TempletName().toStdString())) {
            qInfo("template(%s) is not existed, begin reload", info.TempletName().toUtf8().constData());
            m_vecTempletInfos.append(info);
        }
}

void KSysReportTempletModel::Save()
{
    KSysReportTempletCfg &cfg = KSysReportTempletCfg::GetInstance();
    cfg.SaveTempletInfos(m_vecTempletInfos);
    cfg.SaveTemplateCfg(m_mapCfgCache, m_delList);
    // Буфер новых либ-элементов: если непуст — влить в KTemplateLibCfg и сбросить.
    if (!m_tmpData.m_lstItems.empty()) {
        KReportTemplateManager *mgr = KReportTemplateManager::GetInstance();
        if (mgr && mgr->GetTemplateLibCfg())
            mgr->GetTemplateLibCfg()->UpdateTemplateLib(m_tmpData);
        m_tmpData = KReportTemplateDataNew();   // реф. сбрасывает 3 члена в пустые
    }
}

bool KSysReportTempletModel::IsExist(const std::string &name) const
{
    const QString qn = QString::fromStdString(name);
    for (const KTempletBaseInfo &info : m_vecTempletInfos)
        if (info.TempletName() == qn)
            return true;
    return false;
}

bool KSysReportTempletModel::GetTempletInfoByName(const std::string &name,
                                                  KTempletBaseInfo &out) const
{
    const QString qn = QString::fromStdString(name);
    for (const KTempletBaseInfo &info : m_vecTempletInfos)
        if (info.TempletName() == qn) {
            out = info;
            return true;
        }
    return false;
}

void KSysReportTempletModel::GetTempletInfosByDept(const std::string &dept,
                                                   QVector<KTempletBaseInfo> &out) const
{
    out.clear();
    const QString qd = QString::fromStdString(dept);
    const bool all = (dept == STR_REPORT_ALL_DEPT);   // сентинел → весь каталог
    for (const KTempletBaseInfo &info : m_vecTempletInfos)
        if (all || info.HasDept(qd))
            out.append(info);
}

bool KSysReportTempletModel::GetTemplateCfgByName(const std::string &name,
                                                  KReportTemplateDataNew &out)
{
    auto it = m_mapCfgCache.find(name);
    if (it != m_mapCfgCache.end()) {         // кэш-хит: отдать копию
        out = it->second;
        return true;
    }
    // Промах: делегировать файловому каталогу и закэшировать результат.
    KSysReportTempletCfg::GetInstance().GetTemplateByName(name, out);
    m_mapCfgCache[name] = out;
    return true;
}

std::string KSysReportTempletModel::GetDefalutTempletNameByDept(const std::string &dept)
{
    // QUIRK (реф.): sret инициализируется КОПИЕЙ dept и НЕ переписывается — метод
    // фактически возвращает свой аргумент. Вычисленное имя (localName) отбрасывается.
    const std::string ret = dept;
    const QString qd = QString::fromStdString(dept);
    std::string localName;               // "" — маркер «дефолт не найден»
    for (const KTempletBaseInfo &info : m_vecTempletInfos)
        if (info.IsDefault(qd)) {
            localName = info.TempletName().toStdString();
            qInfo("dept is %s, default templet is %s", dept.c_str(), localName.c_str());
        }
    // Побочный эффект: дефолта нет, но каталог непуст → назначить NP-nx3 дефолтом департамента.
    if (localName.empty() && !m_vecTempletInfos.isEmpty()) {
        localName = NP_nx3;
        SetDefault(NP_nx3, dept, true);
    }
    return ret;                          // НЕ localName (QUIRK)
}

void KSysReportTempletModel::SetDefault(const std::string &templetName,
                                        const std::string &dept, bool isDefault)
{
    if (m_vecTempletInfos.isEmpty())
        return;
    const QString qd = QString::fromStdString(dept);
    // Цикл 1: снять default департамента у ВСЕХ шаблонов (безусловно, даже при isDefault==false).
    for (KTempletBaseInfo &info : m_vecTempletInfos)
        if (info.IsDefault(qd))
            info.SetDefault(qd, false);
    // Цикл 2: установить флаг у шаблона с искомым именем.
    const QString qn = QString::fromStdString(templetName);
    for (KTempletBaseInfo &info : m_vecTempletInfos)
        if (info.TempletName() == qn) {
            info.SetDefault(qd, isDefault);
            return;
        }
}

bool KSysReportTempletModel::AddUserDefineDept(const std::string &dept)
{
    if (m_vecTempletInfos.isEmpty())
        return false;
    const QString qgeneral = QString::fromStdString(STR_GENERAL_TEMPLATE);   // "General"
    for (KTempletBaseInfo &info : m_vecTempletInfos)
        if (info.TempletName() == qgeneral) {           // только шаблон «General»
            info.AddDept(QString::fromStdString(dept), true);
            info.SetModifyDate(QDate::currentDate().toString("yyyy-MM-dd"));
            return true;                                // после первого совпадения
        }
    return false;
}

void KSysReportTempletModel::DeleteTemplate(const std::string &name)
{
    const QString qn = QString::fromStdString(name);
    for (int i = 0; i < m_vecTempletInfos.size(); ++i) {
        if (m_vecTempletInfos[i].TempletName() != qn)
            continue;
        auto it = m_mapCfgCache.find(name);
        if (it != m_mapCfgCache.end()) {
            m_vecTempletInfos.remove(i);
            m_mapCfgCache.erase(it);
            m_delList.push_back(name);
            qInfo("delete templet cfg(%s) success!!!", name.c_str());
            return;
        }
        // QUIRK (реф.): есть в info-каталоге, но нет в cfg-кэше — лог и ПРОДОЛЖИТЬ обход.
        qInfo("%s is in info lib but not in cfg lib", name.c_str());
    }
}

void KSysReportTempletModel::RenameTemplate(const std::string &from, const std::string &to)
{
    const QString qfrom = QString::fromStdString(from);
    for (KTempletBaseInfo &info : m_vecTempletInfos) {
        if (info.TempletName() != qfrom)
            continue;
        auto it = m_mapCfgCache.find(from);
        if (it == m_mapCfgCache.end()) {
            // Не в cfg-кэше → переименование НЕ выполняется (даже SetTempletName пропускается).
            qInfo("%s is in info lib but not in cfg lib", from.c_str());
            continue;
        }
        info.SetTempletName(QString::fromStdString(to));       // инфо-запись → новое имя
        KReportTemplateDataNew data = it->second;              // глубокая копия (list копируется глубоко)
        m_mapCfgCache.erase(it);                               // снять старый ключ
        m_delList.push_back(from);                             // старое имя — в список удалённых
        m_mapCfgCache[to] = data;                             // вставить под новым ключом
        qInfo("rename templet(%s) success!!!", to.c_str());
    }
}

void KSysReportTempletModel::SaveSingleTemplatetCfg(const KTempletBaseInfo &info,
                                                    const KReportTemplateDataNew &data)
{
    const QString qn = info.TempletName();
    // Фаза 1: m_vecTempletInfos — обновить на месте по имени либо дописать.
    bool found = false;
    for (KTempletBaseInfo &t : m_vecTempletInfos)
        if (t.TempletName() == qn) {
            qInfo("Save SingleTemplatetCfg::update existed cfg (%s)", qn.toUtf8().constData());
            t = info;   // перезапись имени/даты/factory/depts
            found = true;
            break;
        }
    if (!found) {
        qInfo("Save SingleTemplatetCfg::new cfg(%s) inserted", qn.toUtf8().constData());
        m_vecTempletInfos.append(info);
    }
    // Фаза 2: m_mapCfgCache[имя] = data (перезапись значения либо вставка).
    m_mapCfgCache[info.TempletName().toStdString()] = data;
}

bool KSysReportTempletModel::DeleteUserDefineDept(const std::string &dept)
{
    if (m_vecTempletInfos.isEmpty())
        return false;
    const QString qd = QString::fromStdString(dept);
    for (KTempletBaseInfo &info : m_vecTempletInfos) {
        if (!info.HasDept(qd))
            continue;
        // Обрабатывается ТОЛЬКО ПЕРВЫЙ шаблон с этим департаментом → return true (реф.).
        info.DeleteDept(qd);
        info.SetModifyDate(QDate::currentDate().toString("yyyy-MM-dd"));

        // (a) удалить под-элементы департамента из cfg этого шаблона.
        const std::string name = info.TempletName().toStdString();
        KReportTemplateDataNew local;
        GetTemplateCfgByName(name, local);
        report_template::RemoveSubItem(local, "/RT_MEASURE", dept);
        m_mapCfgCache[name] = local;

        // (b) буфер либ-данных: лениво засеять из KTemplateLibCfg, затем удалить департамент.
        if (m_tmpData.m_lstItems.empty()) {
            KReportTemplateManager *mgr = KReportTemplateManager::GetInstance();
            KTemplateLibCfg *lib = mgr ? mgr->GetTemplateLibCfg() : nullptr;
            assert(lib != nullptr);   // реф. __assert_fail "lib_cfg != nullptr" (line 367)
            if (lib)
                m_tmpData = lib->Data();
        }
        report_template::RemoveSubItem(m_tmpData, "/RT_MEASURE", dept);
        return true;
    }
    return false;
}

bool KSysReportTempletModel::UpdateUserDefineItem(const KReportTemplateDataNew &data)
{
    // Реф.: ВСЕГДА возвращает true (в т.ч. быстрый выход при пустом каталоге); ассертов нет.
    // Проходит по НЕ-заводским шаблонам; заводские пропускаются (ветка сравнения ModifyDate с
    // REPORT_RW_BACKUP в реф. есть, но её булев результат не используется → фактически continue).
    // Затрагивает ТОЛЬКО m_mapCfgCache (не m_tmpData/m_delList).
    for (KTempletBaseInfo &info : m_vecTempletInfos) {
        if (info.IsFactory())
            continue;
        const std::string name = info.TempletName().toStdString();
        KReportTemplateDataNew local;
        GetTemplateCfgByName(name, local);
        // Список путей измерительных пакетов — из item-config "CalcApp" (CSV через ",").
        std::string base;
        auto cit = local.m_mapItemConfigs.find("CalcApp");
        if (cit != local.m_mapItemConfigs.end())
            base = cit->second.m_strName;   // значение конфига (реф. читает строку узла)
        const std::vector<std::string> paths = report_template::RevertPathByID(base, ",");
        for (const std::string &p : paths) {
            // РЕКОНСТРУКЦИЯ id (append-цепочка из report_template-констант "RT_MEASURE"/"/"/
            // "_OTHER" + токен p; точная расстановка разделителей восстановлена по дизасму).
            const std::string itemId = std::string("RT_MEASURE") + "/" + p + "_OTHER";
            KReportTemplateItem *item = report_template::FindRefItem(local, itemId);
            if (!item)
                continue;
            std::vector<KReportTemplateItem *> delVec;
            GetDelUserDefineItem(item, delVec, data);
            for (KReportTemplateItem *del : delVec) {
                if (!del)
                    continue;
                std::string parentId;
                if (report_template::GetParentItemID(del->m_strID, parentId))
                    report_template::RemoveSubItem(local, parentId, del->m_strName);
            }
        }
        m_mapCfgCache[name] = local;   // запись обновлённого cfg обратно в кэш
    }
    return true;
}

bool KSysReportTempletModel::GetDelUserDefineItem(KReportTemplateItem *item,
                                                  std::vector<KReportTemplateItem *> &out,
                                                  const KReportTemplateDataNew &data)
{
    if (!item)
        return false;
    // Найден среди эталонных (const-ref) элементов → НЕ user-define; иначе — кандидат.
    const KReportTemplateItem *ref = report_template::FindConstRefItem(data, item->m_strID);
    if (!ref) {
        qInfo("The user define item(%s) is to deleted", item->m_strID.c_str());
        out.push_back(item);
        return true;
    }
    // Эталонный: удаляется, только если есть дети И ВСЕ они помечены на удаление.
    int deletedCount = 0;
    for (KReportTemplateItem &child : item->m_lstSubItems)
        if (GetDelUserDefineItem(&child, out, data))
            ++deletedCount;
    if (deletedCount == 0)
        return false;
    if (deletedCount == static_cast<int>(item->m_lstSubItems.size())) {
        qInfo("all child is deleted, The user define item(%s) is to deleted",
              item->m_strID.c_str());
        out.push_back(item);
        return true;
    }
    return false;
}
