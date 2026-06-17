#include "Catalog.h"

#include "SerializationUtils.h"

#include <algorithm>
#include <sstream>

void Catalog::removeChildId(IdType cid) {
    auto it = std::find(m_childIds.begin(), m_childIds.end(), cid);
    if (it != m_childIds.end())
        m_childIds.erase(it);
}

void Catalog::removePaperId(IdType pid) {
    auto it = std::find(m_paperIds.begin(), m_paperIds.end(), pid);
    if (it != m_paperIds.end())
        m_paperIds.erase(it);
}

std::string Catalog::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_name << FIELD_SEP << m_level << FIELD_SEP
        << m_description << FIELD_SEP << m_parentId << FIELD_SEP
        << joinIds(m_childIds, ';') << FIELD_SEP << joinIds(m_paperIds, ';');
    return ss.str();
}

void Catalog::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 7) {
        m_id = std::stoi(parts[0]);
        m_name = parts[1];
        m_level = std::stoi(parts[2]);
        m_description = parts[3];
        m_parentId = std::stoi(parts[4]);
        m_childIds = splitIds(parts[5], ';');
        m_paperIds = splitIds(parts[6], ';');
    }
}
