#ifndef MANAGE_H
#define MANAGE_H
//核心类定义


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

// 前向声明（用于解决循环依赖）
class LibraryManager;

// ========================= 辅助类型 =========================
using IdType = int;
constexpr IdType INVALID_ID = -1;

// 辅助解析函数：将 "1;2;3" 解析为 ID 列表
std::vector<IdType> splitIds(const std::string& s, char delim = ';');


// ========================= 实体基类 (统一序列化接口) =========================
class Serializable {
public:
    virtual ~Serializable() = default;
    // 序列化与反序列化 (此处使用字符串代表 JSON 或自定义格式)
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& json) = 0;
};

// ========================= 1. 作者实体 =========================
class Author : public Serializable {
private:
    IdType m_id;
    std::string m_name;  //姓名
    std::string m_gender;  //性别
    std::string m_affiliation;  // 单位
    std::string m_email;  //邮件
    std::vector<std::string> m_researchAreas;  //研究领域

public:
    Author() : m_id(INVALID_ID) {}
    Author(IdType id, const std::string& name, const std::string& gender,
        const std::string& affiliation, const std::string& email,
        const std::vector<std::string>& areas);

    // getters & setters
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

    // 控制台显示

    // 交互式编辑 (控制台或可被GUI替代)

    // 序列化实现
    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

// ========================= 2. 出版物基类（抽象） =========================
class Source : public Serializable {
protected:
    IdType m_id;
    std::string m_shortName;
    std::string m_fullName;
    std::string m_field;
    std::string m_publisher;
    std::string m_publisherAddress;
    std::string m_retrievalType;   // 检索类型, e.g., SCI/EI
    std::string m_remark;

public:
    Source() : m_id(INVALID_ID) {}
    virtual ~Source() = default;

    // 纯虚函数：区分期刊或会议
    virtual std::string getType() const = 0;

    // 通用访问器
    IdType getId() const { return m_id; }
    void setId(IdType id) { m_id = id; }
    const std::string& getShortName() const { return m_shortName; }
    void setShortName(const std::string& name) { m_shortName = name; }
    const std::string& getFullName() const { return m_fullName; }
    void setFullName(const std::string& name) { m_fullName = name; }
    const std::string& getField() const { return m_field; }
    void setField(const std::string& field) { m_field = field; }
    const std::string& getPublisher() const { return m_publisher; }
    void setPublisher(const std::string& pub) { m_publisher = pub; }
    const std::string& getPublisherAddress() const { return m_publisherAddress; }
    void setPublisherAddress(const std::string& addr) { m_publisherAddress = addr; }
    const std::string& getRetrievalType() const { return m_retrievalType; }
    void setRetrievalType(const std::string& type) { m_retrievalType = type; }
    const std::string& getRemark() const { return m_remark; }
    void setRemark(const std::string& remark) { m_remark = remark; }
};

// ========================= 3. 期刊 (继承 Source) =========================
class Journal : public Source {
private:
    double m_latestImpactFactor;
public:
    Journal() : m_latestImpactFactor(0.0) {}
    std::string getType() const override { return "Journal"; }
    double getImpactFactor() const { return m_latestImpactFactor; }
    void setImpactFactor(double ifactor) { m_latestImpactFactor = ifactor; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

// ========================= 4. 会议 (继承 Source) =========================
class Conference : public Source {
private:
    std::string m_meetingAddress;
public:
    std::string getType() const override { return "Conference"; }
    const std::string& getMeetingAddress() const { return m_meetingAddress; }
    void setMeetingAddress(const std::string& addr) { m_meetingAddress = addr; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

// ========================= 5. 附件实体 =========================
class Attachment : public Serializable {
private:
    IdType m_id;
    std::string m_name;
    IdType m_paperId;            // 所属文献ID
    std::string m_content;       // 附件内容

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

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

// ========================= 6. 文献实体 =========================
class Paper : public Serializable {
private:
    IdType m_id;
    std::string m_code;               // 文献编号 (DOI/自定义编号)
    std::string m_title;
    std::vector<std::string> m_keywords;
    std::string m_abstract;
    std::string m_publishDate;      // 格式: YYYY-MM-DD
    IdType m_sourceId;              // 关联的期刊/会议ID
    std::string m_issue;            // 刊期
    std::string m_issueNumber;      // 刊号 ISSN/ISBN
    std::string m_pageRange;
    std::vector<IdType> m_authorIds;      // 作者ID列表
    std::vector<IdType> m_attachmentIds;  // 附件ID列表
    std::string m_filePath;               // 本地全文文件路径
    std::string m_remark;

public:
    Paper() : m_id(INVALID_ID), m_sourceId(INVALID_ID) {}

    // getters & setters
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
    const std::string& getRemark() const { return m_remark; }
    void setRemark(const std::string& remark) { m_remark = remark; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

// ========================= 7. 目录实体 (可嵌套) =========================
class Catalog : public Serializable {
private:
    IdType m_id;
    std::string m_name;
    int m_level;                       // 层级（0=根目录，1=一级子目录...）
    std::string m_description;
    IdType m_parentId;                 // 父目录ID，-1表示根
    std::vector<IdType> m_childIds;    // 子目录ID列表
    std::vector<IdType> m_paperIds;    // 该目录下的文献ID列表

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

// ========================= 8. 核心管理类 LibraryManager =========================
// 负责所有实体的存储、CRUD、文件IO、高级查询
class LibraryManager {
private:
    // 数据容器
    std::map<IdType, Author> m_authors;
    std::map<IdType, Paper> m_papers;
    std::map<IdType, Attachment> m_attachments;
    std::map<IdType, std::shared_ptr<Source>> m_sources;   // 多态存储期刊/会议
    std::map<IdType, Catalog> m_catalogs;

    // ID 自增生成器
    IdType m_nextAuthorId;
    IdType m_nextPaperId;
    IdType m_nextAttachmentId;
    IdType m_nextSourceId;
    IdType m_nextCatalogId;

    // 私有构造函数（单例模式）
    LibraryManager();
    ~LibraryManager() = default;

    // 禁止拷贝与赋值
    LibraryManager(const LibraryManager&) = delete;
    LibraryManager& operator=(const LibraryManager&) = delete;

    // 辅助方法：生成ID
    IdType generateNextId(IdType& counter);

public:
    // 单例访问
    static LibraryManager& getInstance();

    // -------------------- 作者操作 --------------------
    IdType addAuthor(const Author& author);
    bool removeAuthor(IdType id);
    bool updateAuthor(IdType id, const Author& newData);
    Author* findAuthor(IdType id);
    const std::map<IdType, Author>& getAllAuthors() const { return m_authors; }

    // -------------------- 文献操作 --------------------
    IdType addPaper(const Paper& paper);
    bool removePaper(IdType id);          // 级联删除附件，并清理目录中的引用
    bool updatePaper(IdType id, const Paper& newData);
    Paper* findPaper(IdType id);
    const std::map<IdType, Paper>& getAllPapers() const { return m_papers; }

    // -------------------- 附件操作 --------------------
    IdType addAttachment(const Attachment& att);
    bool removeAttachment(IdType id);
    bool updateAttachment(IdType id, const Attachment& newData);
    Attachment* findAttachment(IdType id);
    std::vector<Attachment> getAttachmentsByPaper(IdType paperId) const;
    const std::map<IdType, Attachment>& getAllAttachments() const { return m_attachments; }

    // -------------------- 出版物操作 (期刊/会议) --------------------
    IdType addSource(std::shared_ptr<Source> source);
    bool removeSource(IdType id);
    bool updateSource(IdType id, std::shared_ptr<Source> newSource);
    Source* findSource(IdType id);
    const std::map<IdType, std::shared_ptr<Source>>& getAllSources() const { return m_sources; }

    // -------------------- 目录操作 --------------------
    IdType addCatalog(const Catalog& catalog);
    bool removeCatalog(IdType id);        // 同时将所有子目录的父ID置为当前父目录，或级联删除（按需求）
    bool updateCatalog(IdType id, const Catalog& newData);
    Catalog* findCatalog(IdType id);
    const std::map<IdType, Catalog>& getAllCatalogs() const { return m_catalogs; }
    std::vector<Catalog> getRootCatalogs() const;      // parentId == -1

    // 移动文献到目录
    bool addPaperToCatalog(IdType paperId, IdType catalogId);
    bool removePaperFromCatalog(IdType paperId, IdType catalogId);

    // -------------------- 高级查询 --------------------
    std::vector<Paper> searchPapersByTitle(const std::string& keyword) const;
    std::vector<Paper> searchPapersByAuthor(IdType authorId) const;
    std::vector<Paper> searchPapersByKeyword(const std::string& keyword) const;
    std::vector<Paper> getPapersInCatalog(IdType catalogId) const; // 递归获取所有文献(含子目录)
    std::vector<Paper> getPapersByAuthorName(const std::string& authorName) const;
    std::vector<Paper> getPapersBySource(IdType sourceId) const;   // 按期刊/会议查询文献

    // 辅助名称查询（通过ID获取名称，用于显示）
    std::string getAuthorName(IdType id) const;
    std::string getSourceName(IdType id) const;

    // -------------------- 文件持久化 --------------------
    bool saveToFile(const std::string& filePath) const;   // 保存全部数据 (JSON格式)
    bool loadFromFile(const std::string& filePath);       // 加载数据，清空当前状态

    // 重置整个数据库
    void clearAll();
};

// 异常类
class LibraryException : public std::runtime_error {
public:
    explicit LibraryException(const std::string& msg) : std::runtime_error(msg) {}
};
#endif // !MANAGE_H
