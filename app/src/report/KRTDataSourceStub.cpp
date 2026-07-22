#include "KRTDataSourceStub.h"

// Каждый Get*(id, out&): очистить/заполнить out из карты; true, если id известен (реф. семантика
// KRTDataSourceDemo: пишет out и возвращает true только для разрешимых id).

bool KRTDataSourceStub::GetTextData(const std::string &id, std::string &out)
{
    auto it = m_text.find(id);
    if (it == m_text.end()) return false;
    out = it->second;
    return true;
}

bool KRTDataSourceStub::GetTextGroupData(const std::string &id, std::vector<std::pair<std::string, std::string>> &out)
{
    auto it = m_textGroup.find(id);
    if (it == m_textGroup.end()) return false;
    out = it->second;
    return true;
}

bool KRTDataSourceStub::GetImageData(const std::string &id, std::string &out)
{
    auto it = m_image.find(id);
    if (it == m_image.end()) return false;
    out = it->second;
    return true;
}

bool KRTDataSourceStub::GetImageGroupData(const std::string &id, std::vector<std::pair<std::string, std::string>> &out)
{
    auto it = m_imageGroup.find(id);
    if (it == m_imageGroup.end()) return false;
    out = it->second;
    return true;
}

bool KRTDataSourceStub::GetSubData(const std::string &id, KReportTemplateDataNew &out)
{
    auto it = m_sub.find(id);
    if (it == m_sub.end()) return false;
    out = it->second;
    return true;
}

bool KRTDataSourceStub::GetMixData(const std::string &id, std::vector<std::string> &out)
{
    auto it = m_mix.find(id);
    if (it == m_mix.end()) return false;
    out = it->second;
    return true;
}
