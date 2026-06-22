#ifndef ONLINE_METADATA_SERVICE_H
#define ONLINE_METADATA_SERVICE_H

#include <QString>
#include <QStringList>

class OnlineMetadataService
{
public:
    struct LookupResult {
        bool success = false;
        QString errorMessage;

        QString sourceName;
        QString doi;
        QString arxivId;
        QString title;
        QStringList authors;
        QString abstractText;
        QStringList keywords;
        QString publicationDate;
        QString venue;
        QString volume;
        QString issue;
        QString pages;
        QString publisher;
        QString url;
        double titleSimilarity = 0.0;
    };

    static LookupResult lookupByDoi(const QString &doi);
    static LookupResult lookupByArxivId(const QString &arxivId);
    static LookupResult lookupByTitle(const QString &titleCandidate);
    static bool isNetworkAvailable();
};

#endif // ONLINE_METADATA_SERVICE_H
