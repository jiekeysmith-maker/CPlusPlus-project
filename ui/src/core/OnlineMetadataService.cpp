#include "OnlineMetadataService.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QXmlStreamReader>

#include <algorithm>

namespace {
constexpr int kNetworkTimeoutMs = 7000;
constexpr double kTitleSimilarityThreshold = 0.85;

struct HttpResult {
    bool success = false;
    int statusCode = 0;
    QByteArray body;
    QString errorMessage;
};

HttpResult getUrl(const QUrl &url, int timeoutMs = kNetworkTimeoutMs)
{
    HttpResult result;
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "CPlusPlus-project/1.0 (mailto:metadata@example.invalid)");

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        result.errorMessage = QStringLiteral("网络请求超时");
        reply->deleteLater();
        return result;
    }
    timer.stop();

    const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    result.statusCode = status.isValid() ? status.toInt() : 0;
    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        return result;
    }

    result.body = reply->readAll();
    result.success = result.statusCode >= 200 && result.statusCode < 300;
    if (!result.success) {
        result.errorMessage = QStringLiteral("HTTP %1").arg(result.statusCode);
    }
    reply->deleteLater();
    return result;
}

QString firstStringInArray(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString().trimmed();
    }
    if (!value.isArray()) {
        return QString();
    }
    const QJsonArray values = value.toArray();
    for (const QJsonValue &item : values) {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty()) {
            return text;
        }
    }
    return QString();
}

QString dateFromParts(const QJsonObject &message, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonObject dateObj = message.value(key).toObject();
        const QJsonArray datePartsOuter = dateObj.value(QStringLiteral("date-parts")).toArray();
        if (datePartsOuter.isEmpty()) {
            continue;
        }
        const QJsonArray parts = datePartsOuter.first().toArray();
        if (parts.isEmpty()) {
            continue;
        }
        const int year = parts.at(0).toInt();
        if (year <= 0) {
            continue;
        }
        const int month = parts.size() > 1 ? parts.at(1).toInt() : 0;
        const int day = parts.size() > 2 ? parts.at(2).toInt() : 0;
        if (month > 0 && day > 0) {
            return QStringLiteral("%1-%2-%3")
                .arg(year, 4, 10, QLatin1Char('0'))
                .arg(month, 2, 10, QLatin1Char('0'))
                .arg(day, 2, 10, QLatin1Char('0'));
        }
        if (month > 0) {
            return QStringLiteral("%1-%2")
                .arg(year, 4, 10, QLatin1Char('0'))
                .arg(month, 2, 10, QLatin1Char('0'));
        }
        return QString::number(year);
    }
    return QString();
}

QStringList authorsFromCrossref(const QJsonObject &message)
{
    QStringList authors;
    const QJsonArray authorArray = message.value(QStringLiteral("author")).toArray();
    for (const QJsonValue &value : authorArray) {
        const QJsonObject author = value.toObject();
        const QString given = author.value(QStringLiteral("given")).toString().trimmed();
        const QString family = author.value(QStringLiteral("family")).toString().trimmed();
        const QString name = QStringLiteral("%1 %2").arg(given, family).simplified();
        if (!name.isEmpty()) {
            authors << name;
        }
    }
    return authors;
}

QStringList keywordsFromCrossref(const QJsonObject &message)
{
    QStringList keywords;
    const QJsonArray subjects = message.value(QStringLiteral("subject")).toArray();
    for (const QJsonValue &value : subjects) {
        const QString keyword = value.toString().trimmed();
        if (!keyword.isEmpty()) {
            keywords << keyword;
        }
    }
    return keywords;
}

QString cleanCrossrefAbstract(QString value)
{
    value.replace(QRegularExpression(QStringLiteral(R"(<[^>]+>)")), QStringLiteral(" "));
    value.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    value.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    value.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    value.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    return value.simplified();
}

OnlineMetadataService::LookupResult crossrefResultFromWork(const QJsonObject &message)
{
    OnlineMetadataService::LookupResult result;
    result.success = true;
    result.sourceName = QStringLiteral("Crossref");
    result.doi = message.value(QStringLiteral("DOI")).toString().trimmed();
    result.title = firstStringInArray(message.value(QStringLiteral("title")));
    result.authors = authorsFromCrossref(message);
    result.abstractText = cleanCrossrefAbstract(message.value(QStringLiteral("abstract")).toString());
    result.keywords = keywordsFromCrossref(message);
    result.venue = firstStringInArray(message.value(QStringLiteral("container-title")));
    result.publicationDate = dateFromParts(message, {
        QStringLiteral("published-print"),
        QStringLiteral("published-online"),
        QStringLiteral("published"),
        QStringLiteral("issued")
    });
    result.volume = message.value(QStringLiteral("volume")).toString().trimmed();
    result.issue = message.value(QStringLiteral("issue")).toString().trimmed();
    result.pages = message.value(QStringLiteral("page")).toString().trimmed();
    result.publisher = message.value(QStringLiteral("publisher")).toString().trimmed();
    result.url = message.value(QStringLiteral("URL")).toString().trimmed();
    return result;
}

QString normalizeTitleForSimilarity(QString title)
{
    title = title.toLower();
    title.replace(QRegularExpression(QStringLiteral(R"([^a-z0-9\x{4e00}-\x{9fff}]+)")), QStringLiteral(" "));
    return title.simplified();
}

QSet<QString> titleTokens(const QString &title)
{
    QSet<QString> tokens;
    for (const QString &token : normalizeTitleForSimilarity(title).split(' ', Qt::SkipEmptyParts)) {
        if (token.size() >= 2) {
            tokens.insert(token);
        }
    }
    return tokens;
}

double titleSimilarity(const QString &a, const QString &b)
{
    const QString normalizedA = normalizeTitleForSimilarity(a);
    const QString normalizedB = normalizeTitleForSimilarity(b);
    if (normalizedA.isEmpty() || normalizedB.isEmpty()) {
        return 0.0;
    }
    if (normalizedA == normalizedB) {
        return 1.0;
    }

    const QSet<QString> tokensA = titleTokens(normalizedA);
    const QSet<QString> tokensB = titleTokens(normalizedB);
    if (tokensA.isEmpty() || tokensB.isEmpty()) {
        return 0.0;
    }

    int intersectionCount = 0;
    for (const QString &token : tokensA) {
        if (tokensB.contains(token)) {
            ++intersectionCount;
        }
    }
    const int unionCount = tokensA.size() + tokensB.size() - intersectionCount;
    if (unionCount <= 0) {
        return 0.0;
    }

    const double jaccard = static_cast<double>(intersectionCount) / static_cast<double>(unionCount);
    const double coverage = static_cast<double>(intersectionCount)
        / static_cast<double>(std::min(tokensA.size(), tokensB.size()));
    return std::max(jaccard, coverage * 0.92);
}

QString normalizeArxivId(QString arxivId)
{
    arxivId = arxivId.trimmed();
    arxivId.remove(QRegularExpression(QStringLiteral(R"(^arXiv\s*:\s*)"),
        QRegularExpression::CaseInsensitiveOption));
    arxivId.remove(QRegularExpression(QStringLiteral(R"(\s+)")));
    return arxivId;
}

OnlineMetadataService::LookupResult failure(const QString &message)
{
    OnlineMetadataService::LookupResult result;
    result.success = false;
    result.errorMessage = message;
    return result;
}
}

OnlineMetadataService::LookupResult OnlineMetadataService::lookupByDoi(const QString &doi)
{
    const QString cleanDoi = doi.trimmed();
    if (cleanDoi.isEmpty()) {
        return failure(QStringLiteral("DOI 为空"));
    }

    const QString encodedDoi = QString::fromLatin1(QUrl::toPercentEncoding(cleanDoi));
    const HttpResult http = getUrl(QUrl(QStringLiteral("https://api.crossref.org/works/%1").arg(encodedDoi)));
    if (!http.success) {
        return failure(http.errorMessage);
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(http.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return failure(QStringLiteral("Crossref 返回 JSON 解析失败"));
    }

    const QJsonObject message = doc.object().value(QStringLiteral("message")).toObject();
    if (message.isEmpty()) {
        return failure(QStringLiteral("Crossref 未返回作品信息"));
    }

    LookupResult result = crossrefResultFromWork(message);
    result.sourceName = QStringLiteral("Crossref");
    return result;
}

OnlineMetadataService::LookupResult OnlineMetadataService::lookupByArxivId(const QString &arxivId)
{
    const QString cleanId = normalizeArxivId(arxivId);
    if (cleanId.isEmpty()) {
        return failure(QStringLiteral("arXiv ID 为空"));
    }

    QUrl url(QStringLiteral("https://export.arxiv.org/api/query"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id_list"), cleanId);
    query.addQueryItem(QStringLiteral("max_results"), QStringLiteral("1"));
    url.setQuery(query);

    const HttpResult http = getUrl(url);
    if (!http.success) {
        return failure(http.errorMessage);
    }

    LookupResult result;
    result.sourceName = QStringLiteral("arXiv");
    result.arxivId = QStringLiteral("arXiv:%1").arg(cleanId);

    QXmlStreamReader xml(http.body);
    bool inEntry = false;
    bool inAuthor = false;
    QString currentAuthorName;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = xml.name().toString();
            if (name == QStringLiteral("entry")) {
                inEntry = true;
                continue;
            }
            if (!inEntry) {
                continue;
            }
            if (name == QStringLiteral("author")) {
                inAuthor = true;
                currentAuthorName.clear();
            } else if (name == QStringLiteral("name") && inAuthor) {
                currentAuthorName = xml.readElementText().simplified();
            } else if (name == QStringLiteral("title")) {
                result.title = xml.readElementText().simplified();
            } else if (name == QStringLiteral("summary")) {
                result.abstractText = xml.readElementText().simplified();
            } else if (name == QStringLiteral("published")) {
                result.publicationDate = xml.readElementText().left(10);
            } else if (name == QStringLiteral("id")) {
                result.url = xml.readElementText().trimmed();
            } else if (name == QStringLiteral("category")) {
                const QString term = xml.attributes().value(QStringLiteral("term")).toString().trimmed();
                if (!term.isEmpty() && !result.keywords.contains(term)) {
                    result.keywords << term;
                }
            } else if (name == QStringLiteral("link")) {
                const auto attrs = xml.attributes();
                const QString title = attrs.value(QStringLiteral("title")).toString();
                const QString href = attrs.value(QStringLiteral("href")).toString().trimmed();
                if (title.compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0 && !href.isEmpty()) {
                    result.url = href;
                }
            }
        } else if (xml.isEndElement()) {
            const QString name = xml.name().toString();
            if (name == QStringLiteral("author")) {
                inAuthor = false;
                if (!currentAuthorName.isEmpty()) {
                    result.authors << currentAuthorName;
                }
            } else if (name == QStringLiteral("entry")) {
                break;
            }
        }
    }

    if (xml.hasError()) {
        return failure(QStringLiteral("arXiv 返回 XML 解析失败"));
    }
    if (result.title.isEmpty()) {
        return failure(QStringLiteral("arXiv 未返回作品信息"));
    }

    result.success = true;
    result.venue.clear();
    return result;
}

OnlineMetadataService::LookupResult OnlineMetadataService::lookupByTitle(const QString &titleCandidate)
{
    const QString cleanTitle = titleCandidate.simplified();
    if (cleanTitle.size() < 5) {
        return failure(QStringLiteral("标题候选为空或过短"));
    }

    QUrl url(QStringLiteral("https://api.crossref.org/works"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query.title"), cleanTitle);
    query.addQueryItem(QStringLiteral("rows"), QStringLiteral("5"));
    url.setQuery(query);

    const HttpResult http = getUrl(url);
    if (!http.success) {
        return failure(http.errorMessage);
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(http.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return failure(QStringLiteral("Crossref 标题搜索 JSON 解析失败"));
    }

    const QJsonArray items = doc.object()
        .value(QStringLiteral("message")).toObject()
        .value(QStringLiteral("items")).toArray();
    if (items.isEmpty()) {
        return failure(QStringLiteral("Crossref 标题搜索无结果"));
    }

    double bestScore = 0.0;
    QJsonObject bestItem;
    for (const QJsonValue &itemValue : items) {
        const QJsonObject item = itemValue.toObject();
        const QString onlineTitle = firstStringInArray(item.value(QStringLiteral("title")));
        const double score = titleSimilarity(cleanTitle, onlineTitle);
        if (score > bestScore) {
            bestScore = score;
            bestItem = item;
        }
    }

    if (bestScore < kTitleSimilarityThreshold || bestItem.isEmpty()) {
        return failure(QStringLiteral("标题搜索结果相似度不足"));
    }

    LookupResult result = crossrefResultFromWork(bestItem);
    result.sourceName = QStringLiteral("Crossref title search");
    result.titleSimilarity = bestScore;
    return result;
}

bool OnlineMetadataService::isNetworkAvailable()
{
    QUrl url(QStringLiteral("https://api.crossref.org/works"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("rows"), QStringLiteral("0"));
    url.setQuery(query);
    const HttpResult http = getUrl(url, 3000);
    return http.success;
}
