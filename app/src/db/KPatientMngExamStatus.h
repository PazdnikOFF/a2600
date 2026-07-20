#pragma once

#include <string>

#include "db/KExamEntry.h"

class QString;
class QStringList;

// Состояние осмотра при завершении (реф. KPatientMngExamStatus : KObject, X-2600).
// Синглтон (call_once + shared_ptr, наружу сырой указатель).
//
// ВАЖНО (реверс): НИКАКОГО ini/файла состояния НЕТ — «состоянием» служит сама
// строка осмотра в tb_ExamList. Обратного чтения (LoadExamState/GetExamState)
// в бинарнике не существует; читают через KExamListDBTableHandler::GetExamEntity
// и GetLatestExamIdFromDb.
//
// В реф. ctor подписывается на сообщения 12026 / 12013 / 12010 и зовёт
// ReUpdateLatestExamIdInfoToDb(); шина сообщений здесь пока не подключена.
class KPatientMngExamStatus
{
public:
    static KPatientMngExamStatus *GetInstance();

    // Реф.:
    //   LogPrintf("[APP][I]: ", "power off SaveExamState");
    //   id = KExamBussinessHandler::GetInstance()->m_strExamId;
    //   if (!id.empty()) { SaveState(id); SaveOverExamPatientInfo(info); }
    // Полей самого info НЕ читает.
    void SaveExamState(const MainUiPatientInfo &info);

    // Реф.: GetExamEntity(examId) → каталог = <usbPath> + RecordPath;
    // пересчёт числа снимков (*.jpg *.png *.bmp) и видео (*.avi *.mp4 *.mkv *.flv);
    // если изменилось — обновить RecordImgNum/RecordVideoNum; если ReportStatus
    // == "Eg" → выставить "--"; UpdateExamEntity + PostMsgToUI(0, 12040, 0, 0, {}).
    void SaveState(const std::string &examId);

    // Реф.: пополнение таблиц быстрого ввода (пациент/направивший) с меткой
    // "yyyy-MM-dd hh:mm:ss". Пока не подключено к KEntityQuickInput.
    void SaveOverExamPatientInfo(const MainUiPatientInfo &info);

    // Реф.: QDir(path).entryInfoList(filters, QDir::NoFilter, QDir::NoSort).size()
    static int GetFiletypeNumFromPath(const QString &path, const QStringList &filters);

private:
    KPatientMngExamStatus() = default;
};
