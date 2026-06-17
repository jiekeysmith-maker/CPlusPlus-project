#ifndef CATALOG_H
#define CATALOG_H

#include "CoreTypes.h"
#include "Serializable.h"

#include <string>
#include <vector>

class Catalog : public Serializable {
private:
    IdType m_id;
    std::string m_name;
    int m_level;
    std::string m_description;
    IdType m_parentId;
    std::vector<IdType> m_childIds;
    std::vector<IdType> m_paperIds;

public:
    Catalog() : m_id(INVALID_ID), m_level(0), m_parentId(INVALID_ID) {}

    IdType getId() const { return m_id; }
    void setId(IdType id) { m_id = id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    int getLevel() const { return m_level; }
    void setLevel(int level) { m_level = level; }
    const std::string& getDescription() const { return m_description; }
    void setDescription(const std::string& desc) { m_description = desc; }
    IdType getParentId() const { return m_parentId; }
    void setParentId(IdType pid) { m_parentId = pid; }
    const std::vector<IdType>& getChildIds() const { return m_childIds; }
    void setChildIds(const std::vector<IdType>& ids) { m_childIds = ids; }
    void addChildId(IdType cid) { m_childIds.push_back(cid); }
    void removeChildId(IdType cid);
    const std::vector<IdType>& getPaperIds() const { return m_paperIds; }
    void setPaperIds(const std::vector<IdType>& ids) { m_paperIds = ids; }
    void addPaperId(IdType pid) { m_paperIds.push_back(pid); }
    void removePaperId(IdType pid);

    bool isRoot() const { return m_parentId == INVALID_ID; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

#endif // CATALOG_H
