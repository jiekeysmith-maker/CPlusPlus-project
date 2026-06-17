#include "Journal.h"

#include "SerializationUtils.h"

#include <sstream>

std::string Journal::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_shortName << FIELD_SEP << m_fullName << FIELD_SEP
        << m_field << FIELD_SEP << m_publisher << FIELD_SEP << m_publisherAddress << FIELD_SEP
        << m_retrievalType << FIELD_SEP << m_remark << FIELD_SEP << m_latestImpactFactor;
    return ss.str();
}

void Journal::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 9) {
        m_id = std::stoi(parts[0]);
        m_shortName = parts[1];
        m_fullName = parts[2];
        m_field = parts[3];
        m_publisher = parts[4];
        m_publisherAddress = parts[5];
        m_retrievalType = parts[6];
        m_remark = parts[7];
        m_latestImpactFactor = std::stod(parts[8]);
    }
}
