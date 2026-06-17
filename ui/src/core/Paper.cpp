#include "Paper.h"

#include "SerializationUtils.h"

#include <sstream>

std::string Paper::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_code << FIELD_SEP << m_title << FIELD_SEP
        << join(m_keywords, ';') << FIELD_SEP << m_abstract << FIELD_SEP
        << m_publishDate << FIELD_SEP << m_sourceId << FIELD_SEP
        << m_issue << FIELD_SEP << m_issueNumber << FIELD_SEP
        << m_pageRange << FIELD_SEP << joinIds(m_authorIds, ';') << FIELD_SEP
        << joinIds(m_attachmentIds, ';') << FIELD_SEP << m_remark << FIELD_SEP
        << m_filePath;
    return ss.str();
}

void Paper::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 13) {
        m_id = std::stoi(parts[0]);
        m_code = parts[1];
        m_title = parts[2];
        m_keywords = split(parts[3], ';');
        m_abstract = parts[4];
        m_publishDate = parts[5];
        m_sourceId = std::stoi(parts[6]);
        m_issue = parts[7];
        m_issueNumber = parts[8];
        m_pageRange = parts[9];
        m_authorIds = splitIds(parts[10], ';');
        m_attachmentIds = splitIds(parts[11], ';');
        m_remark = parts[12];
        m_filePath = (parts.size() >= 14) ? parts[13] : "";
    }
}
