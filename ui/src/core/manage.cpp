#include "manage.h"

#include <sstream>
#include <algorithm>
#include <fstream>

// ========================= 辅助函数 =========================

// 字段分隔符使用 ASCII 不可见字符 \x1E (Record Separator)
// 避免用户输入中的 "|" 等可见字符破坏序列化格式
constexpr char FIELD_SEP = '\x1E';

// 分割字符串：将 "a;b;c" → {"a","b","c"}
// 设计意图：all实体序列化中用";"分隔数组元素，反序列化时需要此函数还原
// 注意：必须保留空字符串，否则序列化时连续分隔符会导致字段错位。
// getline 不处理末尾分隔符，需额外补偿。
static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    if (!s.empty() && s.back() == delim) {
        result.push_back("");
    }
    return result;
}

// 拼接字符串：将 {"a","b","c"} → "a;b;c"
static std::string join(const std::vector<std::string>& vec, char delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += delim;
        result += vec[i];
    }
    return result;
}

// 拼接ID列表
static std::string joinIds(const std::vector<IdType>& vec, char delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += delim;
        result += std::to_string(vec[i]);
    }
    return result;
}

// 解析ID列表
std::vector<IdType> splitIds(const std::string& s, char delim) {
    std::vector<IdType> result;
    if (s.empty()) return result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty())
            result.push_back(std::stoi(item));
    }
    return result;
}

// ========================= Author 实现 =========================

Author::Author(IdType id, const std::string& name, const std::string& gender,
    const std::string& affiliation, const std::string& email,
    const std::vector<std::string>& areas)
    : m_id(id), m_name(name), m_gender(gender),
    m_affiliation(affiliation), m_email(email), m_researchAreas(areas) {
}

// 序列化格式: id|name|gender|affiliation|email|area1;area2
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

// ========================= Source 实现 =========================

// ========================= Journal 实现 =========================

// 序列化格式: id|shortName|fullName|field|publisher|address|retrieval|remark|impactFactor
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

// ========================= Conference 实现 =========================

// 序列化格式: id|shortName|fullName|field|publisher|address|retrieval|remark|meetingAddress
std::string Conference::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_shortName << FIELD_SEP << m_fullName << FIELD_SEP
        << m_field << FIELD_SEP << m_publisher << FIELD_SEP << m_publisherAddress << FIELD_SEP
        << m_retrievalType << FIELD_SEP << m_remark << FIELD_SEP << m_meetingAddress;
    return ss.str();
}

void Conference::deserialize(const std::string& json) {
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
        m_meetingAddress = parts[8];
    }
}

// ========================= Attachment 实现 =========================

Attachment::Attachment(IdType id, const std::string& name, IdType paperId,
    const std::string& content)
    : m_id(id), m_name(name), m_paperId(paperId), m_content(content) {
}

// 序列化格式: id|name|paperId|content
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

// ========================= Paper 实现 =========================

// 序列化格式: id|code|title|kw1;kw2|abstract|publishDate|sourceId|issue|issueNumber|pageRange|authorIds|attachIds|remark|filePath
std::string Paper::serialize() const {
    std::stringstream ss;
    ss << m_id << FIELD_SEP << m_code << FIELD_SEP << m_title << FIELD_SEP
        << join(m_keywords, ';') << FIELD_SEP << m_abstract << FIELD_SEP
        << m_publishDate << FIELD_SEP << m_sourceId << FIELD_SEP
        << m_issue << FIELD_SEP << m_issueNumber << FIELD_SEP
        << m_pageRange << FIELD_SEP << joinIds(m_authorIds, ';') << FIELD_SEP
        << joinIds(m_attachmentIds, ';') << FIELD_SEP << m_remark << FIELD_SEP
        << m_filePath;
    return ss.str();
}

void Paper::deserialize(const std::string& json) {
    auto parts = split(json, FIELD_SEP);
    if (parts.size() >= 13) {
        m_id = std::stoi(parts[0]);
        m_code = parts[1];
        m_title = parts[2];
        m_keywords = split(parts[3], ';');
        m_abstract = parts[4];
        m_publishDate = parts[5];
        m_sourceId = std::stoi(parts[6]);
        m_issue = parts[7];
        m_issueNumber = parts[8];
        m_pageRange = parts[9];
        m_authorIds = splitIds(parts[10], ';');
        m_attachmentIds = splitIds(parts[11], ';');
        m_remark = parts[12];
        m_filePath = (parts.size() >= 14) ? parts[13] : "";
    }
}

// ========================= Catalog 实现 =========================

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

// 序列化格式: id|name|level|description|parentId|childIds|paperIds
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

// ========================= LibraryManager 实现 =========================

LibraryManager::LibraryManager()
    : m_nextAuthorId(1), m_nextPaperId(1), m_nextAttachmentId(1),
    m_nextSourceId(1), m_nextCatalogId(1) {
}

LibraryManager& LibraryManager::getInstance() {
    static LibraryManager instance;
    return instance;
}

IdType LibraryManager::generateNextId(IdType& counter) {
    return counter++;
}

// -------------------- 作者操作 --------------------

IdType LibraryManager::addAuthor(const Author& author) {
    IdType id = generateNextId(m_nextAuthorId);
    Author newAuthor = author;
    newAuthor.setId(id);
    m_authors[id] = newAuthor;
    return id;
}

bool LibraryManager::removeAuthor(IdType id) {
    auto it = m_authors.find(id);
    if (it == m_authors.end()) return false;

    // 清理所有文献中对该作者的引用
    for (auto& pair : m_papers) {
        auto ids = pair.second.getAuthorIds();  // 拷贝
        auto it = std::remove(ids.begin(), ids.end(), id);
        if (it != ids.end()) {
            ids.erase(it, ids.end());
            pair.second.setAuthorIds(ids);
        }
    }
    m_authors.erase(it);
    return true;
}

bool LibraryManager::updateAuthor(IdType id, const Author& newData) {
    auto it = m_authors.find(id);
    if (it == m_authors.end()) return false;
    it->second = newData;
    it->second.setId(id); // 保持ID不变
    return true;
}

Author* LibraryManager::findAuthor(IdType id) {
    auto it = m_authors.find(id);
    return (it != m_authors.end()) ? &it->second : nullptr;
}

// -------------------- 文献操作 --------------------

IdType LibraryManager::addPaper(const Paper& paper) {
    IdType id = generateNextId(m_nextPaperId);
    Paper newPaper = paper;
    newPaper.setId(id);
    m_papers[id] = newPaper;
    return id;
}

bool LibraryManager::removePaper(IdType id) {
    auto it = m_papers.find(id);
    if (it == m_papers.end()) return false;

    // 级联删除关联附件
    std::vector<IdType> attIds = it->second.getAttachmentIds();
    for (IdType aid : attIds)
        m_attachments.erase(aid);

    // 清理所有目录中对这篇文献的引用
    for (auto& pair : m_catalogs) {
        pair.second.removePaperId(id);
    }

    m_papers.erase(it);
    return true;
}

bool LibraryManager::updatePaper(IdType id, const Paper& newData) {
    auto it = m_papers.find(id);
    if (it == m_papers.end()) return false;
    it->second = newData;
    it->second.setId(id);
    return true;
}

Paper* LibraryManager::findPaper(IdType id) {
    auto it = m_papers.find(id);
    return (it != m_papers.end()) ? &it->second : nullptr;
}

// -------------------- 附件操作 --------------------

IdType LibraryManager::addAttachment(const Attachment& att) {
    IdType id = generateNextId(m_nextAttachmentId);
    Attachment newAtt = att;
    newAtt.setId(id);
    m_attachments[id] = newAtt;

    // 在对应文献的附件列表中追加此附件ID
    Paper* paper = findPaper(att.getPaperId());
    if (paper)
        paper->addAttachmentId(id);

    return id;
}

bool LibraryManager::removeAttachment(IdType id) {
    auto it = m_attachments.find(id);
    if (it == m_attachments.end()) return false;

    // 从对应文献的附件列表中移除
    IdType paperId = it->second.getPaperId();
    Paper* paper = findPaper(paperId);
    if (paper) {
        auto attIds = paper->getAttachmentIds();  // 拷贝
        auto it = std::remove(attIds.begin(), attIds.end(), id);
        if (it != attIds.end()) {
            attIds.erase(it, attIds.end());
            paper->setAttachmentIds(attIds);
        }
    }

    m_attachments.erase(it);
    return true;
}

bool LibraryManager::updateAttachment(IdType id, const Attachment& newData) {
    auto it = m_attachments.find(id);
    if (it == m_attachments.end()) return false;
    it->second = newData;
    it->second.setId(id);
    return true;
}

Attachment* LibraryManager::findAttachment(IdType id) {
    auto it = m_attachments.find(id);
    return (it != m_attachments.end()) ? &it->second : nullptr;
}

std::vector<Attachment> LibraryManager::getAttachmentsByPaper(IdType paperId) const {
    std::vector<Attachment> result;
    for (const auto& pair : m_attachments) {
        if (pair.second.getPaperId() == paperId)
            result.push_back(pair.second);
    }
    return result;
}

// -------------------- 出版物操作 --------------------

IdType LibraryManager::addSource(std::shared_ptr<Source> source) {
    IdType id = generateNextId(m_nextSourceId);
    source->setId(id);
    m_sources[id] = source;
    return id;
}

bool LibraryManager::removeSource(IdType id) {
    auto it = m_sources.find(id);
    if (it == m_sources.end()) return false;

    // 清理所有文献中对该出版物的引用（将 sourceId 置为 INVALID_ID）
    for (auto& pair : m_papers) {
        if (pair.second.getSourceId() == id)
            pair.second.setSourceId(INVALID_ID);
    }

    m_sources.erase(it);
    return true;
}

bool LibraryManager::updateSource(IdType id, std::shared_ptr<Source> newSource) {
    auto it = m_sources.find(id);
    if (it == m_sources.end()) return false;
    newSource->setId(id);
    it->second = newSource;
    return true;
}

Source* LibraryManager::findSource(IdType id) {
    auto it = m_sources.find(id);
    return (it != m_sources.end()) ? it->second.get() : nullptr;
}

// -------------------- 目录操作 --------------------

IdType LibraryManager::addCatalog(const Catalog& catalog) {
    IdType id = generateNextId(m_nextCatalogId);
    Catalog newCat = catalog;
    newCat.setId(id);

    // 自动计算层级：根目录=0，子目录=父层级+1
    if (newCat.getParentId() != INVALID_ID) {
        Catalog* parent = findCatalog(newCat.getParentId());
        newCat.setLevel(parent ? parent->getLevel() + 1 : 0);
        if (parent) parent->addChildId(id);
    } else {
        newCat.setLevel(0);
    }
    m_catalogs[id] = newCat;
    return id;
}

bool LibraryManager::removeCatalog(IdType id) {
    auto it = m_catalogs.find(id);
    if (it == m_catalogs.end()) return false;

    Catalog cat = it->second;

    // 从父目录的子列表中移除
    if (cat.getParentId() != INVALID_ID) {
        Catalog* parent = findCatalog(cat.getParentId());
        if (parent)
            parent->removeChildId(id);
    }

    // 将子目录的父ID全部置为当前目录的父ID（上移一级，避免孤儿）
    for (IdType childId : cat.getChildIds()) {
        Catalog* child = findCatalog(childId);
        if (child) {
            child->setParentId(cat.getParentId());
            child->setLevel(std::max(0, child->getLevel() - 1));
            // 将子目录挂到祖父目录下
            if (cat.getParentId() != INVALID_ID) {
                Catalog* grandParent = findCatalog(cat.getParentId());
                if (grandParent)
                    grandParent->addChildId(childId);
            }
        }
    }

    m_catalogs.erase(it);
    return true;
}

bool LibraryManager::updateCatalog(IdType id, const Catalog& newData) {
    auto it = m_catalogs.find(id);
    if (it == m_catalogs.end()) return false;
    it->second = newData;
    it->second.setId(id);
    // 自动重算层级（父目录可能已变）
    if (newData.getParentId() != INVALID_ID) {
        Catalog* parent = findCatalog(newData.getParentId());
        it->second.setLevel(parent ? parent->getLevel() + 1 : 0);
    } else {
        it->second.setLevel(0);
    }
    return true;
}

Catalog* LibraryManager::findCatalog(IdType id) {
    auto it = m_catalogs.find(id);
    return (it != m_catalogs.end()) ? &it->second : nullptr;
}

std::vector<Catalog> LibraryManager::getRootCatalogs() const {
    std::vector<Catalog> result;
    for (const auto& pair : m_catalogs) {
        if (pair.second.isRoot())
            result.push_back(pair.second);
    }
    return result;
}

bool LibraryManager::addPaperToCatalog(IdType paperId, IdType catalogId) {
    if (findPaper(paperId) == nullptr || findCatalog(catalogId) == nullptr)
        return false;
    findCatalog(catalogId)->addPaperId(paperId);
    return true;
}

bool LibraryManager::removePaperFromCatalog(IdType paperId, IdType catalogId) {
    Catalog* cat = findCatalog(catalogId);
    if (cat == nullptr) return false;
    cat->removePaperId(paperId);
    return true;
}

// -------------------- 高级查询 --------------------

std::vector<Paper> LibraryManager::searchPapersByTitle(const std::string& keyword) const {
    std::vector<Paper> result;
    for (const auto& pair : m_papers) {
        if (pair.second.getTitle().find(keyword) != std::string::npos)
            result.push_back(pair.second);
    }
    return result;
}

std::vector<Paper> LibraryManager::searchPapersByAuthor(IdType authorId) const {
    std::vector<Paper> result;
    for (const auto& pair : m_papers) {
        const auto& ids = pair.second.getAuthorIds();
        if (std::find(ids.begin(), ids.end(), authorId) != ids.end())
            result.push_back(pair.second);
    }
    return result;
}

std::vector<Paper> LibraryManager::searchPapersByKeyword(const std::string& keyword) const {
    std::vector<Paper> result;
    for (const auto& pair : m_papers) {
        const auto& kws = pair.second.getKeywords();
        for (const auto& kw : kws) {
            if (kw.find(keyword) != std::string::npos) {
                result.push_back(pair.second);
                break;
            }
        }
    }
    return result;
}

// 递归获取目录及其所有子目录下的文献
// 设计意图：目录可以嵌套，遍历时需递归进入每个子目录，收集所有文献
void collectPapersRecursive(const std::map<IdType, Catalog>& catalogs,
    IdType catalogId,
    const std::map<IdType, Paper>& papers,
    std::map<IdType, bool>& visited,
    std::vector<Paper>& result) {
    auto it = catalogs.find(catalogId);
    if (it == catalogs.end()) return;

    // 收集当前目录的文献
    for (IdType pid : it->second.getPaperIds()) {
        if (!visited[pid]) {
            visited[pid] = true;
            auto pit = papers.find(pid);
            if (pit != papers.end())
                result.push_back(pit->second);
        }
    }

    // 递归进入子目录
    for (IdType childId : it->second.getChildIds()) {
        collectPapersRecursive(catalogs, childId, papers, visited, result);
    }
}

std::vector<Paper> LibraryManager::getPapersInCatalog(IdType catalogId) const {
    std::vector<Paper> result;
    std::map<IdType, bool> visited;
    collectPapersRecursive(m_catalogs, catalogId, m_papers, visited, result);
    return result;
}

std::vector<Paper> LibraryManager::getPapersByAuthorName(const std::string& authorName) const {
    std::vector<Paper> result;
    // 先找到匹配的作者ID
    std::vector<IdType> matchedAuthorIds;
    for (const auto& pair : m_authors) {
        if (pair.second.getName().find(authorName) != std::string::npos)
            matchedAuthorIds.push_back(pair.first);
    }
    // 找到这些作者的文献
    for (IdType aid : matchedAuthorIds) {
        auto papers = searchPapersByAuthor(aid);
        result.insert(result.end(), papers.begin(), papers.end());
    }
    return result;
}

std::vector<Paper> LibraryManager::getPapersBySource(IdType sourceId) const {
    std::vector<Paper> result;
    for (const auto& pair : m_papers) {
        if (pair.second.getSourceId() == sourceId)
            result.push_back(pair.second);
    }
    return result;
}

std::string LibraryManager::getAuthorName(IdType id) const {
    auto it = m_authors.find(id);
    if (it != m_authors.end())
        return it->second.getName();
    return "未知作者(ID:" + std::to_string(id) + ")";
}

std::string LibraryManager::getSourceName(IdType id) const {
    auto it = m_sources.find(id);
    if (it != m_sources.end())
        return it->second->getShortName();
    return "未知出版物";
}

// -------------------- 文件持久化 --------------------

// 文件格式设计：
//   使用分段标记区分不同实体类型，每段以 [TYPE] 开头，后跟每行一条序列化记录。
//   这种格式比纯JSON更轻量，不依赖第三方库，且对人可读。
static const char* SECTION_AUTHORS = "[AUTHORS]";
static const char* SECTION_SOURCES = "[SOURCES]";
static const char* SECTION_PAPERS = "[PAPERS]";
static const char* SECTION_ATTACHMENTS = "[ATTACHMENTS]";
static const char* SECTION_CATALOGS = "[CATALOGS]";

bool LibraryManager::saveToFile(const std::string& filePath) const {
    std::ofstream ofs(filePath);
    if (!ofs.is_open()) return false;

    // 写入作者
    ofs << SECTION_AUTHORS << "\n";
    for (const auto& pair : m_authors)
        ofs << pair.second.serialize() << "\n";

    // 写入出版物（含类型标记前缀：JOURNAL|... 或 CONFERENCE|...）
    ofs << SECTION_SOURCES << "\n";
    for (const auto& pair : m_sources) {
        ofs << pair.second->getType() << FIELD_SEP << pair.second->serialize() << "\n";
    }

    // 写入文献
    ofs << SECTION_PAPERS << "\n";
    for (const auto& pair : m_papers)
        ofs << pair.second.serialize() << "\n";

    // 写入附件
    ofs << SECTION_ATTACHMENTS << "\n";
    for (const auto& pair : m_attachments)
        ofs << pair.second.serialize() << "\n";

    // 写入目录
    ofs << SECTION_CATALOGS << "\n";
    for (const auto& pair : m_catalogs)
        ofs << pair.second.serialize() << "\n";

    ofs.close();
    return true;
}

bool LibraryManager::loadFromFile(const std::string& filePath) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) return false;

    clearAll();

    std::string line;
    std::string currentSection;

    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        // 判断段标记
        if (line == SECTION_AUTHORS) { currentSection = SECTION_AUTHORS; continue; }
        if (line == SECTION_SOURCES) { currentSection = SECTION_SOURCES; continue; }
        if (line == SECTION_PAPERS) { currentSection = SECTION_PAPERS; continue; }
        if (line == SECTION_ATTACHMENTS) { currentSection = SECTION_ATTACHMENTS; continue; }
        if (line == SECTION_CATALOGS) { currentSection = SECTION_CATALOGS; continue; }

        // 根据当前段解析对应实体
        if (currentSection == SECTION_AUTHORS) {
            Author a;
            a.deserialize(line);
            m_authors[a.getId()] = a;
            if (a.getId() >= m_nextAuthorId)
                m_nextAuthorId = a.getId() + 1;
        }
        else if (currentSection == SECTION_SOURCES) {
            // 出版物格式: Journal|... 或 Conference|...
            auto parts = split(line, FIELD_SEP);
            if (parts.size() < 2) continue;
            std::string type = parts[0];
            // 重组剩余部分为原始序列化串
            std::string rest;
            for (size_t i = 1; i < parts.size(); ++i) {
                if (i > 1) rest += FIELD_SEP;
                rest += parts[i];
            }

            if (type == "Journal") {
                auto j = std::make_shared<Journal>();
                j->deserialize(rest);
                m_sources[j->getId()] = j;
                if (j->getId() >= m_nextSourceId)
                    m_nextSourceId = j->getId() + 1;
            }
            else if (type == "Conference") {
                auto c = std::make_shared<Conference>();
                c->deserialize(rest);
                m_sources[c->getId()] = c;
                if (c->getId() >= m_nextSourceId)
                    m_nextSourceId = c->getId() + 1;
            }
        }
        else if (currentSection == SECTION_PAPERS) {
            Paper p;
            p.deserialize(line);
            m_papers[p.getId()] = p;
            if (p.getId() >= m_nextPaperId)
                m_nextPaperId = p.getId() + 1;
        }
        else if (currentSection == SECTION_ATTACHMENTS) {
            Attachment a;
            a.deserialize(line);
            m_attachments[a.getId()] = a;
            if (a.getId() >= m_nextAttachmentId)
                m_nextAttachmentId = a.getId() + 1;
        }
        else if (currentSection == SECTION_CATALOGS) {
            Catalog c;
            c.deserialize(line);
            m_catalogs[c.getId()] = c;
            if (c.getId() >= m_nextCatalogId)
                m_nextCatalogId = c.getId() + 1;
        }
    }

    ifs.close();
    return true;
}

void LibraryManager::clearAll() {
    m_authors.clear();
    m_papers.clear();
    m_attachments.clear();
    m_sources.clear();
    m_catalogs.clear();
    m_nextAuthorId = 1;
    m_nextPaperId = 1;
    m_nextAttachmentId = 1;
    m_nextSourceId = 1;
    m_nextCatalogId = 1;
}
