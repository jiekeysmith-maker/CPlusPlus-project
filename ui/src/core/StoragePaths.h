#ifndef STORAGE_PATHS_H
#define STORAGE_PATHS_H

#include <QString>

namespace StoragePaths {

QString projectRootPath();
QString dataRootPath();
QString libraryDirectoryPath();
QString pdfDirectoryPath();
QString authorsDirectoryPath();
QString sourcesDirectoryPath();
QString papersDirectoryPath();
QString attachmentsDirectoryPath();
QString catalogsDirectoryPath();
QString legacyAppDataPath();
bool ensureDirectory(const QString &path);

}

#endif // STORAGE_PATHS_H
