#include "Attachment.h"

#include "SerializationUtils.h"

#include <sstream>

Attachment::Attachment(IdType id, const std::string& name, IdType paperId,
    const std::string& content)
    : m_id(id), m_name(name), m_paperId(paperId), m_content(content) {
}

std::string Attachment::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_name << FIELD_SEP << m_paperId << FIELD_SEP << m_content;
    return ss.str();
}

void Attachment::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 4) {
        m_id = std::stoi(parts[0]);
        m_name = parts[1];
        m_paperId = std::stoi(parts[2]);
        m_content = parts[3];
    }
}
