#pragma once

#include "KRTAbsDataSource.h"

#include <map>

// Канистра-источник данных для тестов/превью (реф. аналог KRTDataSourceDemo @0x589c70 — своя
// начинка в бинарнике). Реализует KRTAbsDataSource поверх in-memory карт: SetText/SetImage/SetSub/
// SetTextGroup наполняют, Get* возвращают из карт (true если ключ есть). Чисто STL, без Qt/device.
class KRTDataSourceStub : public KRTAbsDataSource
{
public:
    void SetText(const std::string &id, const std::string &value) { m_text[id] = value; }
    void SetImage(const std::string &id, const std::string &path) { m_image[id] = path; }
    void SetSub(const std::string &id, const KReportTemplateDataNew &data) { m_sub[id] = data; }
    void SetTextGroup(const std::string &id, const std::vector<std::pair<std::string, std::string>> &g) { m_textGroup[id] = g; }
    void SetImageGroup(const std::string &id, const std::vector<std::pair<std::string, std::string>> &g) { m_imageGroup[id] = g; }
    void SetMix(const std::string &id, const std::vector<std::string> &m) { m_mix[id] = m; }

    bool GetTextData(const std::string &id, std::string &out) override;
    bool GetTextGroupData(const std::string &id, std::vector<std::pair<std::string, std::string>> &out) override;
    bool GetImageData(const std::string &id, std::string &out) override;
    bool GetImageGroupData(const std::string &id, std::vector<std::pair<std::string, std::string>> &out) override;
    bool GetSubData(const std::string &id, KReportTemplateDataNew &out) override;
    bool GetMixData(const std::string &id, std::vector<std::string> &out) override;

private:
    std::map<std::string, std::string> m_text;
    std::map<std::string, std::string> m_image;
    std::map<std::string, KReportTemplateDataNew> m_sub;
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> m_textGroup;
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> m_imageGroup;
    std::map<std::string, std::vector<std::string>> m_mix;
};
