#pragma once

#include <QString>

#include <map>
#include <string>
#include <vector>

// Персистентный слой конфигурации принтеров (реф. KSysPrintData, X-2600).
// Владелец — KPrinterManager (поле +0xC8), НЕ синглтон; часть методов KPrinterManager —
// тонкие форвардеры сюда. Весь класс OFF-DEVICE: в диапазоне реф. нет ни одного вызова
// CUPS/HAL — реальный ввод-вывод живёт в KCupsPrinter/KWindowsPrinter/KHalPrinterAPI.
//
// Два файла в system/printer/ (реф. хранит их АБСОЛЮТНЫМИ литералами в полях +0x40/+0x60:
// "/home/root/system/printer/KSysPrintService.xml" и ".../KSyPrintUrlDriver.xml"; у нас те же
// файлы адресуются через KSystem::SystemPath(), т.е. тот же путь на устройстве, но с ENDO_ROOT).
//
// ВАЖНО (сверено дизасмом): блок <Printer><Item name="DefaultImagePrinter">…</Item></Printer>
// в KSysPrintService.xml — МЁРТВЫЕ ЛЕГАСИ-ДАННЫЕ: литералов DefaultImagePrinter/
// DefaultPDFPrinter/DefaultVideoPrinter в бинарнике НЕТ ВООБЩЕ, ни один путь кода их не читает.
// Живая схема — только <Root><PrinterList><item …11 атрибутов…/></PrinterList></Root>.
// <Version> тоже нигде не читается. В обоих файлах элемент называется "item" (строчными).

// Тип подключения (реф. E_PRINTER_CONNECT_TYPE; значения доказаны по вызовам
// KAddPrinterDlg::AddWinPrinter/AddUsbPrinter/AddNetPrinter).
enum E_PRINTER_CONNECT_TYPE {
    PRINTER_CONNECT_WINDOWS = 0,   // TR_WPrinter (SMB)
    PRINTER_CONNECT_USB     = 1,   // TR_UPrinter
    PRINTER_CONNECT_NETWORK = 2,   // TR_NPrinter
};

// Тип сервиса (реф. E_PRINTER_SERVICE_TYPE). ДОКАЗАНО из RefreshCurrentPrinterNameInCache:
// type==1 → слот картинок, type==2 → слот отчётов, type==0 пропускается.
// Имена констант 1/2 — ПРЕДПОЛОЖЕНИЕ по поведению (точная привязка к TR_IPService/
// TR_RPService в реф. ServiceTypeToTR НЕ РАЗРЕШЕНА) — на семантику кода не влияет.
enum E_PRINTER_SERVICE_TYPE {
    PRINTER_SERVICE_NONE   = 0,
    PRINTER_SERVICE_IMAGE  = 1,
    PRINTER_SERVICE_REPORT = 2,
};

// Настройки печати (реф. KPrintSettings, sizeof 0x10). Побайтово совпадает с хвостом
// KPrintServiceInfo +0x4C..0x5C — реф. копирует их одной парой ldp/stp.
struct KPrintSettings {
    int  paper_size   = 0;   // +0x00
    int  gamma        = 0;   // +0x04
    int  brightness   = 0;   // +0x08
    bool optimization = false; // +0x0C
};

// Запись о принтере (реф. KPrintServiceInfo, sizeof 0x60). Каждое поле — один XML-атрибут
// элемента <item> (порядок и имена атрибутов сверены с append_attribute в AddPrinter).
struct KPrintServiceInfo {
    std::string name;              // +0x00  @name
    int         type = 0;          // +0x20  @type          (E_PRINTER_SERVICE_TYPE)
    int         connect_type = 0;  // +0x24  @connect_type   (E_PRINTER_CONNECT_TYPE)
    std::string device_or_ip;      // +0x28  @device_or_ip
    bool default_printer = false;  // +0x48  @default_printer
    bool image_printer  = false;   // +0x49  @image_printer
    bool pdf_printer    = false;   // +0x4A  @pdf_printer
    // +0x4C..0x5C — встроенный KPrintSettings:
    int  paper_size   = 0;         // +0x4C  @paper_size
    int  gamma        = 0;         // +0x50  @gamma
    int  brightness   = 0;         // +0x54  @brightness
    bool optimization = false;     // +0x58  @optimization
};

class KSysPrintData
{
public:
    KSysPrintData();

    // Загрузчик, НЕСМОТРЯ НА ИМЯ (реф.): читает /Root/PrinterList/item в кэш и вызывает
    // RefreshCurrentPrinterNameInCache. Файл не открылся → лог и выход, кэш остаётся пустым.
    void QueryAllPrinters();
    // /Root/Printer/item (@url,@driver) второго файла → карта url→driver.
    void GetAllUrlDriverInfo();

    // КВИРК РЕФ.: дубликаты НЕ проверяются — <item> добавляется всегда, даже если @name уже есть
    // (гейт IsPrinterExist — на вызывающем).
    void AddPrinter(const KPrintServiceInfo &info);
    // КВИРК РЕФ.: save выполняется БЕЗУСЛОВНО (даже если узел не найден); новый дефолт взамен
    // удалённого НЕ назначается.
    void DelPrinter(const std::string &name);
    // Пишет 4 атрибута настроек в XML и в кэш; save БЕЗУСЛОВЕН (даже если совпадений нет).
    void UpdatePrintSettings(const std::string &name, const KPrintSettings &st);

    // КВИРК РЕФ.: при неудачной ЗАГРУЗКЕ файла метод всё равно продолжает и СОХРАНЯЕТ —
    // это может затереть повреждённый файл. Воспроизводим 1:1.
    void SetDefaultPrinterInXml(const std::string &name, bool isDefault);
    void SetDefaultPrinterInCache(const std::string &name, bool isDefault);
    // Снимает флаг дефолта у ПЕРВОГО кэш-элемента с этим типом (реф. допускает <=1 дефолт
    // на тип), затем SetDefaultPrinterInXml(name,false). Опечатка имени — как в реф.
    void CancleDefaultStatusByType(int serviceType);
    // Переключает дефолт: new=!GetPrinterDefaultStatus(name); если принтера нет — выход;
    // при включении сперва снимает дефолт с того же типа.
    void ChangeDefaultPrinterStatus(const std::string &name);

    bool GetPrinterInfo(const std::string &name, KPrintServiceInfo &out) const;
    bool IsPrinterExist(const std::string &name) const;
    bool GetPrinterDefaultStatus(const std::string &name) const;
    // Реф. KPrinterManager::GetPrintSettings — GetPrinterInfo + копия хвоста +0x4C..0x5C.
    bool GetPrintSettings(const std::string &name, KPrintSettings &out) const;

    // Обновляет имена текущих принтеров (слоты картинок/отчётов) из кэша: обход С КОНЦА,
    // type==1 → слот картинок, type==2 → слот отчётов, элементы с флагом дефолта в приоритете.
    void RefreshCurrentPrinterNameInCache();

    // Обновление «на месте» либо добавление <item url= driver=/>; дубликаты не создаются.
    void SaveUrlDriverInfo(const std::string &url, const std::string &driver);
    // ВНИМАНИЕ: реф. возвращает путь ПО ЗНАЧЕНИЮ (sret), аргумент НЕ мутируется.
    // Только карта в памяти, файл не читается; промах → пустая строка.
    std::string FindDriverPath(const std::string &url) const;

    // --- доступ к кэшу (для тестов/вызывающих) ---
    const std::vector<KPrintServiceInfo> &Printers() const { return printers_; }
    std::string CurrentImagePrinter() const { return imagePrinter_; }
    std::string CurrentReportPrinter() const { return reportPrinter_; }

    // Пути (реф. — абсолютные литералы в полях; у нас через KSystem::SystemPath()).
    static QString ServiceXmlFile();
    static QString UrlDriverXmlFile();

private:
    std::string imagePrinter_;    // +0x00 — имя текущего принтера картинок
    std::string reportPrinter_;   // +0x20 — имя текущего принтера отчётов
    std::vector<KPrintServiceInfo> printers_;              // +0x80 — кэш принтеров
    std::map<std::string, std::string> urlDriver_;         // +0x98 — url → driver
};
