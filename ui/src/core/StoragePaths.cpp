#include "StoragePaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>

namespace {

bool looksLikeProjectRoot(const QDir &dir)
{
    return dir.exists(QStringLiteral(".git"))
        || dir.exists(QStringLiteral("ui/CMakeLists.txt"));
}

QString findProjectRootFrom(const QString &startPath)
{
    if (startPath.isEmpty()) {
        return QString();
    }

    QDir dir(startPath);
    if (!dir.exists()) {
        return QString();
    }

    while (true) {
        if (looksLikeProjectRoot(dir)) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QString();
}

QString ensureSubDirectory(const QString &name)
{
    QDir root(StoragePaths::dataRootPath());
    root.mkpath(name);
    return root.filePath(name);
}

}

namespace StoragePaths {

bool ensureDirectory(const QString &path)
{
    return !path.isEmpty() && QDir().mkpath(path);
}

QString projectRootPath()
{
    static QString cachedRoot;
    if (!cachedRoot.isEmpty()) {
        return cachedRoot;
    }

    const QStringList starts = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };
    for (const QString &start : starts) {
        const QString found = findProjectRootFrom(start);
        if (!found.isEmpty()) {
            cachedRoot = found;
            return cachedRoot;
        }
    }

    cachedRoot = QCoreApplication::applicationDirPath();
    return cachedRoot;
}

QString dataRootPath()
{
    QDir root(projectRootPath());
    const QString path = root.filePath(QStringLiteral("data"));
    ensureDirectory(path);
    return path;
}

QString libraryDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("library"));
}

QString pdfDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("pdfs"));
}

QString authorsDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("authors"));
}

QString sourcesDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("sources"));
}

QString papersDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("papers"));
}

QString attachmentsDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("attachments"));
}

QString catalogsDirectoryPath()
{
    return ensureSubDirectory(QStringLiteral("catalogs"));
}

QString legacyAppDataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

}
