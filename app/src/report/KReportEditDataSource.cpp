#include "report/KReportEditDataSource.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "db/KPatientExamData.h"
#include "kernel/KSystemLog.h"
#include "report/KReportEntity.h"
#include "sys/KEnvConfig.h"

// --- константы реф. ---------------------------------------------------------

namespace exam_detail {
const char *const CONFIG_QSS_BASE_DIR = "mainapp/application/qss/";
}
namespace report_edit {
const char *const POS_NAME_DIR    = "patient/posname/";
const char *const REGION_DIR      = "mainapp/patient/region/";
const char *const EDITED_MARK_IMG = "mainapp/application/qss/patient/edited_mark.png";
}

namespace KScopeClass {
const char *DeviceName(E_CLASS cls)
{
    // Реф. вектор из 6 записей; E_DUODENOSCOPE(14) в реф. индексирует его
    // ЗА ГРАНИЦАМИ (см. заголовок) — здесь отображаем 14 → индекс 4.
    switch (cls) {
    case E_GASTROSCOPY:     return "DEV_GASTROSCOPY";
    case E_COLONOSCOPY:     return "DEV_COLONOSCOPY";
    case E_BRONCHOSCOPE:    return "DEV_BRONCHOSCOPE";
    case E_NOSELARYNXSCOPE: return "DEV_NOSELARYNXSCOPE";
    case E_DUODENOSCOPE:    return "DEV_DUODENOSCOPE";
    }
    return "DEV_UNKNOW";
}
} // namespace KScopeClass

namespace {

// Реф.: файл-статический сентинел, с которым сверяется LoadDbString.
const char *const kInvalidString = "INVALID_STRING";

// Реф. NVP-тег boost::serialization.
const char *const kOrganNvpTag = "obj_organ_name_list";

QString userOrganFilePath(KScopeClass::E_CLASS cls)
{
    return QString::fromStdString(KEnvConfig::GetInstance().GetUsrDir())
         + QLatin1String(report_edit::POS_NAME_DIR)
         + QLatin1String(KScopeClass::DeviceName(cls))
         + QStringLiteral(".xml");
}

} // namespace

// --- сериализация списка файлов ---------------------------------------------

QString KReportEditDataSource::ChangeFileListToString(QStringList list)
{
    if (list.isEmpty())
        return QString(QLatin1String(""));   // реф.: именно пустая строка
    QString s = QStringLiteral("$");
    for (const QString &e : list) {
        s += QLatin1Char('#');
        s += e;
    }
    return s;
}

QStringList KReportEditDataSource::ChangeStringToFileList(QString str)
{
    // Реф.: нужны ОБА условия.
    if (str.left(1) != QLatin1String("$") || str.length() <= 2)
        return QStringList();
    // mid(2, length()) отбрасывает "$#", поэтому ведущей пустой части не бывает;
    // KeepEmptyParts ⇒ хвостовой '#' даёт пустой элемент.
    return str.mid(2, str.length()).split(QLatin1Char('#'), QString::KeepEmptyParts);
}

// --- списки органов ---------------------------------------------------------

QStringList KReportEditDataSource::GetSysOrganNameList(KScopeClass::E_CLASS cls)
{
    // Реф.: зашитые списки ключей перевода; цепочка сравнений только на 1/2/3,
    // всё остальное (включая 0 и 14) даёт гастро-список.
    QStringList l;
    if (cls == KScopeClass::E_COLONOSCOPY) {
        l << "TR_AColon" << "TR_HFOColon" << "TR_TColon" << "TR_SFOColon"
          << "TR_DColon" << "TR_SColon" << "TR_Rctm";
    } else if (cls == KScopeClass::E_BRONCHOSCOPE) {
        l << "TR_ApSOLULobe" << "TR_ASOLULobe" << "TR_SLSOLULobe" << "TR_ILSOLULobe"
          << "TR_SSOLLLobe" << "TR_AISOLLLobe" << "TR_LBSOLLLobe" << "TR_PBSOLLLobe"
          << "TR_ApSORULobe" << "TR_ASORULobe" << "TR_PSORULobe" << "TR_MSORMLobe"
          << "TR_LSORMLobe" << "TR_ABSORLLobe" << "TR_SASORLLobe" << "TR_CBSORLLobe"
          << "TR_LBSORLLobe" << "TR_PBSORLLobe";
    } else if (cls == KScopeClass::E_NOSELARYNXSCOPE) {
        l << "TR_LNCavity" << "TR_RNCavity" << "TR_LNOperture" << "TR_RNOperture"
          << "TR_LSONasopharynx" << "TR_RSONasopharynx" << "TR_TJONAOropharynx"
          << "TR_OALaryngopharynx" << "TR_LPEpiglottica" << "TR_RPEpiglottica"
          << "TR_ROTongue" << "TR_EVallecula" << "TR_AL(I)" << "TR_AL(E)"
          << "TR_LPSinus" << "TR_RPSinus" << "TR_Lynx(SP)" << "TR_Lynx(GP)"
          << "TR_Lynx(SbP)" << "TR_HPalate" << "TR_SPalate" << "TR_LPTonsil"
          << "TR_RPTonsil" << "TR_LRTrigone" << "TR_RRTrigone";
    } else {
        l << "TR_Egs" << "TR_Cdia" << "TR_GBody" << "TR_GFundus" << "TR_GAngle"
          << "TR_GAntrum" << "TR_Atrm" << "TR_DBulb" << "TR_DDuodenum";
    }
    // Реф. прогоняет каждый ключ через QObject::tr; off-device словаря нет —
    // Qt в таком случае возвращает сам ключ.
    QStringList out;
    for (const QString &k : l)
        out << QObject::tr(k.toUtf8().constData());
    return out;
}

QStringList KReportEditDataSource::GetUserOrganNameList(KScopeClass::E_CLASS cls)
{
    const QString path = userOrganFilePath(cls);
    QFile f(path);
    QStringList list;
    if (f.open(QIODevice::ReadOnly)) {
        // Формат реф. — boost::serialization XML с NVP-тегом obj_organ_name_list.
        // Имя поля-вектора ВНУТРИ KOrganNameList из бинарника НЕ УСТАНОВЛЕНО,
        // поэтому читаем терпимо: любые элементы <item> внутри NVP-тега.
        QXmlStreamReader xml(&f);
        bool inList = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QLatin1String(kOrganNvpTag))
                    inList = true;
                else if (inList && xml.name() == QLatin1String("item"))
                    list << xml.readElementText();
            } else if (xml.isEndElement() && xml.name() == QLatin1String(kOrganNvpTag)) {
                inList = false;
            }
        }
        f.close();
    }
    // Реф.: нет файла / не читается / список пуст → системный список.
    // Слияния, сортировки и дедупликации НЕТ.
    if (list.isEmpty())
        return GetSysOrganNameList(cls);
    return list;
}

void KReportEditDataSource::SetUserOrganNameList(KScopeClass::E_CLASS cls,
                                                 const QStringList &list)
{
    const QString path = userOrganFilePath(cls);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        // Реф.: создать каталог и повторить попытку.
        const QString dir = QFileInfo(path).absoluteDir().absolutePath();
        if (!QDir().exists(dir)) {
            if (!QDir().mkpath(dir))
                LogPrintf("[APP][I]: ",
                          "KReportEditDataSource::SetUserOrganNameList Create Dir Faild:%s",
                          dir.toUtf8().constData());
            else
                LogPrintf("[APP][I]: ",
                          "KReportEditDataSource::SetUserOrganNameList Create Dir Success:%s",
                          dir.toUtf8().constData());
        }
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return;
    }
    // Полная перезапись (реф. — без чтения-слияния).
    QXmlStreamWriter xml(&f);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QLatin1String("boost_serialization"));
    xml.writeAttribute(QLatin1String("signature"), QLatin1String("serialization::archive"));
    xml.writeStartElement(QLatin1String(kOrganNvpTag));
    xml.writeTextElement(QLatin1String("count"), QString::number(list.size()));
    for (const QString &e : list)
        xml.writeTextElement(QLatin1String("item"), e);
    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndDocument();
    f.close();
}

// --- пути к изображениям ----------------------------------------------------

std::string KReportEditDataSource::GetRegionImgPath(KScopeClass::E_CLASS cls)
{
    const char *organ = nullptr;
    switch (cls) {
    case KScopeClass::E_GASTROSCOPY:     organ = "Stomach";    break;
    case KScopeClass::E_COLONOSCOPY:     organ = "Intestine";  break;
    case KScopeClass::E_BRONCHOSCOPE:    organ = "Bronchus";   break;
    case KScopeClass::E_NOSELARYNXSCOPE: organ = "Noselarynx"; break;
    case KScopeClass::E_DUODENOSCOPE:    organ = "Duodeno";    break;
    }
    if (!organ)
        return std::string();   // реф.: пусто, а НЕ базовый каталог
    return KEnvConfig::GetInstance().GetReadOnlyBaseDir()
         + std::string(report_edit::REGION_DIR) + organ + "/1.jpg";
}

std::string KReportEditDataSource::GetCursorImgPath(exam_detail::E_CURSOR_TYPE type)
{
    const char *rel = "";
    switch (type) {
    case exam_detail::E_ARROW_RIGHT_DOWN_CURSOR: rel = "icon/arrow/downright.png"; break;
    case exam_detail::E_ARROW_RIGHT_UP_CURSOR:   rel = "icon/arrow/upright.png";   break;
    case exam_detail::E_ARROW_LEFT_UP_CURSOR:    rel = "icon/arrow/upleft.png";    break;
    case exam_detail::E_ARROW_LEFT_DOWN_CURSOR:  rel = "icon/arrow/downleft.png";  break;
    case exam_detail::E_POINT_CURSOR:            rel = "icon/arrow/point.png";     break;
    case exam_detail::E_CURSOR_NONE:             rel = "";                         break;
    }
    // Реф.: для 0/неизвестного относительная часть пуста ⇒ вернётся ГОЛЫЙ
    // базовый каталог (в отличие от GetRegionImgPath).
    return KEnvConfig::GetInstance().GetReadOnlyBaseDir()
         + std::string(exam_detail::CONFIG_QSS_BASE_DIR) + rel;
}

// --- имена ------------------------------------------------------------------

QString KReportEditDataSource::GetReportTypeName(KScopeClass::E_CLASS cls)
{
    switch (cls) {
    case KScopeClass::E_GASTROSCOPY:     return QObject::tr("TR_VGReport");
    case KScopeClass::E_COLONOSCOPY:     return QObject::tr("TR_VCReport");
    case KScopeClass::E_BRONCHOSCOPE:    return QObject::tr("TR_VBReport");
    case KScopeClass::E_NOSELARYNXSCOPE: return QObject::tr("TR_VNReport");
    case KScopeClass::E_DUODENOSCOPE:    return QObject::tr("TR_VDReport");
    }
    return QString();
}

int KReportEditDataSource::GetSysDeviceType()
{
    return 0;   // реф.: `mov w0,#0; ret`, вызовов нет — мёртвый символ
}

// --- помощники чтения из БД -------------------------------------------------

std::string KReportEditDataSource::LoadDbString(const std::string &s)
{
    return (s == kInvalidString) ? std::string() : s;
}

int KReportEditDataSource::LoadDbInt(const std::string &s, int &out)
{
    try {
        const int v = std::stoi(s, nullptr, 10);
        out = v;              // реф.: out пишется ТОЛЬКО при успехе
        return 0;
    } catch (const std::invalid_argument &) {
        LogPrintf("[APP][I]: ", "std::stoi(%s) Invalid Argument: %s\n",
                  s.c_str(), "invalid_argument");
        return -2;
    } catch (const std::out_of_range &) {
        return -3;
    } catch (const std::exception &) {
        return -1;
    }
}

// --- таблица отчётов --------------------------------------------------------

int KReportEditDataSource::DeleteReportByExamId(const QString &examId)
{
    // Реф.: код возврата отдаётся как есть.
    return KReportDBTableHandler::DeleteEntity(examId.toStdString());
}

int KReportEditDataSource::QueryReportTable(const QString &examId)
{
    KReportEntity tmp;   // реф.: временная запись, результат выбрасывается
    return KReportDBTableHandler::GetEntity(examId.toStdString(), tmp);
}

int KReportEditDataSource::GetOneRecordFromReportTB(const std::string &examId,
                                                    std::vector<std::string> &outImgs,
                                                    std::string &outPath)
{
    KReportEntity e;
    if (KReportDBTableHandler::GetEntity(examId, e) != 0) {
        LogPrintf("[APP][I]: ", "KReportDBTableHandler::GetEntity(exam id: %s) failed",
                  examId.c_str());
        return -1;
    }
    std::vector<std::string> dirs;
    if (PatientExamData::GetExamDataPath(examId, dirs) != 0 || dirs.empty()) {
        LogPrintf("[APP][I]: ", "PatientExamData::GetExamDataPath(exam id: %s) failed",
                  examId.c_str());
        return -1;
    }
    outPath = dirs.front();

    const QStringList all = ChangeStringToFileList(
        QString::fromStdString(LoadDbString(e.ExamImg)));
    for (const QString &f : all) {
        const std::string full = f.toStdString();
        if (PatientExamData::IsFileExist(full))
            outImgs.push_back(full);
    }
    LogPrintf("[APP][I]: ", "all(num:%d) report images, get (num:%d) images",
              int(all.size()), int(outImgs.size()));
    return 1;   // реф.: 1 — успех
}
