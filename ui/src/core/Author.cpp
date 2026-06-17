#include "Author.h"

#include "SerializationUtils.h"

#include <sstream>

Author::Author(IdType id, const std::string& name, const std::string& gender,
    const std::string& affiliation, const std::string& email,
    const std::vector<std::string>& areas)
    : m_id(id), m_name(name), m_gender(gender),
    m_affiliation(affiliation), m_email(email), m_researchAreas(areas) {
}

std::string Author::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_name << FIELD_SEP << m_gender << FIELD_SEP
        << m_affiliation << FIELD_SEP << m_email << FIELD_SEP
        << join(m_researchAreas, ';');
    return ss.str();
}

void Author::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 6) {
        m_id = std::stoi(parts[0]);
        m_name = parts[1];
        m_gender = parts[2];
        m_affiliation = parts[3];
        m_email = parts[4];
        m_researchAreas = split(parts[5], ';');
    }
}
