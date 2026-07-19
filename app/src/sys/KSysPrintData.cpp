#include "sys/KSysPrintData.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

// Реф. использует вендоренный pugixml (load_file / save_file(path, "\t", …)); off-device мы
// на QDomDocument (как KMeaXMLBase/XmlParser). Отступ сохраняем табом — как в реф. save_file.
namespace {

const char *const kElemItem   = "item";          // ЕДИНСТВЕННОЕ написание в обоих файлах
const char *const kElemRoot   = "Root";
const char *const kElemPList  = "PrinterList";   // KSysPrintService.xml
const char *const kElemPrn    = "Printer";       // KSyPrintUrlDriver.xml

bool loadDoc(const QString &file, QDomDocument &doc)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const bool ok = doc.setContent(&f);
    f.close();
    return ok;
}

bool saveDoc(const QString &file, const QDomDocument &doc)
{
    QDir().mkpath(QFileInfo(file).absolutePath());
    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    doc.save(ts, 1);   // реф. save_file(..., "\t", …) — отступ в один шаг
    f.close();
    return true;
}

// <Root> с гарантированным дочерним <child> (создаёт документ/узлы при отсутствии).
QDomElement ensureRootChild(QDomDocument &doc, const char *child)
{
    QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String(kElemRoot)) {
        doc.clear();
        root = doc.createElement(kElemRoot);
        doc.appendChild(root);
    }
    QDomElement c = root.firstChildElement(child);
    if (c.isNull()) {
        c = doc.createElement(child);
        root.appendChild(c);
    }
    return c;
}

void infoToElement(QDomElement &e, const KPrintServiceInfo &i)
{
    // Порядок атрибутов — как последовательность append_attribute в реф. AddPrinter.
    e.setAttribute("name", QString::fromStdString(i.name));
    e.setAttribute("type", i.type);
    e.setAttribute("connect_type", i.connect_type);
    e.setAttribute("device_or_ip", QString::fromStdString(i.device_or_ip));
    e.setAttribute("default_printer", i.default_printer ? "true" : "false");
    e.setAttribute("image_printer", i.image_printer ? "true" : "false");
    e.setAttribute("pdf_printer", i.pdf_printer ? "true" : "false");
    e.setAttribute("paper_size", i.paper_size);
    e.setAttribute("gamma", i.gamma);
    e.setAttribute("brightness", i.brightness);
    e.setAttribute("optimization", i.optimization ? "true" : "false");
}

// pugi::as_bool: "true"/"1"/"yes"/"on" (регистр не важен) → true; иначе false.
bool asBool(const QString &v)
{
    const QString s = v.trimmed().toLower();
    return s == "true" || s == "1" || s == "yes" || s == "on";
}

KPrintServiceInfo elementToInfo(const QDomElement &e)
{
    KPrintServiceInfo i;                                   // реф. as_string("")/as_int(0)/as_bool(false)
    i.name         = e.attribute("name").toStdString();
    i.type         = e.attribute("type").toInt();
    i.connect_type = e.attribute("connect_type").toInt();
    i.device_or_ip = e.attribute("device_or_ip").toStdString();
    i.default_printer = asBool(e.attribute("default_printer"));
    i.image_printer   = asBool(e.attribute("image_printer"));
    i.pdf_printer     = asBool(e.attribute("pdf_printer"));
    i.paper_size   = e.attribute("paper_size").toInt();
    i.gamma        = e.attribute("gamma").toInt();
    i.brightness   = e.attribute("brightness").toInt();
    i.optimization = asBool(e.attribute("optimization"));
    return i;
}

} // namespace

KSysPrintData::KSysPrintData() = default;

QString KSysPrintData::ServiceXmlFile()
{
    // Реф.: литерал "/home/root/system/printer/KSysPrintService.xml" (поле +0x40).
    return QDir(KSystem::SystemPath()).absoluteFilePath("printer/KSysPrintService.xml");
}

QString KSysPrintData::UrlDriverXmlFile()
{
    // Реф.: литерал "/home/root/system/printer/KSyPrintUrlDriver.xml" (поле +0x60).
    return QDir(KSystem::SystemPath()).absoluteFilePath("printer/KSyPrintUrlDriver.xml");
}

void KSysPrintData::QueryAllPrinters()
{
    // Реф.: имя вводит в заблуждение — это ЗАГРУЗЧИК кэша из XML, а не опрос CUPS.
    printers_.clear();
    QDomDocument doc;
    if (!loadDoc(ServiceXmlFile(), doc)) {
        qWarning("KSysPrintData: не удалось загрузить %s",
                 ServiceXmlFile().toUtf8().constData());
        return;                       // кэш остаётся пустым (реф.)
    }
    const QDomElement root = doc.documentElement();
    const QDomElement list = root.firstChildElement(kElemPList);   // /Root/PrinterList/item
    for (QDomElement e = list.firstChildElement(kElemItem); !e.isNull();
         e = e.nextSiblingElement(kElemItem))
        printers_.push_back(elementToInfo(e));
    RefreshCurrentPrinterNameInCache();
}

void KSysPrintData::GetAllUrlDriverInfo()
{
    urlDriver_.clear();
    QDomDocument doc;
    if (!loadDoc(UrlDriverXmlFile(), doc))
        return;
    const QDomElement root = doc.documentElement();
    const QDomElement prn = root.firstChildElement(kElemPrn);      // /Root/Printer/item
    for (QDomElement e = prn.firstChildElement(kElemItem); !e.isNull();
         e = e.nextSiblingElement(kElemItem))
        urlDriver_.emplace(e.attribute("url").toStdString(),
                           e.attribute("driver").toStdString());
}

void KSysPrintData::AddPrinter(const KPrintServiceInfo &info)
{
    QDomDocument doc;
    loadDoc(ServiceXmlFile(), doc);
    QDomElement list = ensureRootChild(doc, kElemPList);
    // КВИРК РЕФ.: проверки дубликата НЕТ — добавляем всегда.
    QDomElement e = doc.createElement(kElemItem);
    infoToElement(e, info);
    list.appendChild(e);
    saveDoc(ServiceXmlFile(), doc);
    printers_.push_back(info);
    RefreshCurrentPrinterNameInCache();
}

void KSysPrintData::DelPrinter(const std::string &name)
{
    QDomDocument doc;
    loadDoc(ServiceXmlFile(), doc);
    QDomElement list = ensureRootChild(doc, kElemPList);
    const QString qn = QString::fromStdString(name);
    for (QDomElement e = list.firstChildElement(kElemItem); !e.isNull();) {
        QDomElement next = e.nextSiblingElement(kElemItem);
        if (e.attribute("name") == qn)
            list.removeChild(e);
        e = next;
    }
    saveDoc(ServiceXmlFile(), doc);   // КВИРК: save БЕЗУСЛОВЕН (даже если ничего не удалили)
    for (auto it = printers_.begin(); it != printers_.end();)
        it = (it->name == name) ? printers_.erase(it) : it + 1;
    // КВИРК: новый дефолт взамен удалённого НЕ назначается.
    RefreshCurrentPrinterNameInCache();
}

void KSysPrintData::UpdatePrintSettings(const std::string &name, const KPrintSettings &st)
{
    QDomDocument doc;
    loadDoc(ServiceXmlFile(), doc);
    QDomElement list = ensureRootChild(doc, kElemPList);
    const QString qn = QString::fromStdString(name);
    for (QDomElement e = list.firstChildElement(kElemItem); !e.isNull();
         e = e.nextSiblingElement(kElemItem)) {
        if (e.attribute("name") != qn)
            continue;
        e.setAttribute("paper_size", st.paper_size);
        e.setAttribute("gamma", st.gamma);
        e.setAttribute("brightness", st.brightness);
        e.setAttribute("optimization", st.optimization ? "true" : "false");
    }
    saveDoc(ServiceXmlFile(), doc);   // КВИРК: save даже без совпадений
    for (KPrintServiceInfo &i : printers_)
        if (i.name == name) {         // те же 4 поля в кэше (+0x4C..0x5C)
            i.paper_size = st.paper_size;
            i.gamma = st.gamma;
            i.brightness = st.brightness;
            i.optimization = st.optimization;
        }
}

void KSysPrintData::SetDefaultPrinterInXml(const std::string &name, bool isDefault)
{
    QDomDocument doc;
    // КВИРК РЕФ.: даже при неудачной загрузке метод ПРОДОЛЖАЕТ и сохраняет (может затереть файл).
    loadDoc(ServiceXmlFile(), doc);
    QDomElement list = ensureRootChild(doc, kElemPList);
    const QString qn = QString::fromStdString(name);
    for (QDomElement e = list.firstChildElement(kElemItem); !e.isNull();
         e = e.nextSiblingElement(kElemItem))
        if (e.attribute("name") == qn)
            e.setAttribute("default_printer", isDefault ? "true" : "false");
    saveDoc(ServiceXmlFile(), doc);
}

void KSysPrintData::SetDefaultPrinterInCache(const std::string &name, bool isDefault)
{
    for (KPrintServiceInfo &i : printers_)
        if (i.name == name) {
            i.default_printer = isDefault;
            return;                    // молча ничего не делает, если принтера нет
        }
}

void KSysPrintData::CancleDefaultStatusByType(int serviceType)
{
    // Реф.: ПЕРВЫЙ элемент кэша с установленным дефолтом и совпавшим типом.
    for (KPrintServiceInfo &i : printers_) {
        if (!i.default_printer || i.type != serviceType)
            continue;
        i.default_printer = false;
        SetDefaultPrinterInXml(i.name, false);
        return;
    }
}

void KSysPrintData::ChangeDefaultPrinterStatus(const std::string &name)
{
    const bool next = !GetPrinterDefaultStatus(name);
    KPrintServiceInfo info;
    if (!GetPrinterInfo(name, info))
        return;                        // принтера нет — выход
    if (next)
        CancleDefaultStatusByType(info.type);   // снять дефолт с того же типа
    SetDefaultPrinterInXml(name, next);
    SetDefaultPrinterInCache(name, next);
    RefreshCurrentPrinterNameInCache();
}

bool KSysPrintData::GetPrinterInfo(const std::string &name, KPrintServiceInfo &out) const
{
    for (const KPrintServiceInfo &i : printers_)
        if (i.name == name) {
            out = i;
            return true;
        }
    return false;
}

bool KSysPrintData::IsPrinterExist(const std::string &name) const
{
    KPrintServiceInfo tmp;
    return GetPrinterInfo(name, tmp);
}

bool KSysPrintData::GetPrinterDefaultStatus(const std::string &name) const
{
    for (const KPrintServiceInfo &i : printers_)
        if (i.name == name)
            return i.default_printer;
    return false;                      // пустой кэш / не найден
}

bool KSysPrintData::GetPrintSettings(const std::string &name, KPrintSettings &out) const
{
    KPrintServiceInfo i;
    if (!GetPrinterInfo(name, i))
        return false;
    out.paper_size   = i.paper_size;   // копия хвоста +0x4C..0x5C
    out.gamma        = i.gamma;
    out.brightness   = i.brightness;
    out.optimization = i.optimization;
    return true;
}

void KSysPrintData::RefreshCurrentPrinterNameInCache()
{
    imagePrinter_.clear();
    reportPrinter_.clear();
    // Реф.: обход С КОНЦА; элементы с флагом дефолта имеют приоритет над обычными.
    for (auto it = printers_.rbegin(); it != printers_.rend(); ++it) {
        if (it->type == PRINTER_SERVICE_IMAGE) {
            if (imagePrinter_.empty() || it->default_printer)
                imagePrinter_ = it->name;
        } else if (it->type == PRINTER_SERVICE_REPORT) {
            if (reportPrinter_.empty() || it->default_printer)
                reportPrinter_ = it->name;
        }
        // type == 0 пропускается (реф.)
    }
}

void KSysPrintData::SaveUrlDriverInfo(const std::string &url, const std::string &driver)
{
    QDomDocument doc;
    loadDoc(UrlDriverXmlFile(), doc);
    QDomElement prn = ensureRootChild(doc, kElemPrn);
    const QString qurl = QString::fromStdString(url);
    bool found = false;
    for (QDomElement e = prn.firstChildElement(kElemItem); !e.isNull();
         e = e.nextSiblingElement(kElemItem))
        if (e.attribute("url") == qurl) {          // обновление НА МЕСТЕ
            e.setAttribute("driver", QString::fromStdString(driver));
            found = true;
        }
    if (!found) {                                   // дубликаты не создаются (реф.)
        QDomElement e = doc.createElement(kElemItem);
        e.setAttribute("url", qurl);
        e.setAttribute("driver", QString::fromStdString(driver));
        prn.appendChild(e);
    }
    saveDoc(UrlDriverXmlFile(), doc);
    urlDriver_[url] = driver;
}

std::string KSysPrintData::FindDriverPath(const std::string &url) const
{
    // Реф.: возврат ПО ЗНАЧЕНИЮ (sret); аргумент не мутируется; только карта, без файла.
    const auto it = urlDriver_.find(url);
    return it == urlDriver_.end() ? std::string() : it->second;
}
