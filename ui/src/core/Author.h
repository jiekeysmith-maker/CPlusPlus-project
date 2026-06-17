#ifndef AUTHOR_H
#define AUTHOR_H

#include "CoreTypes.h"
#include "Serializable.h"

#include <string>
#include <vector>

class Author : public Serializable {
private:
    IdType m_id;
    std::string m_name;
    std::string m_gender;
    std::string m_affiliation;
    std::string m_email;
    std::vector<std::string> m_researchAreas;

public:
    Author() : m_id(INVALID_ID) {}
    Author(IdType id, const std::string& name, const std::string& gender,
        const std::string& affiliation, const std::string& email,
        const std::vector<std::string>& areas);

    IdType getId() const { return m_id; }
    void setId(IdType id) { m_id = id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    const std::string& getGender() const { return m_gender; }
    void setGender(const std::string& gender) { m_gender = gender; }
    const std::string& getAffiliation() const { return m_affiliation; }
    void setAffiliation(const std::string& aff) { m_affiliation = aff; }
    const std::string& getEmail() const { return m_email; }
    void setEmail(const std::string& email) { m_email = email; }
    const std::vector<std::string>& getResearchAreas() const { return m_researchAreas; }
    void setResearchAreas(const std::vector<std::string>& areas) { m_researchAreas = areas; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

#endif // AUTHOR_H
