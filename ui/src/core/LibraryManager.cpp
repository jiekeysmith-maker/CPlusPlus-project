#include "LibraryManager.h"

#include "SerializationUtils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

LibraryManager::LibraryManager()
    : m_nextAuthorId(1), m_nextPaperId(1), m_nextAttachmentId(1),
    m_nextSourceId(1), m_nextCatalogId(1), m_nextAuthorCatalogId(1) {
}

LibraryManager& LibraryManager::getInstance() {
    static LibraryManager instance;
    return instance;
}

IdType LibraryManager::generateNextId(IdType& counter) {
    return counter++;
}

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

    for (auto& pair : m_papers) {
        auto ids = pair.second.getAuthorIds();
        auto removeIt = std::remove(ids.begin(), ids.end(), id);
        if (removeIt != ids.end()) {
            ids.erase(removeIt, ids.end());
            pair.second.setAuthorIds(ids);
        }
    }
    for (auto& pair : m_authorCatalogs) {
        pair.second.removeAuthorId(id);
    }
    m_authors.erase(it);
    return true;
}

bool LibraryManager::updateAuthor(IdType id, const Author& newData) {
    auto it = m_authors.find(id);
    if (it == m_authors.end()) return false;
    it->second = newData;
    it->second.setId(id);
    return true;
}

Author* LibraryManager::findAuthor(IdType id) {
    auto it = m_authors.find(id);
    return (it != m_authors.end()) ? &it->second : nullptr;
}

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

    std::vector<IdType> attIds = it->second.getAttachmentIds();
    for (IdType aid : attIds)
        m_attachments.erase(aid);

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

IdType LibraryManager::addAttachment(const Attachment& att) {
    IdType id = generateNextId(m_nextAttachmentId);
    Attachment newAtt = att;
    newAtt.setId(id);
    m_attachments[id] = newAtt;

    Paper* paper = findPaper(att.getPaperId());
    if (paper)
        paper->addAttachmentId(id);

    return id;
}

bool LibraryManager::removeAttachment(IdType id) {
    auto it = m_attachments.find(id);
    if (it == m_attachments.end()) return false;

    IdType paperId = it->second.getPaperId();
    Paper* paper = findPaper(paperId);
    if (paper) {
        auto attIds = paper->getAttachmentIds();
        auto removeIt = std::remove(attIds.begin(), attIds.end(), id);
        if (removeIt != attIds.end()) {
            attIds.erase(removeIt, attIds.end());
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

IdType LibraryManager::addSource(std::shared_ptr<Source> source) {
    IdType id = generateNextId(m_nextSourceId);
    source->setId(id);
    m_sources[id] = source;
    return id;
}

bool LibraryManager::removeSource(IdType id) {
    auto it = m_sources.find(id);
    if (it == m_sources.end()) return false;

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

IdType LibraryManager::addCatalog(const Catalog& catalog) {
    IdType id = generateNextId(m_nextCatalogId);
    Catalog newCat = catalog;
    newCat.setId(id);

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

    if (cat.getParentId() != INVALID_ID) {
        Catalog* parent = findCatalog(cat.getParentId());
        if (parent)
            parent->removeChildId(id);
    }

    for (IdType childId : cat.getChildIds()) {
        Catalog* child = findCatalog(childId);
        if (child) {
            child->setParentId(cat.getParentId());
            child->setLevel(std::max(0, child->getLevel() - 1));
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

IdType LibraryManager::addAuthorCatalog(const AuthorCatalog& catalog) {
    IdType id = generateNextId(m_nextAuthorCatalogId);
    AuthorCatalog newCat = catalog;
    newCat.setId(id);

    if (newCat.getParentId() != INVALID_ID) {
        AuthorCatalog* parent = findAuthorCatalog(newCat.getParentId());
        newCat.setLevel(parent ? parent->getLevel() + 1 : 0);
        if (parent) parent->addChildId(id);
    } else {
        newCat.setLevel(0);
    }
    m_authorCatalogs[id] = newCat;
    return id;
}

bool LibraryManager::removeAuthorCatalog(IdType id) {
    auto it = m_authorCatalogs.find(id);
    if (it == m_authorCatalogs.end()) return false;

    AuthorCatalog cat = it->second;

    if (cat.getParentId() != INVALID_ID) {
        AuthorCatalog* parent = findAuthorCatalog(cat.getParentId());
        if (parent)
            parent->removeChildId(id);
    }

    for (IdType childId : cat.getChildIds()) {
        AuthorCatalog* child = findAuthorCatalog(childId);
        if (child) {
            child->setParentId(cat.getParentId());
            child->setLevel(std::max(0, child->getLevel() - 1));
            if (cat.getParentId() != INVALID_ID) {
                AuthorCatalog* grandParent = findAuthorCatalog(cat.getParentId());
                if (grandParent)
                    grandParent->addChildId(childId);
            }
        }
    }

    m_authorCatalogs.erase(it);
    return true;
}

bool LibraryManager::updateAuthorCatalog(IdType id, const AuthorCatalog& newData) {
    auto it = m_authorCatalogs.find(id);
    if (it == m_authorCatalogs.end()) return false;

    const IdType oldParentId = it->second.getParentId();
    const IdType newParentId = newData.getParentId();
    if (oldParentId != newParentId) {
        if (AuthorCatalog* oldParent = findAuthorCatalog(oldParentId))
            oldParent->removeChildId(id);
        if (AuthorCatalog* newParent = findAuthorCatalog(newParentId))
            newParent->addChildId(id);
    }

    it->second = newData;
    it->second.setId(id);
    if (newParentId != INVALID_ID) {
        AuthorCatalog* parent = findAuthorCatalog(newParentId);
        it->second.setLevel(parent ? parent->getLevel() + 1 : 0);
    } else {
        it->second.setLevel(0);
    }
    return true;
}

AuthorCatalog* LibraryManager::findAuthorCatalog(IdType id) {
    auto it = m_authorCatalogs.find(id);
    return (it != m_authorCatalogs.end()) ? &it->second : nullptr;
}

bool LibraryManager::addAuthorToCatalog(IdType authorId, IdType catalogId) {
    if (findAuthor(authorId) == nullptr || findAuthorCatalog(catalogId) == nullptr)
        return false;
    findAuthorCatalog(catalogId)->addAuthorId(authorId);
    return true;
}

bool LibraryManager::removeAuthorFromCatalog(IdType authorId, IdType catalogId) {
    AuthorCatalog* cat = findAuthorCatalog(catalogId);
    if (cat == nullptr) return false;
    cat->removeAuthorId(authorId);
    return true;
}

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

static void collectPapersRecursive(const std::map<IdType, Catalog>& catalogs,
    IdType catalogId,
    const std::map<IdType, Paper>& papers,
    std::map<IdType, bool>& visited,
    std::vector<Paper>& result) {
    auto it = catalogs.find(catalogId);
    if (it == catalogs.end()) return;

    for (IdType pid : it->second.getPaperIds()) {
        if (!visited[pid]) {
            visited[pid] = true;
            auto pit = papers.find(pid);
            if (pit != papers.end())
                result.push_back(pit->second);
        }
    }

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

static void collectAuthorsRecursive(const std::map<IdType, AuthorCatalog>& catalogs,
    IdType catalogId,
    const std::map<IdType, Author>& authors,
    std::map<IdType, bool>& visited,
    std::vector<Author>& result) {
    auto it = catalogs.find(catalogId);
    if (it == catalogs.end()) return;

    for (IdType aid : it->second.getAuthorIds()) {
        if (!visited[aid]) {
            visited[aid] = true;
            auto ait = authors.find(aid);
            if (ait != authors.end())
                result.push_back(ait->second);
        }
    }

    for (IdType childId : it->second.getChildIds()) {
        collectAuthorsRecursive(catalogs, childId, authors, visited, result);
    }
}

std::vector<Author> LibraryManager::getAuthorsInCatalog(IdType catalogId) const {
    std::vector<Author> result;
    std::map<IdType, bool> visited;
    collectAuthorsRecursive(m_authorCatalogs, catalogId, m_authors, visited, result);
    return result;
}

std::vector<Paper> LibraryManager::getPapersByAuthorName(const std::string& authorName) const {
    std::vector<Paper> result;
    std::vector<IdType> matchedAuthorIds;
    for (const auto& pair : m_authors) {
        if (pair.second.getName().find(authorName) != std::string::npos)
            matchedAuthorIds.push_back(pair.first);
    }
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

static const char* SECTION_AUTHORS = "[AUTHORS]";
static const char* SECTION_SOURCES = "[SOURCES]";
static const char* SECTION_PAPERS = "[PAPERS]";
static const char* SECTION_ATTACHMENTS = "[ATTACHMENTS]";
static const char* SECTION_CATALOGS = "[CATALOGS]";
static const char* SECTION_AUTHOR_CATALOGS = "[AUTHOR_CATALOGS]";

namespace {
namespace fs = std::filesystem;

const char* DIR_AUTHORS = "authors";
const char* DIR_SOURCES = "sources";
const char* DIR_PAPERS = "papers";
const char* DIR_ATTACHMENTS = "attachments";
const char* DIR_CATALOGS = "catalogs";
const char* DIR_AUTHOR_CATALOGS = "author_catalogs";

fs::path authorsFilePath(const fs::path& root) { return root / DIR_AUTHORS / "authors.txt"; }
fs::path sourcesFilePath(const fs::path& root) { return root / DIR_SOURCES / "sources.txt"; }
fs::path papersFilePath(const fs::path& root) { return root / DIR_PAPERS / "papers.txt"; }
fs::path attachmentsFilePath(const fs::path& root) { return root / DIR_ATTACHMENTS / "attachments.txt"; }
fs::path catalogsFilePath(const fs::path& root) { return root / DIR_CATALOGS / "catalogs.txt"; }
fs::path authorCatalogsFilePath(const fs::path& root) { return root / DIR_AUTHOR_CATALOGS / "author_catalogs.txt"; }

bool ensureDataDirectories(const fs::path& root)
{
    std::error_code ec;
    fs::create_directories(root / DIR_AUTHORS, ec);
    if (ec) return false;
    fs::create_directories(root / DIR_SOURCES, ec);
    if (ec) return false;
    fs::create_directories(root / DIR_PAPERS, ec);
    if (ec) return false;
    fs::create_directories(root / DIR_ATTACHMENTS, ec);
    if (ec) return false;
    fs::create_directories(root / DIR_CATALOGS, ec);
    if (ec) return false;
    fs::create_directories(root / DIR_AUTHOR_CATALOGS, ec);
    return !ec;
}

bool hasAnyLibraryDataFile(const fs::path& root)
{
    return fs::exists(authorsFilePath(root))
        || fs::exists(sourcesFilePath(root))
        || fs::exists(papersFilePath(root))
        || fs::exists(attachmentsFilePath(root))
        || fs::exists(catalogsFilePath(root))
        || fs::exists(authorCatalogsFilePath(root));
}

template <typename Handler>
bool loadLines(const fs::path& filePath, Handler handler)
{
    if (!fs::exists(filePath)) {
        return true;
    }

    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return false;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(ifs, line)) {
        if (firstLine && line.rfind("\xEF\xBB\xBF", 0) == 0) {
            line.erase(0, 3);
        }
        firstLine = false;
        if (!line.empty()) {
            handler(line);
        }
    }
    return true;
}
}

bool LibraryManager::saveToFile(const std::string& filePath) const {
    std::ofstream ofs(filePath);
    if (!ofs.is_open()) return false;

    ofs << SECTION_AUTHORS << "\n";
    for (const auto& pair : m_authors)
        ofs << pair.second.serialize() << "\n";

    ofs << SECTION_SOURCES << "\n";
    for (const auto& pair : m_sources) {
        ofs << pair.second->getType() << FIELD_SEP << pair.second->serialize() << "\n";
    }

    ofs << SECTION_PAPERS << "\n";
    for (const auto& pair : m_papers)
        ofs << pair.second.serialize() << "\n";

    ofs << SECTION_ATTACHMENTS << "\n";
    for (const auto& pair : m_attachments)
        ofs << pair.second.serialize() << "\n";

    ofs << SECTION_CATALOGS << "\n";
    for (const auto& pair : m_catalogs)
        ofs << pair.second.serialize() << "\n";

    ofs << SECTION_AUTHOR_CATALOGS << "\n";
    for (const auto& pair : m_authorCatalogs)
        ofs << pair.second.serialize() << "\n";

    ofs.close();
    return true;
}

bool LibraryManager::saveToDirectory(const std::string& directoryPath) const {
    if (directoryPath.empty()) return false;

    const fs::path root(directoryPath);
    if (!ensureDataDirectories(root)) return false;

    {
        std::ofstream ofs(authorsFilePath(root));
        if (!ofs.is_open()) return false;
        for (const auto& pair : m_authors)
            ofs << pair.second.serialize() << "\n";
    }

    {
        std::ofstream ofs(sourcesFilePath(root));
        if (!ofs.is_open()) return false;
        for (const auto& pair : m_sources)
            ofs << pair.second->getType() << FIELD_SEP << pair.second->serialize() << "\n";
    }

    {
        std::ofstream ofs(papersFilePath(root));
        if (!ofs.is_open()) return false;
        for (const auto& pair : m_papers)
            ofs << pair.second.serialize() << "\n";
    }

    {
        std::ofstream ofs(attachmentsFilePath(root));
        if (!ofs.is_open()) return false;
        for (const auto& pair : m_attachments)
            ofs << pair.second.serialize() << "\n";
    }

    {
        std::ofstream ofs(catalogsFilePath(root));
        if (!ofs.is_open()) return false;
        for (const auto& pair : m_catalogs)
            ofs << pair.second.serialize() << "\n";
    }

    {
        std::ofstream ofs(authorCatalogsFilePath(root));
        if (!ofs.is_open()) return false;
        for (const auto& pair : m_authorCatalogs)
            ofs << pair.second.serialize() << "\n";
    }

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

        if (line == SECTION_AUTHORS) { currentSection = SECTION_AUTHORS; continue; }
        if (line == SECTION_SOURCES) { currentSection = SECTION_SOURCES; continue; }
        if (line == SECTION_PAPERS) { currentSection = SECTION_PAPERS; continue; }
        if (line == SECTION_ATTACHMENTS) { currentSection = SECTION_ATTACHMENTS; continue; }
        if (line == SECTION_CATALOGS) { currentSection = SECTION_CATALOGS; continue; }
        if (line == SECTION_AUTHOR_CATALOGS) { currentSection = SECTION_AUTHOR_CATALOGS; continue; }

        if (currentSection == SECTION_AUTHORS) {
            Author a;
            a.deserialize(line);
            m_authors[a.getId()] = a;
            if (a.getId() >= m_nextAuthorId)
                m_nextAuthorId = a.getId() + 1;
        }
        else if (currentSection == SECTION_SOURCES) {
            auto parts = split(line, FIELD_SEP);
            if (parts.size() < 2) continue;
            std::string type = parts[0];
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
        else if (currentSection == SECTION_AUTHOR_CATALOGS) {
            AuthorCatalog c;
            c.deserialize(line);
            m_authorCatalogs[c.getId()] = c;
            if (c.getId() >= m_nextAuthorCatalogId)
                m_nextAuthorCatalogId = c.getId() + 1;
        }
    }

    ifs.close();
    return true;
}

bool LibraryManager::importFromFile(const std::string& filePath) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) return false;

    std::map<IdType, Author> importedAuthors;
    std::map<IdType, std::shared_ptr<Source>> importedSources;
    std::map<IdType, Paper> importedPapers;
    std::map<IdType, Attachment> importedAttachments;
    std::map<IdType, Catalog> importedCatalogs;
    std::map<IdType, AuthorCatalog> importedAuthorCatalogs;

    std::string line;
    std::string currentSection;

    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        if (line == SECTION_AUTHORS) { currentSection = SECTION_AUTHORS; continue; }
        if (line == SECTION_SOURCES) { currentSection = SECTION_SOURCES; continue; }
        if (line == SECTION_PAPERS) { currentSection = SECTION_PAPERS; continue; }
        if (line == SECTION_ATTACHMENTS) { currentSection = SECTION_ATTACHMENTS; continue; }
        if (line == SECTION_CATALOGS) { currentSection = SECTION_CATALOGS; continue; }
        if (line == SECTION_AUTHOR_CATALOGS) { currentSection = SECTION_AUTHOR_CATALOGS; continue; }

        if (currentSection == SECTION_AUTHORS) {
            Author a;
            a.deserialize(line);
            importedAuthors[a.getId()] = a;
        }
        else if (currentSection == SECTION_SOURCES) {
            auto parts = split(line, FIELD_SEP);
            if (parts.size() < 2) continue;
            std::string type = parts[0];
            std::string rest;
            for (size_t i = 1; i < parts.size(); ++i) {
                if (i > 1) rest += FIELD_SEP;
                rest += parts[i];
            }

            if (type == "Journal") {
                auto j = std::make_shared<Journal>();
                j->deserialize(rest);
                importedSources[j->getId()] = j;
            }
            else if (type == "Conference") {
                auto c = std::make_shared<Conference>();
                c->deserialize(rest);
                importedSources[c->getId()] = c;
            }
        }
        else if (currentSection == SECTION_PAPERS) {
            Paper p;
            p.deserialize(line);
            importedPapers[p.getId()] = p;
        }
        else if (currentSection == SECTION_ATTACHMENTS) {
            Attachment a;
            a.deserialize(line);
            importedAttachments[a.getId()] = a;
        }
        else if (currentSection == SECTION_CATALOGS) {
            Catalog c;
            c.deserialize(line);
            importedCatalogs[c.getId()] = c;
        }
        else if (currentSection == SECTION_AUTHOR_CATALOGS) {
            AuthorCatalog c;
            c.deserialize(line);
            importedAuthorCatalogs[c.getId()] = c;
        }
    }

    std::map<IdType, IdType> authorIdMap;
    std::map<IdType, IdType> sourceIdMap;
    std::map<IdType, IdType> paperIdMap;
    std::map<IdType, IdType> catalogIdMap;
    std::map<IdType, IdType> authorCatalogIdMap;

    for (const auto& pair : importedAuthors) {
        Author author = pair.second;
        authorIdMap[pair.first] = addAuthor(author);
    }

    for (const auto& pair : importedSources) {
        sourceIdMap[pair.first] = addSource(pair.second);
    }

    for (const auto& pair : importedPapers) {
        Paper paper = pair.second;

        std::vector<IdType> remappedAuthorIds;
        for (IdType oldAuthorId : paper.getAuthorIds()) {
            auto it = authorIdMap.find(oldAuthorId);
            if (it != authorIdMap.end()) {
                remappedAuthorIds.push_back(it->second);
            }
        }
        paper.setAuthorIds(remappedAuthorIds);

        auto sourceIt = sourceIdMap.find(paper.getSourceId());
        paper.setSourceId(sourceIt != sourceIdMap.end() ? sourceIt->second : INVALID_ID);
        paper.setAttachmentIds({});

        paperIdMap[pair.first] = addPaper(paper);
    }

    for (const auto& pair : importedAttachments) {
        Attachment attachment = pair.second;
        auto paperIt = paperIdMap.find(attachment.getPaperId());
        if (paperIt == paperIdMap.end()) {
            continue;
        }
        attachment.setPaperId(paperIt->second);
        addAttachment(attachment);
    }

    std::set<IdType> pendingCatalogIds;
    for (const auto& pair : importedCatalogs) {
        pendingCatalogIds.insert(pair.first);
    }
    while (!pendingCatalogIds.empty()) {
        const size_t before = pendingCatalogIds.size();
        for (auto it = pendingCatalogIds.begin(); it != pendingCatalogIds.end(); ) {
            const IdType oldCatalogId = *it;
            Catalog catalog = importedCatalogs[oldCatalogId];
            const IdType oldParentId = catalog.getParentId();
            if (oldParentId != INVALID_ID && !catalogIdMap.count(oldParentId)) {
                ++it;
                continue;
            }

            std::vector<IdType> remappedPaperIds;
            for (IdType oldPaperId : catalog.getPaperIds()) {
                auto paperIt = paperIdMap.find(oldPaperId);
                if (paperIt != paperIdMap.end()) {
                    remappedPaperIds.push_back(paperIt->second);
                }
            }
            catalog.setPaperIds(remappedPaperIds);
            catalog.setChildIds({});
            catalog.setParentId(oldParentId == INVALID_ID ? INVALID_ID : catalogIdMap[oldParentId]);
            catalogIdMap[oldCatalogId] = addCatalog(catalog);
            it = pendingCatalogIds.erase(it);
        }

        if (pendingCatalogIds.size() == before) {
            for (IdType oldCatalogId : pendingCatalogIds) {
                Catalog catalog = importedCatalogs[oldCatalogId];
                std::vector<IdType> remappedPaperIds;
                for (IdType oldPaperId : catalog.getPaperIds()) {
                    auto paperIt = paperIdMap.find(oldPaperId);
                    if (paperIt != paperIdMap.end()) {
                        remappedPaperIds.push_back(paperIt->second);
                    }
                }
                catalog.setPaperIds(remappedPaperIds);
                catalog.setChildIds({});
                catalog.setParentId(INVALID_ID);
                catalogIdMap[oldCatalogId] = addCatalog(catalog);
            }
            pendingCatalogIds.clear();
        }
    }

    std::set<IdType> pendingAuthorCatalogIds;
    for (const auto& pair : importedAuthorCatalogs) {
        pendingAuthorCatalogIds.insert(pair.first);
    }
    while (!pendingAuthorCatalogIds.empty()) {
        const size_t before = pendingAuthorCatalogIds.size();
        for (auto it = pendingAuthorCatalogIds.begin(); it != pendingAuthorCatalogIds.end(); ) {
            const IdType oldCatalogId = *it;
            AuthorCatalog catalog = importedAuthorCatalogs[oldCatalogId];
            const IdType oldParentId = catalog.getParentId();
            if (oldParentId != INVALID_ID && !authorCatalogIdMap.count(oldParentId)) {
                ++it;
                continue;
            }

            std::vector<IdType> remappedAuthorIds;
            for (IdType oldAuthorId : catalog.getAuthorIds()) {
                auto authorIt = authorIdMap.find(oldAuthorId);
                if (authorIt != authorIdMap.end()) {
                    remappedAuthorIds.push_back(authorIt->second);
                }
            }
            catalog.setAuthorIds(remappedAuthorIds);
            catalog.setChildIds({});
            catalog.setParentId(oldParentId == INVALID_ID ? INVALID_ID : authorCatalogIdMap[oldParentId]);
            authorCatalogIdMap[oldCatalogId] = addAuthorCatalog(catalog);
            it = pendingAuthorCatalogIds.erase(it);
        }

        if (pendingAuthorCatalogIds.size() == before) {
            for (IdType oldCatalogId : pendingAuthorCatalogIds) {
                AuthorCatalog catalog = importedAuthorCatalogs[oldCatalogId];
                std::vector<IdType> remappedAuthorIds;
                for (IdType oldAuthorId : catalog.getAuthorIds()) {
                    auto authorIt = authorIdMap.find(oldAuthorId);
                    if (authorIt != authorIdMap.end()) {
                        remappedAuthorIds.push_back(authorIt->second);
                    }
                }
                catalog.setAuthorIds(remappedAuthorIds);
                catalog.setChildIds({});
                catalog.setParentId(INVALID_ID);
                authorCatalogIdMap[oldCatalogId] = addAuthorCatalog(catalog);
            }
            pendingAuthorCatalogIds.clear();
        }
    }

    return true;
}

bool LibraryManager::loadFromDirectory(const std::string& directoryPath) {
    if (directoryPath.empty()) return false;

    const fs::path root(directoryPath);
    if (!hasAnyLibraryDataFile(root)) return false;

    clearAll();

    if (!loadLines(authorsFilePath(root), [this](const std::string& line) {
            Author a;
            a.deserialize(line);
            m_authors[a.getId()] = a;
            if (a.getId() >= m_nextAuthorId)
                m_nextAuthorId = a.getId() + 1;
        })) {
        return false;
    }

    if (!loadLines(sourcesFilePath(root), [this](const std::string& line) {
            auto parts = split(line, FIELD_SEP);
            if (parts.size() < 2) return;
            std::string type = parts[0];
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
        })) {
        return false;
    }

    if (!loadLines(papersFilePath(root), [this](const std::string& line) {
            Paper p;
            p.deserialize(line);
            m_papers[p.getId()] = p;
            if (p.getId() >= m_nextPaperId)
                m_nextPaperId = p.getId() + 1;
        })) {
        return false;
    }

    if (!loadLines(attachmentsFilePath(root), [this](const std::string& line) {
            Attachment a;
            a.deserialize(line);
            m_attachments[a.getId()] = a;
            if (a.getId() >= m_nextAttachmentId)
                m_nextAttachmentId = a.getId() + 1;
        })) {
        return false;
    }

    if (!loadLines(catalogsFilePath(root), [this](const std::string& line) {
            Catalog c;
            c.deserialize(line);
            m_catalogs[c.getId()] = c;
            if (c.getId() >= m_nextCatalogId)
                m_nextCatalogId = c.getId() + 1;
        })) {
        return false;
    }

    if (!loadLines(authorCatalogsFilePath(root), [this](const std::string& line) {
            AuthorCatalog c;
            c.deserialize(line);
            m_authorCatalogs[c.getId()] = c;
            if (c.getId() >= m_nextAuthorCatalogId)
                m_nextAuthorCatalogId = c.getId() + 1;
        })) {
        return false;
    }

    return true;
}

void LibraryManager::clearAll() {
    m_authors.clear();
    m_papers.clear();
    m_attachments.clear();
    m_sources.clear();
    m_catalogs.clear();
    m_authorCatalogs.clear();
    m_nextAuthorId = 1;
    m_nextPaperId = 1;
    m_nextAttachmentId = 1;
    m_nextSourceId = 1;
    m_nextCatalogId = 1;
    m_nextAuthorCatalogId = 1;
}
