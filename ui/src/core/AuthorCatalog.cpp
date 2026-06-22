#include "AuthorCatalog.h"

#include "SerializationUtils.h"

#include <algorithm>
#include <sstream>

void AuthorCatalog::addChildId(IdType cid) {
    if (std::find(m_childIds.begin(), m_childIds.end(), cid) == m_childIds.end())
        m_childIds.push_back(cid);
}

void AuthorCatalog::removeChildId(IdType cid) {
    auto it = std::find(m_childIds.begin(), m_childIds.end(), cid);
    if (it != m_childIds.end())
        m_childIds.erase(it);
}

void AuthorCatalog::addAuthorId(IdType aid) {
    if (std::find(m_authorIds.begin(), m_authorIds.end(), aid) == m_authorIds.end())
        m_authorIds.push_back(aid);
}

void AuthorCatalog::removeAuthorId(IdType aid) {
    auto it = std::remove(m_authorIds.begin(), m_authorIds.end(), aid);
    if (it != m_authorIds.end())
        m_authorIds.erase(it, m_authorIds.end());
}

std::string AuthorCatalog::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_name << FIELD_SEP << m_level << FIELD_SEP
        << m_description << FIELD_SEP << m_parentId << FIELD_SEP
        << joinIds(m_childIds, ';') << FIELD_SEP << joinIds(m_authorIds, ';');
    return ss.str();
}

void AuthorCatalog::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 7) {
        m_id = std::stoi(parts[0]);
        m_name = parts[1];
        m_level = std::stoi(parts[2]);
        m_description = parts[3];
        m_parentId = std::stoi(parts[4]);
        m_childIds = splitIds(parts[5], ';');
        m_authorIds = splitIds(parts[6], ';');
    }
}
