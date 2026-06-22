#ifndef LIBRARY_MANAGER_H
#define LIBRARY_MANAGER_H

#include "Attachment.h"
#include "Author.h"
#include "AuthorCatalog.h"
#include "Catalog.h"
#include "Conference.h"
#include "Journal.h"
#include "Paper.h"
#include "Source.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class LibraryManager {
private:
    std::map<IdType, Author> m_authors;
    std::map<IdType, Paper> m_papers;
    std::map<IdType, Attachment> m_attachments;
    std::map<IdType, std::shared_ptr<Source>> m_sources;
    std::map<IdType, Catalog> m_catalogs;
    std::map<IdType, AuthorCatalog> m_authorCatalogs;

    IdType m_nextAuthorId;
    IdType m_nextPaperId;
    IdType m_nextAttachmentId;
    IdType m_nextSourceId;
    IdType m_nextCatalogId;
    IdType m_nextAuthorCatalogId;

    LibraryManager();
    ~LibraryManager() = default;

    LibraryManager(const LibraryManager&) = delete;
    LibraryManager& operator=(const LibraryManager&) = delete;

    IdType generateNextId(IdType& counter);

public:
    static LibraryManager& getInstance();

    IdType addAuthor(const Author& author);
    bool removeAuthor(IdType id);
    bool updateAuthor(IdType id, const Author& newData);
    Author* findAuthor(IdType id);
    const std::map<IdType, Author>& getAllAuthors() const { return m_authors; }

    IdType addPaper(const Paper& paper);
    bool removePaper(IdType id);
    bool updatePaper(IdType id, const Paper& newData);
    Paper* findPaper(IdType id);
    const std::map<IdType, Paper>& getAllPapers() const { return m_papers; }

    IdType addAttachment(const Attachment& att);
    bool removeAttachment(IdType id);
    bool updateAttachment(IdType id, const Attachment& newData);
    Attachment* findAttachment(IdType id);
    std::vector<Attachment> getAttachmentsByPaper(IdType paperId) const;
    const std::map<IdType, Attachment>& getAllAttachments() const { return m_attachments; }

    IdType addSource(std::shared_ptr<Source> source);
    bool removeSource(IdType id);
    bool updateSource(IdType id, std::shared_ptr<Source> newSource);
    Source* findSource(IdType id);
    const std::map<IdType, std::shared_ptr<Source>>& getAllSources() const { return m_sources; }

    IdType addCatalog(const Catalog& catalog);
    bool removeCatalog(IdType id);
    bool updateCatalog(IdType id, const Catalog& newData);
    Catalog* findCatalog(IdType id);
    const std::map<IdType, Catalog>& getAllCatalogs() const { return m_catalogs; }
    std::vector<Catalog> getRootCatalogs() const;

    bool addPaperToCatalog(IdType paperId, IdType catalogId);
    bool removePaperFromCatalog(IdType paperId, IdType catalogId);

    IdType addAuthorCatalog(const AuthorCatalog& catalog);
    bool removeAuthorCatalog(IdType id);
    bool updateAuthorCatalog(IdType id, const AuthorCatalog& newData);
    AuthorCatalog* findAuthorCatalog(IdType id);
    const std::map<IdType, AuthorCatalog>& getAllAuthorCatalogs() const { return m_authorCatalogs; }

    bool addAuthorToCatalog(IdType authorId, IdType catalogId);
    bool removeAuthorFromCatalog(IdType authorId, IdType catalogId);
    std::vector<Author> getAuthorsInCatalog(IdType catalogId) const;

    std::vector<Paper> searchPapersByTitle(const std::string& keyword) const;
    std::vector<Paper> searchPapersByAuthor(IdType authorId) const;
    std::vector<Paper> searchPapersByKeyword(const std::string& keyword) const;
    std::vector<Paper> getPapersInCatalog(IdType catalogId) const;
    std::vector<Paper> getPapersByAuthorName(const std::string& authorName) const;
    std::vector<Paper> getPapersBySource(IdType sourceId) const;

    std::string getAuthorName(IdType id) const;
    std::string getSourceName(IdType id) const;

    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);
    bool importFromFile(const std::string& filePath);
    bool saveToDirectory(const std::string& directoryPath) const;
    bool loadFromDirectory(const std::string& directoryPath);

    void clearAll();
};

#endif // LIBRARY_MANAGER_H
