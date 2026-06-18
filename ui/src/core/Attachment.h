#ifndef ATTACHMENT_H
#define ATTACHMENT_H

#include "CoreTypes.h"
#include "Serializable.h"

#include <string>

class Attachment : public Serializable {
private:
    IdType m_id;
    std::string m_name;
    IdType m_paperId;
    std::string m_content;
    std::string m_filePath;

public:
    Attachment() : m_id(INVALID_ID), m_paperId(INVALID_ID) {}
    Attachment(IdType id, const std::string& name, IdType paperId, const std::string& content);

    IdType getId() const { return m_id; }
    void setId(IdType id) { m_id = id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    IdType getPaperId() const { return m_paperId; }
    void setPaperId(IdType pid) { m_paperId = pid; }
    const std::string& getContent() const { return m_content; }
    void setContent(const std::string& content) { m_content = content; }
    const std::string& getFilePath() const { return m_filePath; }
    void setFilePath(const std::string& path) { m_filePath = path; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

#endif // ATTACHMENT_H
