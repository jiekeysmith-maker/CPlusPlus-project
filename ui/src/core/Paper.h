#ifndef PAPER_H
#define PAPER_H

#include "CoreTypes.h"
#include "Serializable.h"

#include <string>
#include <vector>

class Paper : public Serializable {
private:
    IdType m_id;
    std::string m_code;
    std::string m_title;
    std::vector<std::string> m_keywords;
    std::string m_abstract;
    std::string m_publishDate;
    IdType m_sourceId;
    std::string m_issue;
    std::string m_issueNumber;
    std::string m_pageRange;
    std::vector<IdType> m_authorIds;
    std::vector<IdType> m_attachmentIds;
    std::string m_filePath;
    std::string m_uploadTime;
    std::string m_remark;

public:
    Paper() : m_id(INVALID_ID), m_sourceId(INVALID_ID) {}

    IdType getId() const { return m_id; }
    void setId(IdType id) { m_id = id; }
    const std::string& getCode() const { return m_code; }
    void setCode(const std::string& code) { m_code = code; }
    const std::string& getTitle() const { return m_title; }
    void setTitle(const std::string& title) { m_title = title; }
    const std::vector<std::string>& getKeywords() const { return m_keywords; }
    void setKeywords(const std::vector<std::string>& kw) { m_keywords = kw; }
    const std::string& getAbstract() const { return m_abstract; }
    void setAbstract(const std::string& abst) { m_abstract = abst; }
    const std::string& getPublishDate() const { return m_publishDate; }
    void setPublishDate(const std::string& date) { m_publishDate = date; }
    IdType getSourceId() const { return m_sourceId; }
    void setSourceId(IdType sid) { m_sourceId = sid; }
    const std::string& getIssue() const { return m_issue; }
    void setIssue(const std::string& issue) { m_issue = issue; }
    const std::string& getIssueNumber() const { return m_issueNumber; }
    void setIssueNumber(const std::string& num) { m_issueNumber = num; }
    const std::string& getPageRange() const { return m_pageRange; }
    void setPageRange(const std::string& range) { m_pageRange = range; }
    const std::vector<IdType>& getAuthorIds() const { return m_authorIds; }
    void setAuthorIds(const std::vector<IdType>& ids) { m_authorIds = ids; }
    void addAuthorId(IdType aid) { m_authorIds.push_back(aid); }
    const std::vector<IdType>& getAttachmentIds() const { return m_attachmentIds; }
    void setAttachmentIds(const std::vector<IdType>& ids) { m_attachmentIds = ids; }
    void addAttachmentId(IdType attId) { m_attachmentIds.push_back(attId); }
    const std::string& getFilePath() const { return m_filePath; }
    void setFilePath(const std::string& path) { m_filePath = path; }
    const std::string& getUploadTime() const { return m_uploadTime; }
    void setUploadTime(const std::string& time) { m_uploadTime = time; }
    const std::string& getRemark() const { return m_remark; }
    void setRemark(const std::string& remark) { m_remark = remark; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

#endif // PAPER_H
